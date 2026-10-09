"""Actual MicTX handler/execute with host mutex and controlled worker overlap.

Host scheduling is not a model of target priority inheritance or cycle timing.
"""
from pathlib import Path
import subprocess
import tempfile
import unittest
from test_mic_modulator_config import ROOT, harness, lock_support

class LockTests(unittest.TestCase):
    def test_mutex_initialized_before_worker(self):
        header=(ROOT/'firmware/baseband/proc_mictx.hpp').read_text()
        self.assertLess(header.index('chMtxInit(&mutex)'),header.index('} state_mutex{};'))
        self.assertLess(header.index('} state_mutex{};'),header.index('BasebandThread baseband_thread'))

    def test_actual_execute_replacement_and_beep_exclusion(self):
        code=harness(False).split('using M=')[0]
        start=code.index('struct Mutex {');end=code.index('namespace {',start)
        code=code[:start]+r'''
#include <mutex>
#include <condition_variable>
#include <thread>
#include <future>
#include <chrono>
#include <atomic>
using namespace std::chrono_literals;
struct Mutex {std::mutex m;bool initialized=false;};
void chMtxInit(Mutex* m){m->initialized=true;}
thread_local Mutex* locked_mutex=nullptr;
void chMtxLock(Mutex* m){assert(m->initialized);assert(!locked_mutex);m->m.lock();locked_mutex=m;}
void chMtxUnlock(){assert(locked_mutex);auto* m=locked_mutex;locked_mutex=nullptr;m->m.unlock();}
std::mutex gate_mutex;
std::condition_variable gate_cv;
bool entered=false,released=false;
std::atomic<int> processed{0};
struct buffer_c8_t{};struct buffer_s16_t{int16_t* p;size_t count;};
struct AudioLevelReportMessage{};struct TXProgressMessage{bool done=false;};
struct BasebandProcessor{virtual ~BasebandProcessor()=default;virtual void execute(const buffer_c8_t&)=0;virtual void on_message(const Message*)=0;};
namespace baseband{enum class Direction{Transmit};}
struct BasebandThread{
 BasebandThread(size_t,BasebandProcessor* p,baseband::Direction){p->execute(buffer_c8_t{});}
};
struct AudioInput{void read_audio_buffer(buffer_s16_t&){assert(locked_mutex);}};
'''+code[end:]
        code=code.replace(' locked_mutex = &mutex;','')
        code=code.replace('Modulator(){++constructed;', 'Modulator(){assert(locked_mutex);++constructed;')
        code=code.replace('virtual ~Modulator(){++destroyed;', 'virtual ~Modulator(){assert(locked_mutex);++destroyed;')
        code=code.replace('void set_mode(Mode v)',r'''
 void set_gain_shiftbits_vumeter_beep(float,uint8_t,bool){assert(locked_mutex);}
 void execute(buffer_s16_t&,const buffer_c8_t&,bool& configured,uint32_t& index,uint32_t& timer,
              auto&,auto&,uint32_t&,uint32_t&){
  assert(locked_mutex);assert(live.count(this));
  std::unique_lock<std::mutex> gate(gate_mutex);
  entered=true;gate_cv.notify_all();gate_cv.wait(gate,[]{return released;});
  assert(live.count(this));++processed;configured=false;index=9;timer=10;
 }
 void set_mode(Mode v)''')
        start=code.index('struct MicTXProcessor {')
        end=code.index('void MicTXProcessor::on_message(',start)
        header=(ROOT/'firmware/baseband/proc_mictx.hpp').read_text()
        declaration=header[header.index('class MicTXProcessor :'):header.index('\n#endif')]
        declaration=declaration.replace('   private:', '   public:')
        code=code[:start]+declaration+'\n'+code[end:]
        source=(ROOT/'firmware/baseband/proc_mictx.cpp').read_text()
        code+=source[source.index('void MicTXProcessor::execute('):source.index('void MicTXProcessor::on_message(')]
        code+=r'''
int main(){
 MicTXProcessor p;buffer_c8_t buffer;
 p.execute(buffer);assert(!locked_mutex && processed==0); // early return unlocks
 AudioTXConfigMessage cfg{76800,3000,1,6,8,0,0,false,false,true,false};
 p.on_message(&cfg);auto* initial=p.modulator;
 auto worker=std::async(std::launch::async,[&]{p.execute(buffer);assert(!locked_mutex);});
 {std::unique_lock<std::mutex> gate(gate_mutex);gate_cv.wait(gate,[]{return entered;});}
 std::promise<void> started;
 auto replace=std::async(std::launch::async,[&]{started.set_value();p.on_message(&cfg);assert(!locked_mutex);});
 started.get_future().wait();assert(replace.wait_for(30ms)==std::future_status::timeout);
 assert(dsp::modulate::Modulator::live.count(initial));
 {std::lock_guard<std::mutex> gate(gate_mutex);released=true;}gate_cv.notify_all();
 worker.get();replace.get();assert(p.configured && processed==1);
 assert(dsp::modulate::Modulator::live.size()==1);
 {std::lock_guard<std::mutex> gate(gate_mutex);entered=false;released=false;}
 auto worker2=std::async(std::launch::async,[&]{p.execute(buffer);});
 {std::unique_lock<std::mutex> gate(gate_mutex);gate_cv.wait(gate,[]{return entered;});}
 RequestSignalMessage beep{{Message::ID::RequestSignal},RequestSignalMessage::Signal::RogerBeepRequest};
 std::promise<void> beep_started;
 auto request=std::async(std::launch::async,[&]{beep_started.set_value();p.on_message(&beep);assert(!locked_mutex);});
 beep_started.get_future().wait();assert(request.wait_for(30ms)==std::future_status::timeout);
 {std::lock_guard<std::mutex> gate(gate_mutex);released=true;}gate_cv.notify_all();
 worker2.get();request.get();assert(p.play_beep && p.beep_index==0 && p.beep_timer==0);
 {MicTXStateLock lock{p.state_mutex.mutex};delete p.modulator;}
 assert(!locked_mutex && dsp::modulate::Modulator::live.empty());
}
'''
        with tempfile.TemporaryDirectory() as tmp:
            p=Path(tmp);(p/'test.cpp').write_text(code)
            subprocess.run(['g++','-std=c++20','-O2','-pthread','-fsanitize=undefined','-fno-sanitize-recover=all',str(p/'test.cpp'),'-o',str(p/'test')],check=True)
            subprocess.run([str(p/'test')],check=True,timeout=10)

if __name__=='__main__':unittest.main()
