"""Exercise the actual AudioTXConfig handler with host-only allocation hooks.

This is sequential ownership testing, not a concurrency/race test.
"""
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
BASE = '510d579d7405ca899963691513c46de74943e9c0'

def handler(text):
    return text[text.index('void MicTXProcessor::on_message('):text.index('\nint main()')]

def lock_support(source):
    return source[source.index('namespace {'):source.index('void MicTXProcessor::execute(')]

def harness(legacy=False):
    source = (ROOT/'firmware/baseband/proc_mictx.cpp').read_text()
    if legacy:
        source = subprocess.check_output(['git','show',BASE+':firmware/baseband/proc_mictx.cpp'],cwd=ROOT,text=True)
    messages = (ROOT/'firmware/common/message.hpp').read_text()
    config = messages[messages.index('class AudioTXConfigMessage'):messages.index('class SigGenConfigMessage')]
    code = r'''
#include <cassert>
#include <cstdint>
#include <cstddef>
#include <new>
#include <set>
struct Message {enum class ID{AudioTXConfig,RequestSignal};ID id;constexpr Message(ID v):id(v){}};
struct RequestSignalMessage:Message {enum class Signal{RogerBeepRequest};Signal signal;};
'''+config+r'''
struct Mutex {bool held=false;};
void chMtxLock(Mutex* m){assert(!m->held);m->held=true;}
Mutex* locked_mutex=nullptr;
void chMtxUnlock(){assert(locked_mutex && locked_mutex->held);locked_mutex->held=false;}
''' + ('' if legacy else lock_support(source).replace('chMtxLock(&mutex);','chMtxLock(&mutex); locked_mutex = &mutex;')) + r'''
namespace dsp { namespace modulate {
enum class Mode{FM,AM,DSB,USB,LSB};
struct Modulator {
 inline static int allocations=0,frees=0,constructed=0,destroyed=0,fm=0,ssb=0,am=0;
 inline static std::set<Modulator*> live;
 Mode mode;uint32_t over=0,delta=0,tone=0;float mix=0,bw=0;
 static void* operator new(size_t n){++allocations;return ::operator new(n);}
 static void operator delete(void* p){++frees;::operator delete(p);}
 Modulator(){++constructed;live.insert(this);}
 virtual ~Modulator(){++destroyed;assert(live.erase(this)==1);}
 void set_mode(Mode v){mode=v;}void set_over(uint32_t v){over=v;}
};
struct FM:Modulator {FM(){++fm;mode=Mode::FM;}void set_fm_delta(uint32_t v){delta=v;}void set_tone_gen_configure(uint32_t d,float w){tone=d;mix=w;}};
struct SSB:Modulator {SSB(){++ssb;mode=Mode::LSB;}void set_fs_div_factor(float v){bw=v;}};
struct AM:Modulator {AM(){++am;mode=Mode::AM;}};
}}
struct MicTXProcessor {
 struct {Mutex mutex;} state_mutex;
 static constexpr uint32_t baseband_fs=1536000;
 dsp::modulate::Modulator* modulator=nullptr;
 bool fm_enabled=true,am_enabled=false,dsb_enabled=false,usb_enabled=false,lsb_enabled=false;
 float audio_gain=0;uint8_t audio_shift_bits_s16=0;uint32_t divider=0,power_acc_count=0,beep_index=0,beep_timer=0;
 bool play_beep=false,configured=false;struct{bool done=false;}txprogress_message;
 void on_message(const Message*);
};
'''+handler(source)+r'''
using M=dsp::modulate::Modulator;using Mode=dsp::modulate::Mode;
AudioTXConfigMessage config(int mode, bool tx){
 return {76800,tx?(mode==0?75000.f:mode==1?5000.f:3000.f):0.f,1.5f,6,8,12345,.25f,
         mode==2,mode==3,mode==4,mode==5};
}
'''
    if legacy:
        return code+r'''
int main(){
 MicTXProcessor p;auto msg=config(4,true);
 p.on_message(&msg);assert(p.modulator->mode==Mode::FM); // wrong initial USB selection
 p.on_message(&msg);assert(p.modulator->mode==Mode::USB);
 assert(M::fm==2 && M::ssb==1 && M::allocations==3 && M::frees==1 && M::live.size()==2);
 // Host-only cleanup of the demonstrated orphan, never added to firmware.
 while(!M::live.empty())delete *M::live.begin();
}
'''
    return code+r'''
int main(){
 MicTXProcessor p;int calls=0;
 // Six GUI modes, both RF-enabled and idle configs, repeated VOX cycles.
 for(int cycle=0;cycle<10000;cycle++)for(int mode=0;mode<6;mode++)for(bool tx:{false,true}){
  auto msg=config(mode,tx);const int fm=M::fm,ssb=M::ssb,am=M::am;
  p.play_beep=true;p.power_acc_count=91;p.on_message(&msg);++calls;
  const Mode wanted=mode<2?Mode::FM:mode==2?Mode::AM:mode==3?Mode::DSB:mode==4?Mode::USB:Mode::LSB;
  assert(p.modulator->mode==wanted && p.modulator->over==64);
  assert(M::allocations==calls && M::constructed==calls);
  assert(M::destroyed==calls-1 && M::frees==calls-1 && M::live.size()==1);
  assert(M::fm-fm==(mode<2) && M::ssb-ssb==(mode>=4) && M::am-am==(mode==2||mode==3));
  if(mode<2){assert(p.modulator->delta==uint32_t(msg.deviation_hz*(0xFFFFFFUL/1536000)));assert(p.modulator->tone==msg.tone_key_delta && p.modulator->mix==msg.tone_key_mix_weight);}
  if(mode>=4)assert(p.modulator->bw==msg.deviation_hz);
  assert(p.audio_gain==msg.audio_gain && p.audio_shift_bits_s16==msg.audio_shift_bits_s16 && p.divider==msg.divider);
  assert(!p.play_beep && p.configured && p.txprogress_message.done && p.power_acc_count==0);
 }
 // Even malformed multi-flag requests allocate one object. Valid GUI requests
 // are one-hot or all-clear; defensive precedence is USB, LSB, AM, DSB.
 for(unsigned bits=0;bits<16;bits++){
  AudioTXConfigMessage msg{76800,3000,1,6,8,0,0,bool(bits&1),bool(bits&2),bool(bits&4),bool(bits&8)};
  p.on_message(&msg);++calls;
  Mode wanted=bits&4?Mode::USB:bits&8?Mode::LSB:bits&1?Mode::AM:bits&2?Mode::DSB:Mode::FM;
  assert(p.modulator->mode==wanted && M::live.size()==1 && M::allocations==calls && M::frees==calls-1);
 }
 delete p.modulator;
 assert(M::live.empty() && M::allocations==M::frees && M::constructed==M::destroyed);
 // Correct first configuration on a fresh processor, including USB/LSB.
 for(int mode=0;mode<6;mode++){
  MicTXProcessor fresh;auto msg=config(mode,true);const int allocations=M::allocations;
  fresh.on_message(&msg);
  Mode wanted=mode<2?Mode::FM:mode==2?Mode::AM:mode==3?Mode::DSB:mode==4?Mode::USB:Mode::LSB;
  assert(fresh.modulator->mode==wanted && M::allocations==allocations+1 && M::live.size()==1);
  delete fresh.modulator;
 }
 assert(M::live.empty() && M::allocations==M::frees && M::constructed==M::destroyed);
}
'''

class ModulatorConfigTests(unittest.TestCase):
    def run_cpp(self, legacy):
        with tempfile.TemporaryDirectory() as tmp:
            p=Path(tmp);(p/'test.cpp').write_text(harness(legacy))
            subprocess.run(['g++','-std=c++17','-O2','-fsanitize=undefined','-fno-sanitize-recover=all',str(p/'test.cpp'),'-o',str(p/'test')],check=True)
            subprocess.run([str(p/'test')],check=True)
    def test_current_selection_and_120016_replacements(self):self.run_cpp(False)
    def test_baseline_reproduces_stale_mode_and_fm_leak(self):self.run_cpp(True)

if __name__=='__main__':unittest.main()
