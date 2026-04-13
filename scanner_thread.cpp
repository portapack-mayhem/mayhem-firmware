#include "scanner_thread.hpp"
#include "portapack.hpp"

namespace drone_analyzer {

ScannerThread::ScannerThread(DroneScanner& scanner) noexcept
    : scanner_(scanner), thread_(nullptr) {
}

ScannerThread::~ScannerThread() noexcept {
    stop();
}

msg_t ScannerThread::static_fn(void* arg) {
    static_cast<ScannerThread*>(arg)->run();
    return 0;
}

void ScannerThread::run() noexcept {
    static RetuneMessage message{};

    while (!chThdShouldTerminate()) {
        if (__atomic_load_n(&scanning_, __ATOMIC_ACQUIRE)) {
            ErrorResult<FreqHz> freq_before = scanner_.get_current_frequency();

            ErrorCode err = scanner_.perform_scan_cycle();
            if (err == ErrorCode::SUCCESS && freq_before.has_value()) {
                message.freq = static_cast<int64_t>(freq_before.value());
                message.range = 0;
                EventDispatcher::send_message(message);
            }

            // Sleep during dwell — use scan_interval_ms from settings (default 100ms)
            // Normal scanning: no sleep, runs at full hardware speed
            if (scanner_.is_dwelling()) {
                const ScanConfig cfg = scanner_.get_config();
                const uint32_t dwell_sleep = (cfg.scan_interval_ms > 0) ? cfg.scan_interval_ms : SCANNER_SLEEP_MS;
                chThdSleepMilliseconds(dwell_sleep);
            }
        }
        chThdSleepMilliseconds(1);  // Yield to RTOS when not scanning
    }
}

void ScannerThread::start() noexcept {
    if (thread_ == nullptr) {
        thread_ = chThdCreateStatic(
            wa_, sizeof(wa_), NORMALPRIO + 10,
            ScannerThread::static_fn, this);
    }
}

void ScannerThread::stop() noexcept {
    if (thread_ != nullptr) {
        __atomic_store_n(&scanning_, false, __ATOMIC_RELEASE);
        chThdTerminate(thread_);
        chThdWait(thread_);
        thread_ = nullptr;
    }
}

void ScannerThread::set_scanning(bool scanning) noexcept {
    __atomic_store_n(&scanning_, scanning, __ATOMIC_RELEASE);
}

bool ScannerThread::is_scanning() const noexcept {
    return __atomic_load_n(&scanning_, __ATOMIC_ACQUIRE);
}

bool ScannerThread::is_active() const noexcept {
    return thread_ != nullptr;
}

}  // namespace drone_analyzer
