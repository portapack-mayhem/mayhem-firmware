"""Safety boundaries and actual SSB integration after observer removal."""
from pathlib import Path
import subprocess
import tempfile
import unittest
from test_mic_ssb_interp_x16 import source, ROOT
from test_mic_ssb_interp_x8 import ssb_body

CPP = r'''
int main(){
 const float values[]={126.9f,127.f,127.25f,127.99f,128.f,-127.9f,-128.f,-128.25f,-128.99f,-129.f};
 for(unsigned k=0;k<2;k++)for(float v:values){
  float y=ssb_clamp(v/256.f,v);
  assert(y==(v>127.f?127.f:v< -128.f?-128.f:v));
 }
 for(float v:{INFINITY,-INFINITY,NAN})assert(ssb_clamp(v,v)==0.f);
 // Below-limit values preserve the float exactly, not just the C8 result.
 for(int n=-128000;n<=127000;n++){
  float v=n/1000.f;assert(ssb_clamp(v/256,v)==v);
 }
 for(auto mode:{Mode::USB,Mode::LSB}){
  Observed obs;obs.mode=mode;obs.audio_gain=16;
  HilbertTransform h;dsp::interpolate::ComplexFIRInterpolate8 fir8;
  dsp::interpolate::ComplexFIRInterpolate2 fir2;
  int16_t audio[32];std::complex<int8_t> out[2048];
  bool cfg=true;uint32_t beep=0,timer=0,count=10000000,div=10000000;
  TXProgressMessage tx;AudioLevelReportMessage message;
  uint32_t pos[2]{},neg[2]{};
  float peaks[2]{};
  for(int block=0;block<100;block++){
   for(int k=0;k<32;k++)audio[k]=int16_t(30000*std::cos(2*3.141592653589793*2003.7*(32*block+k)/24000));
   obs.execute({audio,32},{out,2048},cfg,beep,timer,tx,message,count,div);
   for(int n=0;n<16;n++){
    float i,q;h.execute(((audio[2*n]>>2)*16)/32768.f,i,q);
    dsp::interpolate::ComplexFIRInterpolate8::Output stage;
    fir8.execute(i,q,stage);
    for(int p=0;p<8;p++){
     dsp::interpolate::ComplexFIRInterpolate8::Sample pair[2];
     fir2.execute(stage[p],pair[0],pair[1]);
     for(int k=0;k<2;k++){
      float v[2]{pair[k].i*256,pair[k].q*256};
      for(int c=0;c<2;c++){
       peaks[c]=std::max(peaks[c],std::fabs(v[c]));
       pos[c]+=v[c]>127;neg[c]+=v[c]<-128;
       v[c]=std::max(-128.f,std::min(127.f,v[c]));
      }
      std::complex<int8_t> expected=mode==Mode::USB?std::complex<int8_t>{int8_t(v[0]),int8_t(v[1])}:std::complex<int8_t>{int8_t(v[1]),int8_t(v[0])};
      for(int r=0;r<8;r++)assert(out[n*128+p*16+k*8+r]==expected);
     }
    }
   }
  }
  for(int c=0;c<2;c++){
   assert(pos[c]>0 && neg[c]>0);
  }
 }
 std::cout<<"PASS clamp endpoints/fractional overrange/nonfinite/in-range identity and actual USB/LSB overloaded loop\n";
}
'''

class ClampTests(unittest.TestCase):
    def test_in_range_byte_equivalence(self):
        baseline=subprocess.check_output(['git','show','510d579d7405ca899963691513c46de74943e9c0:firmware/baseband/dsp_modulate.cpp'],cwd=ROOT,text=True)
        current=(ROOT/'firmware/baseband/dsp_modulate.cpp').read_text()
        unclamped=current.replace('i = ssb_clamp(raw_i, i);','').replace('q = ssb_clamp(raw_q, q);','')
        code=source().replace(ssb_body(baseline,'Baseline'),ssb_body(unclamped,'Baseline'))
        code=code.replace('assert(mo.value==mb.value', 'assert(!std::memcmp(out,original,sizeof(out)));\n   assert(mo.value==mb.value')
        with tempfile.TemporaryDirectory() as tmp:
            p=Path(tmp);(p/'test.cpp').write_text(code)
            subprocess.run(['g++','-std=c++17','-O2','-ffp-contract=off','-I'+str(ROOT/'firmware/baseband'),str(p/'test.cpp'),'-o',str(p/'test')],check=True)
            subprocess.run([str(p/'test')],check=True)

    def test_actual_cpp(self):
        with tempfile.TemporaryDirectory() as tmp:
            p=Path(tmp);(p/'test.cpp').write_text(source().split('int main(int argc,char** argv)')[0]+'\n'+CPP)
            subprocess.run(['g++','-std=c++17','-O2','-ffp-contract=off','-I'+str(ROOT/'firmware/baseband'),str(p/'test.cpp'),'-o',str(p/'test')],check=True)
            print(subprocess.check_output([str(p/'test')],text=True).strip())

if __name__=='__main__':unittest.main()
