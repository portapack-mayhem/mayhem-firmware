"""Actual AM execute loop, independent convolution/spectra, and frozen SSB checks."""
from pathlib import Path
import re
import subprocess
import tempfile
import unittest
import numpy as np
from scipy.signal import lfilter, upfirdn
from design_mic_am_decimator import read_taps, metrics
from design_mic_ssb_interp import read_taps as x8_taps
from design_mic_ssb_interp_x2 import read_taps as x2_taps
ROOT = Path(__file__).resolve().parents[2]
BASE = '510d579d7405ca899963691513c46de74943e9c0'


def body(text, name):
    start = text.index('void AM::execute(')
    end = text.index('\n}  // namespace modulate', start)
    return text[start:end].replace('AM::execute', name+'::execute')


def source():
    current = (ROOT/'firmware/baseband/dsp_modulate.cpp').read_text()
    old = subprocess.check_output(['git','show',BASE+':firmware/baseband/dsp_modulate.cpp'], cwd=ROOT, text=True)
    signature = current[current.index('void AM::execute('):].split('{',1)[0].replace('void AM::','void ')
    pre = r'''
#include <array>
#include <complex>
#include <cmath>
#include <cstdint>
#include <cassert>
#include <iostream>
#include <cstring>
#include <vector>
#include "dsp_am_interpolate.hpp"
#include "dsp_am_output.hpp"
struct buffer_s16_t {int16_t* p; size_t count;};
struct buffer_c8_t {std::complex<int8_t>* p; size_t count;};
struct TXProgressMessage { bool done=true; };
struct AudioLevelReportMessage {uint32_t value=0;};
struct Queue {
 unsigned progress=0;
 bool push(const TXProgressMessage&){++progress;return true;}
 bool push(const AudioLevelReportMessage&){return true;}
} queue;
struct Shared { Queue& application_queue; } shared_memory{queue};
enum class Mode {AM,DSB};
'''
    # Use the actual tone table, ToneGen process/configure and apply_beep logic.
    pre += '\n#include "sine_table_int8.hpp"\n#include "tonesets.hpp"\n'
    tone = (ROOT/'firmware/baseband/tone_gen.cpp').read_text()
    pre += '''class ToneGen {public: void configure(uint32_t,float); int32_t process(int32_t); uint32_t delta_{0},tone_phase_{0}; float tone_mix_weight_{0},input_mix_weight_{0};};\n'''
    a=tone.index('void ToneGen::configure('); b=tone.index('\n}',tone.index('int32_t ToneGen::process(',a))+2
    pre += tone[a:b]
    pre += r'''
class Modulator {public:
 Mode mode=Mode::AM; float audio_gain=1; unsigned audio_shift_bits_s16_AM_DSB_SSB=2,over=64;
 bool play_beep=false; uint64_t power_acc=0; static constexpr uint32_t baseband_fs=1536000;
 ToneGen beep_gen; unsigned beep_calls=0;
 int32_t apply_beep(int32_t,bool&,uint32_t&,uint32_t&,TXProgressMessage&);
};
'''
    a=current.index('int32_t Modulator::apply_beep('); b=current.index('\n}',a)+2
    pre += current[a:b].replace('    if (play_beep)', '    ++beep_calls;\n    if (play_beep)',1)
    for cls in ['AM','Old']:
        pre += '\nclass '+cls+' : public Modulator {public:\n'+signature+';\n'
        pre += 'dsp::am::Microphone microphone_{}; bool was_beep_{false};\n};\n'
    # Remove old signed-shift UB only in reference harness, preserving intended beep value.
    pre += body(old,'Old').replace(') << 5;', ') * 32;')
    pre += body(current,'AM')
    return pre + r'''
int main(int argc,char** argv){
 const char mode=argc>1?argv[1][0]:'t';
 if(mode=='d'||mode=='8'||mode=='2'||mode=='c'){
  dsp::am::Decimate2 dec;dsp::am::Interpolate8 fir8;dsp::am::Interpolate2 fir2;dsp::am::Microphone mic;
  float x,y;
  while(std::cin.read(reinterpret_cast<char*>(&x),4)){
   if(mode=='d'||mode=='c'){
    assert(std::cin.read(reinterpret_cast<char*>(&y),4));
    if(mode=='d'){float z=dec.execute(x,y);std::cout.write(reinterpret_cast<char*>(&z),4);}
    else{std::array<float,16> z;mic.execute(x,y,1,z);std::cout.write(reinterpret_cast<char*>(z.data()),64);}
   }else if(mode=='8'){std::array<float,8> z;fir8.execute(x,z);std::cout.write(reinterpret_cast<char*>(z.data()),32);}
   else{float a,b;fir2.execute(x,a,b);std::cout.write(reinterpret_cast<char*>(&a),4);std::cout.write(reinterpret_cast<char*>(&b),4);}
  }return 0;
 }
 if(mode=='a'||mode=='s'||mode=='o'||mode=='p'){
  AM am;Old old;bool carrier=mode=='a'||mode=='o';am.mode=old.mode=carrier?Mode::AM:Mode::DSB;
  int16_t in[32];std::complex<int8_t> out[2048];bool cfg=true;uint32_t bi=0,bt=0,c=0,d=76800;TXProgressMessage tx;AudioLevelReportMessage level;
  while(std::cin.read(reinterpret_cast<char*>(in),sizeof(in))){
   if(mode=='a'||mode=='s')am.execute({in,32},{out,2048},cfg,bi,bt,tx,level,c,d);
   else old.execute({in,32},{out,2048},cfg,bi,bt,tx,level,c,d);
   std::cout.write(reinterpret_cast<char*>(out),sizeof(out));
  }return 0;
 }
 // Direct equivalence of microphone/beep safety conversion, including
 // fractional endpoints/nonfinite values; neither path has an observer.
 for(bool carrier:{false,true}){
  for(float v:{-1000.f,-191.f,-129.f,-128.999f,-128.25f,-128.f,-127.9f,0.f,63.f,64.f,126.9f,127.f,127.25f,127.999f,128.f,1000.f,NAN,INFINITY,-INFINITY}){
   assert(dsp::am::beep_output(v,carrier)==dsp::am::output(v,carrier));
  }
  for(int n=-128000;n<=127000;n++){
   float v=n/1000.f;
   assert(dsp::am::beep_output(v,carrier)==dsp::am::output(v,carrier));
  }
 }
 assert(dsp::am::output(0,true)==63);assert(dsp::am::output(-63,true)==0);assert(dsp::am::output(63,true)==126);
 for(float v:{126.9f,127.f,127.25f,127.999f,128.f,-127.9f,-128.f,-128.25f,-128.999f,-129.f}){
  const int expected=v>127?127:v< -128?-128:int(v);
  assert(dsp::am::output(v,false)==expected);
 }
 for(float v:{NAN,INFINITY,-INFINITY})assert(dsp::am::output(v,false)==0);
 for(int n=-128000;n<=127000;n++){float v=n/1000.f;assert(dsp::am::output(v,false)==int8_t(v));}
 for(auto m:{Mode::AM,Mode::DSB})for(unsigned shift:{0,1,2,3})for(float gain:{.5f,1.f,1.5f,2.f}){
  AM am;Old old;am.mode=old.mode=m;am.audio_gain=old.audio_gain=gain;
  am.audio_shift_bits_s16_AM_DSB_SSB=old.audio_shift_bits_s16_AM_DSB_SSB=shift;
  int16_t in[32];std::complex<int8_t> out[2048],original[2048];
  bool cfg=true;uint32_t bi=0,bt=0,c=0,oc=0,d=76800;TXProgressMessage tx;AudioLevelReportMessage level,ol;
  for(int block=0;block<100;block++){
   for(int k=0;k<32;k++)in[k]=int16_t(1000*std::cos((block*32+k)*.527));
   am.execute({in,32},{out,2048},cfg,bi,bt,tx,level,c,d);
   old.execute({in,32},{original,2048},cfg,bi,bt,tx,ol,oc,d);
   assert(am.power_acc==old.power_acc && level.value==ol.value && c==oc);
   for(int k=0;k<2048;k++){assert(out[k].real()==out[k].imag());assert(out[k]==out[k&~7]);}
  }
 }
 // Actual full-rate tone generator, timing and AM/DSB output versus old path.
 for(auto m:{Mode::AM,Mode::DSB}){
  AM am;Old old;am.mode=old.mode=m;am.play_beep=old.play_beep=true;
  int16_t in[32]{};std::complex<int8_t> out[2048],original[2048];
  bool cfg=true,ocfg=true;uint32_t bi=0,bt=0,obi=0,obt=0,c=0,oc=0,d=76800;TXProgressMessage tx;AudioLevelReportMessage level,ol;
  // Warm microphone history before switching to beep, then require clean resume.
  am.play_beep=false;
  for(auto& v:in)v=16000;
  am.execute({in,32},{out,2048},cfg,bi,bt,tx,level,c,d);
  am.play_beep=true;
  for(auto& v:in)v=0;
  for(int block=0;block<230;block++){
   am.execute({in,32},{out,2048},cfg,bi,bt,tx,level,c,d);
   old.execute({in,32},{original,2048},ocfg,obi,obt,tx,ol,oc,d);
   assert(cfg==ocfg && bi==obi && bt==obt);
   for(int k=0;k<2048;k++)assert(int(out[k].real())==int(original[k].real())-(m==Mode::AM?17:0));
   if(!cfg)break;
  }
  assert(!cfg && am.beep_calls==old.beep_calls && am.beep_calls>460000);
  // Resume microphone defensively: no old FIR state survives the beep.
  am.play_beep=false;am.execute({in,32},{out,2048},cfg,bi,bt,tx,level,c,d);
  for(auto z:out)assert(z.real()==(m==Mode::AM?63:0));
 }
 // Overloaded microphone reaches both protected C8 endpoints.
 for(auto m:{Mode::AM,Mode::DSB}){
  AM am;am.mode=m;am.audio_gain=2;am.audio_shift_bits_s16_AM_DSB_SSB=0;
  int16_t in[32];std::complex<int8_t> out[2048];
  bool cfg=true;uint32_t bi=0,bt=0,c=0,d=76800;TXProgressMessage tx;AudioLevelReportMessage level;
  for(int block=0;block<20;block++){
   for(int k=0;k<32;k++)in[k]=int16_t(30000*std::cos((block*32+k)*.5235987756));
   am.execute({in,32},{out,2048},cfg,bi,bt,tx,level,c,d);
  }
  bool positive=false,negative=false;
  for(auto z:out){positive |= z.real()==127;negative |= z.real()==-128;}
  assert(positive && negative);

 }
 std::cerr<<"PASS actual AM/DSB loop, VU all gains/shifts, hold8, clamp, beep timing/scaling, state reset\n";
}
'''


class AmTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp=tempfile.TemporaryDirectory(); p=Path(cls.tmp.name); (p/'test.cpp').write_text(source()); cls.exe=p/'test'
        subprocess.run(['g++','-std=c++17','-O2','-ffp-contract=off','-fsanitize=undefined','-fno-sanitize-recover=all','-I'+str(ROOT/'firmware/baseband'),'-I'+str(ROOT/'firmware/common'),str(p/'test.cpp'),'-o',str(cls.exe)],check=True)
    @classmethod
    def tearDownClass(cls):cls.tmp.cleanup()
    def run_float(self,mode,x):
        return np.frombuffer(subprocess.check_output([str(self.exe),mode],input=np.asarray(x,dtype=np.float32).tobytes()),dtype=np.float32)
    def run_audio(self,mode,x):
        return np.frombuffer(subprocess.check_output([str(self.exe),mode],input=np.asarray(x,dtype=np.int16).tobytes()),dtype=np.int8).reshape(-1,2)
    def test_actual_loop(self):subprocess.run([str(self.exe)],check=True)
    def test_independent_convolution(self):
        rng=np.random.default_rng(52); impulse=np.zeros(3200);impulse[0]=1
        for x in [impulse,rng.uniform(-10000,10000,3200),np.full(3200,1000.)]:
            for mode in ['d','8','2','c']:
                if mode=='d':ref=lfilter(read_taps().astype(float),[1],x)[1::2]
                elif mode=='8':ref=upfirdn(x8_taps().astype(float),x,up=8)[:len(x)*8]
                elif mode=='2':ref=upfirdn(x2_taps().astype(float),x,up=2)[:len(x)*2]
                else:
                    dec=lfilter(read_taps().astype(float),[1],x)[1::2]
                    ref=upfirdn(x8_taps().astype(float),dec,up=8)[:len(dec)*8]
                    ref=upfirdn(x2_taps().astype(float),ref,up=2)[:len(dec)*16]
                np.testing.assert_allclose(self.run_float(mode,x),ref,atol=.008,rtol=2e-6)
    def test_filter_response_independent_dft(self):
        h=read_taps().astype(float);self.assertEqual(len(h),35);np.testing.assert_array_equal(h,h[::-1])
        # Direct complex-exponential sum, independent of freqz/designer.
        f=np.unique(np.r_[np.linspace(0,3000,20001),np.linspace(6000,12000,40001)])
        H=np.abs(np.exp(-2j*np.pi*f[:,None]*np.arange(35)/24000)@h)
        p=H[f<=3000];stop=H[f>=6000]
        ripple=20*np.log10(p.max()/p.min());rejection=20*np.log10(stop.max())
        self.assertLess(ripple,.0034);self.assertLess(rejection,-74);self.assertAlmostEqual(h.sum(),1,places=6)
        print('Independent DFT:',dict(dc=h.sum(),ripple_db=ripple,stopband_db=rejection,delay_us=34/48000*1e6))
        f=np.linspace(0,3000,10001)
        def response(taps,rate):return np.exp(-2j*np.pi*f[:,None]*np.arange(len(taps))/rate)@taps
        combined=abs(response(h,24000)*response(x8_taps().astype(float)/8,96000)*response(x2_taps().astype(float)/2,192000))
        cr=20*np.log10(combined.max()/combined.min());self.assertLess(cr,.0074)
        print('Combined passband ripple dB',cr)

    def test_actual_output_reference(self):
        rng=np.random.default_rng(9);x=rng.integers(-15000,15000,3200,dtype=np.int16)
        reconstructed=self.run_float('c',x.astype(np.float32)) / 512
        for mode,carrier in [('a',63),('s',0)]:
            out=self.run_audio(mode,x);scalar=np.clip(reconstructed+carrier,-128,127)
            expected=np.repeat(np.trunc(scalar).astype(np.int8),8)
            # Float accumulation order can differ at integer boundaries; this
            # uses actual C++ chain and power-of-two gain, so require exact C8.
            np.testing.assert_array_equal(out[:,0],expected);np.testing.assert_array_equal(out[:,0],out[:,1])
    def test_unchanged_hilbert_and_fm(self):
        for name in ['dsp_hilbert.cpp','dsp_hilbert.hpp']:
            path='firmware/baseband/'+name
            self.assertEqual((ROOT/path).read_bytes(),subprocess.check_output(['git','show',BASE+':'+path],cwd=ROOT))
        path='firmware/baseband/dsp_modulate.cpp';cur=(ROOT/path).read_text();old=subprocess.check_output(['git','show',BASE+':'+path],cwd=ROOT,text=True)
        # FM modulation remains identical to the public baseline.
        self.assertEqual(cur[cur.index('FM::FM()'):cur.index('AM::AM()')],
                         old[old.index('FM::FM()'):old.index('AM::AM()')])

    def test_spectra(self):
        fs=1536000;n=24000;t=np.arange(n)/24000
        for tone in [1000,2000,3000]:
            pure=16000*np.cos(2*np.pi*tone*t)
            x=np.rint(pure).astype(np.int16)
            # Separate linear reconstruction rejection from later C8 quantization.
            linear=np.repeat(self.run_float('c',pure),8)[-fs//2:]
            la=abs(np.fft.rfft(linear))/len(linear)
            for image in [12000-tone,12000+tone]:
                self.assertLess(20*np.log10(la[image//2]/la[tone//2]),-65)

            for new,old in [('a','o'),('s','p')]:
                # Last 0.5 seconds is coherent for all test tones, after settling.
                def amps(mode):
                    z=self.run_audio(mode,x)[-fs//2:,0].astype(float)
                    return abs(np.fft.rfft(z))/len(z)
                a,b=amps(new),amps(old);wanted=tone//2
                oldimage=20*np.log10(b[(12000-tone)//2]/b[wanted]); residual=20*np.log10(max(a[(12000-tone)//2],1e-15)/a[wanted])
                far=20*np.log10(a[(192000-tone)//2]/a[wanted])
                theory=20*np.log10(abs(np.sin(np.pi*tone/fs)/np.sin(np.pi*(192000-tone)/fs)))
                self.assertLess(residual,oldimage-25);self.assertAlmostEqual(far,theory,places=3)
                upper=20*np.log10(a[(192000+tone)//2]/a[wanted])
                upper_theory=20*np.log10(abs(np.sin(np.pi*tone/fs)/np.sin(np.pi*(192000+tone)/fs)))
                self.assertAlmostEqual(upper,upper_theory,places=3)
                self.assertLess(20*np.log10(max(a[(12000+tone)//2],1e-15)/a[wanted]),20*np.log10(b[(12000+tone)//2]/b[wanted])-20)

                print('spectrum',new,tone,'old12k',oldimage,'new residual12k',residual,'new192k',far)
        # 100% AM: amplitude ~=32256 codec counts at shift2/gain1.
        x=np.rint(32256*np.cos(2*np.pi*2000*t)).astype(np.int16)
        z=self.run_audio('a',x)[-fs//2:,0].astype(float);a=abs(np.fft.rfft(z))/len(z)
        ratio=20*np.log10(a[1000]/a[0]);self.assertAlmostEqual(ratio,-6.0206,delta=.15)
        self.assertGreaterEqual(z.min(),0);self.assertLessEqual(z.max(),127)
        print('100% AM sideband/carrier dBc',ratio,'C8 range',z.min(),z.max())

if __name__=='__main__':unittest.main()
