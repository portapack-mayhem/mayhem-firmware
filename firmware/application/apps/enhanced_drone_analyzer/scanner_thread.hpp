#ifndef SCANNER_THREAD_HPP
#define SCANNER_THREAD_HPP

#include <cstdint>
#include <cstddef>
#include "ch.h"
#include "scanner.hpp"
#include "message.hpp"
#include "event_m0.hpp"
#include "constants.hpp"

namespace drone_analyzer {

class ScannerThread {
public:
    explicit ScannerThread(DroneScanner& scanner) noexcept;
    ~ScannerThread() noexcept;

    ScannerThread(const ScannerThread&) = delete;
    ScannerThread& operator=(const ScannerThread&) = delete;
    ScannerThread(ScannerThread&&) = delete;
    ScannerThread& operator=(ScannerThread&&) = delete;

    void start() noexcept;
    void stop() noexcept;

    void set_scanning(bool scanning) noexcept;
    [[nodiscard]] bool is_scanning() const noexcept;
    [[nodiscard]] bool is_active() const noexcept;

    /**
     * @brief Reset dwell counter in scanner class
     * @note Called when entering sweep mode to clear stale dwell state
     */
    void reset_dwell() noexcept { scanner_.reset_dwell_cycles(); }

private:
    static constexpr size_t STACK_WORDS = THD_WA_SIZE(SCANNER_THREAD_STACK_SIZE) / sizeof(stkalign_t);

    static msg_t static_fn(void* arg);
    void run() noexcept;

    DroneScanner& scanner_;
    Thread* thread_{nullptr};
    stkalign_t wa_[STACK_WORDS];

    bool scanning_{false};
};

} // namespace drone_analyzer

#endif // SCANNER_THREAD_HPP
