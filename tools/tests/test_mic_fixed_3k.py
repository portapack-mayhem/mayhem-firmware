"""Actual UI bandwidth methods with host fields/model and unchanged-source guards."""
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[2]
BASE='510d579d7405ca899963691513c46de74943e9c0'
UI='firmware/application/apps/ui_mictx.cpp'
METHODS=('configure_baseband','set_rxbw_defaults')
def method(text,name):
    start=text.index('void MicTXView::'+name+'(')
    end=text.index('\n}\n',start)+3
    return text[start:end]
def frozen_ui(text):
    original=subprocess.check_output(['git','show',BASE+':'+UI],cwd=ROOT,text=True)
    for name in METHODS:text=text.replace(method(text,name),method(original,name),1)
    return text

class Fixed3kTests(unittest.TestCase):
    def test_only_two_ui_methods_changed(self):
        original=subprocess.check_output(['git','show',BASE+':'+UI],cwd=ROOT,text=True)
        self.assertEqual(frozen_ui((ROOT/UI).read_text()),original)

    def test_actual_ui_saved_settings_transitions_and_messages(self):
        source=(ROOT/UI).read_text()
        code=r'''
#include <cassert>
#include <algorithm>
#include <functional>
#include <cstdint>
struct Field {
 int v=0,lo=0,hi=150,step=1,calls=0;
 std::function<void(int)> on_change;
 void set_value(int n,bool trigger=true){n=std::clamp(n,lo,hi);if(n!=v){v=n;if(trigger&&on_change)on_change(v);}}
 void set_range(int a,int b){lo=a;hi=b;set_value(v,false);}
 void set_step(int n){step=n;}
 void set_by_value(int n){v=n;++calls;}
};
struct Model {uint32_t bw=2000;uint32_t channel_bandwidth(){return bw;}void set_channel_bandwidth(uint32_t n){bw=n;}} transmitter_model;
struct Config{uint32_t divider;float deviation,gain;uint8_t shift;bool am,dsb,usb,lsb;} last;
namespace baseband {
 void set_audiotx_config(uint32_t divider,float dev,float gain,uint8_t shift,uint8_t,uint32_t,bool am,bool dsb,bool usb,bool lsb){last={divider,dev,gain,shift,am,dsb,usb,lsb};}
}
uint32_t tone_key_frequency(int){return 0;}
uint32_t TONES_F2D(uint32_t,uint32_t){return 0;}
struct MicTXView{
 enum {MIC_MOD_NFM=0,MIC_MOD_WFM=1,MIC_MOD_AM=2,MIC_MOD_USB=3,MIC_MOD_LSB=4,MIC_MOD_DSB=5};
 static constexpr uint32_t sampling_rate=1536000;
 int mic_mod_index=0,rxbw_index=7,mic_gain_x10=15,tone_key_index=0;
 bool transmitting=false;
 Field field_bw,field_rxbw;
 MicTXView(){field_bw.on_change=[](int n){transmitter_model.set_channel_bandwidth(n*1000);};}
 uint8_t shift_bits(){return 6;}
 void configure_baseband();void set_rxbw_defaults(bool);
};
'''+''.join(method(source,n) for n in METHODS)+r'''
int main(){
 for(int ssb:{3,4})for(bool saved:{false,true})for(uint32_t bw:{2000,3000,75000}){
  MicTXView p;p.mic_mod_index=ssb;transmitter_model.bw=bw;p.field_bw.set_value(bw/1000,false);
  p.set_rxbw_defaults(saved);
  assert(p.field_bw.lo==3&&p.field_bw.hi==3&&p.field_bw.v==3&&p.field_bw.step==1&&transmitter_model.bw==3000);
  assert(p.field_rxbw.calls==int(saved));if(saved)assert(p.field_rxbw.v==7);
  p.field_bw.set_value(2);assert(p.field_bw.v==3&&transmitter_model.bw==3000);
  // The sender independently enforces SSB even if model state is stale.
  transmitter_model.bw=2000;p.transmitting=true;p.configure_baseband();
  assert(last.deviation==3000&&last.usb==(ssb==3)&&last.lsb==(ssb==4));
  assert(last.divider==76800&&last.gain==1.5f&&last.shift==6);
  p.transmitting=false;p.configure_baseband();assert(last.deviation==0);
  for(int mode:{0,1,2,5}){
   p.mic_mod_index=ssb;p.set_rxbw_defaults(false); // re-enter fixed range
   p.mic_mod_index=mode;p.set_rxbw_defaults(false);
   int wanted=mode==0?10:mode==1?75:3;
   assert(p.field_bw.lo==(mode==2||mode==5?0:1));
   assert(p.field_bw.hi==(mode==0?60:150));
   assert(p.field_bw.v==wanted&&transmitter_model.bw==uint32_t(wanted*1000));
   p.transmitting=true;p.configure_baseband();assert(last.deviation==wanted*1000);
   p.transmitting=false;p.configure_baseband();assert(last.deviation==0);
  }
 }
 for(int mode:{0,1,2,5}){
  MicTXView p;p.mic_mod_index=3;p.set_rxbw_defaults(false);
  p.mic_mod_index=mode;transmitter_model.bw=mode==0?12000:mode==1?100000:25000;
  const auto saved=transmitter_model.bw;p.set_rxbw_defaults(true);
  assert(transmitter_model.bw==saved&&p.field_bw.v==int(saved/1000));
  assert(p.field_rxbw.v==7&&p.field_rxbw.calls==1);
 }
}
'''
        with tempfile.TemporaryDirectory() as tmp:
            p=Path(tmp);(p/'test.cpp').write_text(code)
            subprocess.run(['g++','-std=c++17','-O2','-fsanitize=undefined','-fno-sanitize-recover=all',str(p/'test.cpp'),'-o',str(p/'test')],check=True)
            subprocess.run([str(p/'test')],check=True)

if __name__=='__main__':unittest.main()
