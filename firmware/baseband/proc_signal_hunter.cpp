#include "proc_signal_hunter.hpp"
#include "event_m4.hpp"
#include "portapack_shared_memory.hpp"
#include "dsp_fir_taps.hpp"

void SignalHunterProcessor::configure() {
    decim_0.configure(taps_4k25_decim_0.taps);

    window_idx = 0;
    window_sum = 0;
    for (auto& v : window_buf) v = 0;

    reset_hunt_state();
    configured = true;
}

void SignalHunterProcessor::reset_hunt_state() {
    iq_ring_idx = 0;
    for (auto& v : iq_ring) v = complex16_t{0, 0};
    hangtime_counter = 0;
    hunt_state = HuntState::IDLE;
    flush_pending = false;  // ADD
}

void SignalHunterProcessor::execute(const buffer_c8_t& buffer) {
    if (!hunting) return;

    const auto out = decim_0.execute(buffer, dst_buffer);
    feed_channel_stats(out);

    // Pre-roll flush: prvý execute() po vytvorení streamu
    // Bezpečné tu (nie v on_message) — M0 CaptureThread už beží a konzumuje buffery
    if (flush_pending && stream) {
        // Zapíšeme ring buffer od najstaršej po najnovšiu vzorku (2 chunky kvôli wrap)
        size_t first = IQ_RING_SAMPLES - flush_start_idx;
        stream->write(&iq_ring[flush_start_idx], first * sizeof(complex16_t));
        if (flush_start_idx > 0)
            stream->write(&iq_ring[0], flush_start_idx * sizeof(complex16_t));
        flush_pending = false;
    }

    for (size_t i = 0; i < out.count; i++) {
        auto s = out.p[i];

        iq_ring[iq_ring_idx] = s;
        iq_ring_idx = (iq_ring_idx + 1) % IQ_RING_SAMPLES;

        uint32_t energy = ((int32_t)s.real() * s.real() +
                           (int32_t)s.imag() * s.imag()) >> 16;
        window_sum -= window_buf[window_idx];
        window_buf[window_idx] = energy;
        window_sum += energy;
        window_idx = (window_idx + 1) % WINDOW_SIZE;

        uint32_t avg = window_sum / WINDOW_SIZE;

        switch (hunt_state) {
            case HuntState::IDLE:
                if (avg > energy_threshold) {
                    HunterTriggerMessage msg{};
                    msg.energy = avg;
                    shared_memory.application_queue.push(msg);
                    hunt_state = HuntState::AWAITING_STREAM;
                }
                break;
            case HuntState::AWAITING_STREAM:
                break;
            case HuntState::RECORDING:
                if (avg < energy_threshold) {
                    hangtime_counter = hangtime_samples_limit;
                    hunt_state = HuntState::HANGTIME;
                }
                break;
            case HuntState::HANGTIME:
                if (avg > energy_threshold) {
                    hunt_state = HuntState::RECORDING;
                } else if (--hangtime_counter == 0) {
                    HunterStopMessage stop_msg{};
                    shared_memory.application_queue.push(stop_msg);
                    hunt_state = HuntState::AWAITING_CLOSE;
                }
                break;
            case HuntState::AWAITING_CLOSE:
                // Neprechádzame do IDLE sami — čakáme na CaptureConfigMessage(nullptr)
                // ktorý príde cez BasebandCapture deštruktor po ukončení CaptureThread
                break;
        }
    }

    // KĽÚČOVÝ FIX: píšeme VŽDY keď stream existuje, aj v AWAITING_CLOSE
    // Bez toho CaptureThread uviazne v buffers.get() a chThdWait() zmrazí UI
    if (stream) {
        stream->write(out.p, sizeof(complex16_t) * out.count);
    }
}

void SignalHunterProcessor::on_message(const Message* const message) {
    switch (message->id) {
        case Message::ID::HunterConfig: {
            const auto& m = *reinterpret_cast<const HunterConfigMessage*>(message);
            energy_threshold = m.energy_threshold;

            // PREPOČET: 1 ms = 250 vzoriek (pri 250kHz post-decimation)
            hangtime_samples_limit = m.hangtime_ms * 250;

            if (!configured) configure();

            if (!m.start && hunting) {
                stream.reset();
                reset_hunt_state();
            }
            hunting = m.start;
            break;
        }

        case Message::ID::CaptureConfig: {
            const auto& m = *reinterpret_cast<const CaptureConfigMessage*>(message);
            if (m.config) {
                stream = std::make_unique<StreamInput>(m.config);
                flush_start_idx = iq_ring_idx;  // snapshot — sem sme sa dopísali
                flush_pending = true;           // execute() spracuje ring buffer flush
                hunt_state = HuntState::RECORDING;
            } else {
                stream.reset();
                reset_hunt_state();
            }
            break;
        } 

        default:
            break;
    }
}

int main() {
    EventDispatcher event_dispatcher{std::make_unique<SignalHunterProcessor>()};
    event_dispatcher.run();
    return 0;
}
