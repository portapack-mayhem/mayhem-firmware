"""Actual x16 SSB loop regression and independent float convolution checks."""
from pathlib import Path
import subprocess
import tempfile
import unittest
import numpy as np
from scipy.signal import upfirdn
from test_mic_ssb_interp_x8 import ROOT, harness, ssb_body
from design_mic_ssb_interp import read_taps as x8_taps
from design_mic_ssb_interp_x2 import read_taps as x2_taps, metrics

BASE = '510d579d7405ca899963691513c46de74943e9c0'

def source():
    s = harness()
    s = s.replace('#include <array>', '#include <array>\n#include "dsp_ssb_clamp.hpp"\nusing dsp::modulate::ssb_clamp;')
    old = subprocess.check_output(['git','show','510d579d7405ca899963691513c46de74943e9c0:firmware/baseband/dsp_modulate.cpp'],cwd=ROOT,text=True)
    s = s.replace('#include "dsp_interpolate_x8.hpp"','#include "dsp_interpolate_x8.hpp"\n#include "dsp_interpolate_x2.hpp"')
    s = s.replace('Mode mode=Mode::USB;', 'dsp::interpolate::ComplexFIRInterpolate2 interpolator_x2;\n Mode mode=Mode::USB;')
    start=s.index(' if(argc>1){'); end=s.index(' for(auto mode:', start)
    s=s[:start]+r'''
 if(argc>1){
  using Sample=dsp::interpolate::ComplexFIRInterpolate8::Sample;
  dsp::interpolate::ComplexFIRInterpolate8 fir8;
  dsp::interpolate::ComplexFIRInterpolate2 fir2;
  Sample input,even,odd;
  while(std::cin.read(reinterpret_cast<char*>(&input),sizeof(input))){
   if(argv[1][0]=='2'){
    fir2.execute(input,even,odd);
    std::cout.write(reinterpret_cast<char*>(&even),sizeof(even));
    std::cout.write(reinterpret_cast<char*>(&odd),sizeof(odd));
   }else{
    dsp::interpolate::ComplexFIRInterpolate8::Output out;
    fir8.execute(input.i,input.q,out);
    for(auto v:out){
     fir2.execute(v,even,odd);
     std::cout.write(reinterpret_cast<char*>(&even),sizeof(even));
     std::cout.write(reinterpret_cast<char*>(&odd),sizeof(odd));
    }
   }
  }
  return 0;
 }
'''+s[end:]
    s=s.replace('HilbertTransform reference_hilbert;', 'dsp::interpolate::ComplexFIRInterpolate2 reference_x2;\n  HilbertTransform reference_hilbert;')
    s=s.replace('    for(int p=0;p<8;p++){\n     float si=reconstructed[p].i*256, sq=reconstructed[p].q*256;', '''    for(int p=0;p<8;p++){
     dsp::interpolate::ComplexFIRInterpolate8::Sample pair[2];
     reference_x2.execute(reconstructed[p],pair[0],pair[1]);
     for(int k=0;k<2;k++){
     float si=pair[k].i*256, sq=pair[k].q*256;''')
    s=s.replace('for(int r=0;r<16;r++)expected[128*n+16*p+r]=c;', 'for(int r=0;r<8;r++)expected[128*n+16*p+8*k+r]=c;\n     }')
    return s.replace('3k FIR+hold16','3k FIR x8+x2+hold8')

class X16Tests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp=tempfile.TemporaryDirectory()
        p=Path(cls.tmp.name);(p/'test.cpp').write_text(source());cls.exe=p/'test'
        subprocess.run(['g++','-std=c++17','-O2','-ffp-contract=off','-I'+str(ROOT/'firmware/baseband'),str(p/'test.cpp'),'-o',str(cls.exe)],check=True)
    @classmethod
    def tearDownClass(cls):cls.tmp.cleanup()
    def test_actual_ssb_integration(self):
        print(subprocess.check_output([str(self.exe)],text=True).strip())
    def test_independent_convolution(self):
        rng=np.random.default_rng(37)
        impulse=np.zeros((4096,2),dtype=np.float32);impulse[0]=[.25,-.125]
        random=rng.uniform(-.4,.4,(4096,2)).astype(np.float32)
        for mode in ['2','16']:
            fs=96000 if mode=='2' else 12000
            t=np.arange(4096)/fs
            tone=np.column_stack([.25*np.cos(2*np.pi*2003.7*t),.25*np.sin(2*np.pi*2003.7*t)]).astype(np.float32)
            for x in [impulse,random,tone]:
                with self.subTest(mode=mode,signal='impulse/random/tone'):
                    actual=np.frombuffer(subprocess.check_output([str(self.exe),mode],input=x.tobytes()),dtype=np.float32).reshape(-1,2)
                    ref=x.astype(float)
                    if mode=='16':ref=upfirdn(x8_taps().astype(float),ref,up=8,axis=0)[:8*len(x)]
                    ref=upfirdn(x2_taps().astype(float),ref,up=2,axis=0)[:len(actual)]
                    self.assertLess(float(np.max(abs(actual-ref))),2e-7)
    def test_response_and_unchanged_fm_dma(self):
        result=metrics(x2_taps())
        self.assertLess(result['combined_ripple_db'],.005)
        self.assertLess(result['combined_stopband_db'],-70)
        for name in ['baseband_dma.cpp']:
            path='firmware/baseband/'+name
            self.assertEqual((ROOT/path).read_bytes(),subprocess.check_output(['git','show',BASE+':'+path],cwd=ROOT))
        current=(ROOT/'firmware/baseband/dsp_modulate.cpp').read_text()
        base=subprocess.check_output(['git','show',BASE+':firmware/baseband/dsp_modulate.cpp'],cwd=ROOT,text=True)
        # Check that FM processing remains identical to the public baseline.
        self.assertEqual(current[current.index('FM::FM()'):current.index('AM::AM()')],
                         base[base.index('FM::FM()'):base.index('AM::AM()')])

if __name__=='__main__':unittest.main()
