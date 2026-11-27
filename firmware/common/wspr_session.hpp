/*
 * Copyright (C) 2026 Ivan
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

/* WSPR RX capture-session boundary (T05).
 *
 * The M4 producer and the M0 UI do not share a lock. These two small state
 * objects are the whole handshake:
 *
 *  Session (M4): the event thread only RECORDS a request; the capture thread
 *  (the sole producer of the ring) performs take_reset() at the top of a
 *  buffer, resets the ring/filters, then commit_reset() publishes the new
 *  generation and the acknowledged request id. This keeps the matrix reset on
 *  the same thread as feed_sample() and makes stale decode results detectable
 *  by a plain generation comparison.
 *
 *  UiGate (M0): the UI stops accepting packets the moment it requests a
 *  restart and accepts them again only for the status whose restart_ack
 *  exactly matches its request id. Reports already queued from the previous
 *  session are therefore discarded deterministically, without depending on
 *  queue ordering. Once accepted, packets must also match the accepted
 *  generation (defence in depth against an M4-side publish bug).
 *
 * Both objects are header-only and free of ChibiOS so the host tests can drive
 * the exact production logic through controlled interleavings. */
#ifndef __WSPR_SESSION_H__
#define __WSPR_SESSION_H__

#include <stdint.h>

#include <atomic>

namespace wspr {
namespace session {

/* M4 producer-side request/ack/generation state. */
class Session {
   public:
    /* A consistent (generation, ack) pair. The producer commits both under a
     * seqlock, so the decode thread can never pair an OLD generation with a
     * NEWER ack id (which would let the UI accept a stale-session status). */
    struct View {
        uint32_t generation;
        uint16_t ack;
    };

    /* Event thread: post a restart request. Later requests overwrite this one
     * if the producer has not consumed it yet; the ACK then carries the id
     * that was actually pending at reset time. */
    void request(const uint16_t id) {
        requested_id_.store(id, std::memory_order_relaxed);
        pending_.store(true, std::memory_order_release);
    }

    /* Capture thread, at a buffer boundary: claim the pending request. Returns
     * false when there is none. The reset work follows, then commit_reset(). */
    bool take_reset(uint16_t& id) {
        if (!pending_.exchange(false, std::memory_order_acquire)) return false;
        id = requested_id_.load(std::memory_order_acquire);
        return true;
    }

    /* Capture thread, after the in-place reset has completed: publish the new
     * generation and the acknowledged request id. Returns the new generation.
     * Only this thread writes the pair; readers use view(). */
    uint32_t commit_reset(const uint16_t id) {
        const uint32_t s = seq_.load(std::memory_order_relaxed);
        seq_.store(s + 1u, std::memory_order_relaxed);  // odd = update in progress
        std::atomic_thread_fence(std::memory_order_release);
        const uint32_t next = generation_.load(std::memory_order_relaxed) + 1u;
        generation_.store(next, std::memory_order_relaxed);
        ack_id_.store(id, std::memory_order_relaxed);
        seq_.store(s + 2u, std::memory_order_release);  // even = consistent
        return next;
    }

    uint32_t generation() const {
        return generation_.load(std::memory_order_acquire);
    }

    uint16_t ack_id() const {
        return view().ack;
    }

    /* Consistent pair (seqlock read). The retry loop makes progress only while
     * the writer completes a commit; it is NOT an unconditional bounded wait.
     * In this firmware the only writer is the capture thread, which commits at
     * most once per buffer, so a reader that is preempted mid-read can spin
     * until it is scheduled again. With the current priorities (the decode
     * thread runs below baseband) that contention is short, but the loop's
     * progress is a property of scheduling, not a guarantee of this object. */
    View view() const {
        for (;;) {
            const uint32_t s1 = seq_.load(std::memory_order_acquire);
            if (s1 & 1u) continue;
            const uint32_t g = generation_.load(std::memory_order_relaxed);
            const uint16_t a = ack_id_.load(std::memory_order_relaxed);
            std::atomic_thread_fence(std::memory_order_acquire);
            const uint32_t s2 = seq_.load(std::memory_order_relaxed);
            if (s1 == s2) return View{g, a};
        }
    }

   private:
    std::atomic<bool> pending_{false};
    std::atomic<uint16_t> requested_id_{0};
    std::atomic<uint32_t> seq_{0};
    std::atomic<uint32_t> generation_{0};
    std::atomic<uint16_t> ack_id_{0xFFFF};
};

/* M0 UI-side acceptance gate. Not atomic: only the UI thread uses it, driven by
 * messages delivered on that thread. */
class UiGate {
   public:
    /* A restart was requested with this id: stop accepting reports now. The
     * session observed at request time is remembered so a reused request id
     * can never be satisfied by a status from that SAME session. */
    void request(const uint16_t expected_ack) {
        expected_ack_ = expected_ack;
        session_at_request_ = session_id_;
        session_known_at_request_ = session_known_;
        pending_ = true;
        accepting_ = false;
    }

    /* Feed one WSPRRxStatusMessage. Returns true only when this status
     * acknowledged the outstanding request AND reports a different (newer)
     * generation than the one observed when the request was made. */
    bool on_status(const uint32_t session_id, const uint16_t restart_ack) {
        if (pending_) {
            if (restart_ack != expected_ack_) return false;
            if (session_known_at_request_ && session_id == session_at_request_)
                return false;
            pending_ = false;
            accepting_ = true;
            session_id_ = session_id;
            session_known_ = true;
            return true;
        }
        session_id_ = session_id;
        session_known_ = true;
        accepting_ = true;
        return false;
    }

    /* Is a packet from this generation allowed through? Reports from a session
     * whose ACK has not arrived are dropped, and after acceptance a report from
     * any other generation is dropped too. */
    bool packet_allowed(const uint32_t session_id) const {
        if (!accepting_) return false;
        if (session_known_ && session_id != session_id_) return false;
        return true;
    }

    bool pending() const { return pending_; }
    bool accepting() const { return accepting_; }
    bool session_known() const { return session_known_; }
    uint32_t session_id() const { return session_id_; }
    uint16_t expected_ack() const { return expected_ack_; }

   private:
    uint16_t expected_ack_{0};
    bool pending_{false};
    /* Closed until the first status: a packet already queued from a previous
     * app instance must not be accepted under an unknown generation. The M4
     * queues a status before any possible report, so this costs nothing. */
    bool accepting_{false};
    bool session_known_{false};
    uint32_t session_id_{0};
    bool session_known_at_request_{false};
    uint32_t session_at_request_{0};
};

}  // namespace session
}  // namespace wspr

#endif  // __WSPR_SESSION_H__
