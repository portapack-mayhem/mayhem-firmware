/**
 * @brief Thread synchronization primitives for ChibiOS RTOS
 * @note RAII mutex wrappers and atomic flag for thread-safe state management
 */
#ifndef LOCKING_HPP
#define LOCKING_HPP

#include <cstdint>
#include <cstddef>

#include "ch.h"

namespace drone_analyzer {

/**
 * @brief Lock ordering for deadlock prevention
 * @note MUST be acquired in ASCENDING order only:
 *       0 → 1 → 2 → 3 (PATTERN → DATA → DATABASE → STATE)
 * @warning Acquiring in descending order will deadlock if another
 *          thread holds the lower-order lock. This is compile-time
 *          tagged but NOT enforced — reviewers must verify.
 */
enum class LockOrder : uint8_t {
    PATTERN_MUTEX = 0,
    DATA_MUTEX = 1,
    DATABASE_MUTEX = 2,
    STATE_MUTEX = 3,
};

class AtomicFlag {
public:
    constexpr AtomicFlag() noexcept
        : flag_(0) {}

    bool set() noexcept {
        return __atomic_test_and_set(&flag_, __ATOMIC_SEQ_CST) != 0;
    }

    void clear() noexcept {
        __atomic_clear(&flag_, __ATOMIC_SEQ_CST);
    }

    [[nodiscard]] bool test() const noexcept {
        uint8_t f = 0;
        __atomic_load(&flag_, &f, __ATOMIC_SEQ_CST);
        return f != 0;
    }

    [[nodiscard]] bool test_and_set() noexcept {
        return set();
    }

    [[nodiscard]] bool try_set() noexcept {
        return !set();
    }

    [[nodiscard]] uint8_t get() const noexcept {
        return flag_;
    }

    AtomicFlag(const AtomicFlag&) = delete;
    AtomicFlag& operator=(const AtomicFlag&) = delete;
    AtomicFlag(AtomicFlag&&) = delete;
    AtomicFlag& operator=(AtomicFlag&&) = delete;

private:
    uint8_t flag_;
};

template<LockOrder ORDER>
class MutexLock {
public:
    explicit MutexLock(Mutex& mutex) noexcept
        : mutex_(mutex) {
        chMtxLock(&mutex_);
    }

    ~MutexLock() noexcept {
        chMtxUnlock();
    }

    MutexLock(const MutexLock&) = delete;
    MutexLock& operator=(const MutexLock&) = delete;
    MutexLock(MutexLock&&) = delete;
    MutexLock& operator=(MutexLock&&) = delete;

private:
    Mutex& mutex_;
};

template<LockOrder ORDER>
class MutexTryLock {
public:
    explicit MutexTryLock(Mutex& mutex) noexcept
        : mutex_(mutex), locked_(chMtxTryLock(&mutex_)) {}

    ~MutexTryLock() noexcept {
        if (locked_) {
            chMtxUnlock();
        }
    }

    [[nodiscard]] bool is_locked() const noexcept {
        return locked_;
    }

    MutexTryLock(const MutexTryLock&) = delete;
    MutexTryLock& operator=(const MutexTryLock&) = delete;
    MutexTryLock(MutexTryLock&&) = delete;
    MutexTryLock& operator=(MutexTryLock&&) = delete;

private:
    Mutex& mutex_;
    bool locked_;
};

} // namespace drone_analyzer

#endif // LOCKING_HPP
