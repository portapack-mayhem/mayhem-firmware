"""Compile and exercise actual FIR and SSB loops on the host; never build firmware.

Reference SSB loop is the upstream x256 loop at the public baseline commit. Hilbert algorithm/coefficient/biquad bodies come from the
current tree; only hardware messaging/buffer plumbing is stubbed.
"""
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import unittest
import numpy as np
from scipy.signal import upfirdn

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools'))
from design_mic_ssb_interp import read_taps,metrics


def ssb_body(text, name):
    start=text.index('void SSB::execute(')
    end=text.index('\n///\n\nFM::FM()',start)
    return text[start:end].replace('SSB::execute',name+'::execute')


def harness(observed_source=None):
    old=subprocess.check_output(['git','show','510d579d7405ca899963691513c46de74943e9c0:firmware/baseband/dsp_modulate.cpp'],cwd=ROOT,text=True)
    new=observed_source if observed_source is not None else (ROOT/'firmware/baseband/dsp_modulate.cpp').read_text()
    sig=ssb_body(new,'Observed').split('{')[0].replace('Observed::','')+';'
    hilbert=(ROOT/'firmware/baseband/dsp_hilbert.cpp').read_text()
    hilbert=hilbert[hilbert.index('HilbertTransform::HilbertTransform()'):hilbert.index('Real_to_Complex::Real_to_Complex()')]
    coeff=(ROOT/'firmware/common/dsp_sos_config.hpp').read_text()
    coeff=coeff[coeff.index('constexpr iir_biquad_df2_config_t half_band'):coeff.index('// scipy.signal.iirfilter',coeff.index('constexpr iir_biquad_df2_config_t half_band'))]
    iir=(ROOT/'firmware/common/dsp_iir.cpp').read_text()
    iir=iir[iir.index('void IIRBiquadDF2Filter::configure'):]
    # The DF2 functions are the last definitions in this source file.
    return r'''
#include <array>
#include <cassert>
#include <cmath>
#include <complex>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>
#include "dsp_interpolate_x8.hpp"
using iir_biquad_df2_config_t=std::array<float,6>;
struct IIRBiquadDF2Filter {
 float b0,b1,b2,a1,a2,z0=0,z1=0;
 void configure(const iir_biquad_df2_config_t&);
 float execute(float);
};
'''+iir+coeff+r'''
struct SOS {std::array<IIRBiquadDF2Filter,5> filters;
 void configure(const iir_biquad_df2_config_t* c){for(int k=0;k<5;k++)filters[k].configure(c[k]);}
 float execute(float x){for(auto& f:filters)x=f.execute(x);return x;}
};
struct HilbertTransform {int n;SOS sos_input,sos_i,sos_q;
 HilbertTransform();void execute(float,float&,float&);
};
'''+hilbert+r'''
template<class T> struct buffer_t {T* p;size_t count;};
using buffer_s16_t=buffer_t<int16_t>;using buffer_c8_t=buffer_t<std::complex<int8_t>>;
struct TXProgressMessage{};
'''+r'''
struct AudioLevelReportMessage{uint32_t value=0;};
struct Queue{bool push(const AudioLevelReportMessage&){return true;}};
struct Shared{Queue application_queue;} shared_memory;
enum class Mode{None,AM,DSB,LSB,USB,FM};
struct Fields{
 HilbertTransform hilbert;dsp::interpolate::ComplexFIRInterpolate8 interpolator;
 Mode mode=Mode::USB;int fs_div_factor=128;uint32_t over=64;
 uint8_t audio_shift_bits_s16_AM_DSB_SSB=2;float audio_gain=1;
 uint64_t power_acc=0;
};
'''+('struct Observed:Fields{'+sig+'};\nstruct Baseline:Fields{'+sig+'};\n')+ssb_body(old,'Baseline')+ssb_body(new,'Observed')+r'''
int main(int argc,char** argv){
 if(argc>1){
  // Noncoherent complex tone, two separate components, actual float kernel.
  dsp::interpolate::ComplexFIRInterpolate8 fir;
  dsp::interpolate::ComplexFIRInterpolate8::Output out;
  for(int n=0;n<4096;n++){
   float i=.25f*std::cos(2*3.141592653589793*2003.7*n/12000);
   float q=.25f*std::sin(2*3.141592653589793*2003.7*n/12000);
   fir.execute(i,q,out);
   for(auto x:out){std::cout.write(reinterpret_cast<char*>(&x),sizeof(x));}
  }
  return 0;
 }
 for(auto mode:{Mode::USB,Mode::LSB})for(int divider:{128,192}){
  Observed obs;Baseline base;obs.mode=base.mode=mode;obs.fs_div_factor=base.fs_div_factor=divider;
  HilbertTransform reference_hilbert;dsp::interpolate::ComplexFIRInterpolate8 reference_fir;
  int16_t audio[32];std::complex<int8_t> out[2048],original[2048],expected[2048];
  bool configured=true;uint32_t beep=0,timer=0,co=0,cb=0,div=76800;
  TXProgressMessage tx;AudioLevelReportMessage mo,mb;
  for(int block=0;block<100;block++){
   for(int k=0;k<32;k++)audio[k]=int16_t(12000*std::cos(2*3.141592653589793*2003.7*(block*32+k)/24000));
   obs.execute({audio,32},{out,2048},configured,beep,timer,tx,mo,co,div);
   base.execute({audio,32},{original,2048},configured,beep,timer,tx,mb,cb,div);
   assert(mo.value==mb.value && obs.power_acc==base.power_acc && co==cb);
   if(divider==192){assert(!std::memcmp(out,original,sizeof(out)));continue;}
   for(int n=0;n<16;n++){
    float i,q;reference_hilbert.execute((audio[2*n]>>2)/32768.f,i,q);
    dsp::interpolate::ComplexFIRInterpolate8::Output reconstructed;
    reference_fir.execute(i,q,reconstructed);
    for(int p=0;p<8;p++){
     float si=reconstructed[p].i*256, sq=reconstructed[p].q*256;
     std::complex<int8_t> c=mode==Mode::USB?std::complex<int8_t>{int8_t(si),int8_t(sq)}:std::complex<int8_t>{int8_t(sq),int8_t(si)};
     for(int r=0;r<16;r++)expected[128*n+16*p+r]=c;
    }
   }
   assert(!std::memcmp(out,expected,sizeof(out)));
  }
 }
 std::cout<<"PASS actual SSB: 3k FIR+hold16, USB/LSB mapping, 100 block boundaries, unchanged VU, bit-identical legacy 2k\n";
}
'''

