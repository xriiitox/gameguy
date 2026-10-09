#include "apu.h"
#include <SDL3/SDL_audio.h>
#include <iostream>

namespace {
constexpr uint8_t duty_patterns[4][8] = {
    {0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 1, 1, 1},
    {0, 1, 1, 1, 1, 1, 1, 0}
};

int16_t clamp_sample(int value) {
    if (value > 32767) return 32767;
    if (value < -32768) return -32768;
    return static_cast<int16_t>(value);
}
}

void SquareChannel::tick_frequency() {
    if (!enabled) {
        return;
    }

    if (--frequency_timer <= 0) {
        frequency_timer = (2048 - frequency) * 4;
        duty_position = (duty_position + 1) & 7;
    }
}

void SquareChannel::clock_length() {
    if (!length_enabled || length_counter == 0) {
        return;
    }

    --length_counter;

    if (length_counter == 0) {
        enabled = false;
        std::cerr << "channel expired by length\n";
    }
}


void SquareChannel::clock_envelope() {
    if (envelope_period == 0) {
        return;
    }

    if (envelope_timer > 0 && --envelope_timer == 0) {
        envelope_timer = envelope_period;

        if (envelope_increase && volume < 15) {
            ++volume;
        } else if (!envelope_increase && volume > 0) {
            --volume;
        }

        std::cerr << "envelope volume=" << static_cast<int>(volume)
                  << " length=" << static_cast<int>(length_counter)
                  << " enabled=" << enabled << '\n';
    }
}

void SquareChannel::trigger(bool master_enabled, int value) {
    if (length_counter == 0) {
        length_counter = 64;
    }

    volume = (value >> 4) & 0x0F;
    enabled = master_enabled && volume != 0;
    duty_position = 0;
    frequency_timer = (2048 - frequency) * 4;
    envelope_timer = envelope_period == 0 ? 8 : envelope_period;
}

uint8_t SquareChannel::output() const {
    if (!enabled) {
        return 0;
    }

    return duty_patterns[duty][duty_position]
           ? volume
           : 0;
}

APU::APU() {
    SDL_AudioSpec spec{};
    spec.format = SDL_AUDIO_S16;
    spec.channels = 2;
    spec.freq = 48000;

    out = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
        &spec, nullptr, nullptr);

    if (!out) {
        SDL_Log("Could not open audio: %s", SDL_GetError());
        return;
    }

    SDL_ResumeAudioStreamDevice(out);
}

APU::~APU() {
    if (out) {
        SDL_DestroyAudioStream(out);
    }
}

void APU::tick() {
    for (int i = 0; i < 4; i++) {
        sample_accumulator += sample_rate;
        if (sample_accumulator >= cpu_clock) {
            sample_accumulator -= cpu_clock;
            generate_sample();
        }
        channel1.tick_frequency();
        channel2.tick_frequency();
        channel3.tick_frequency();
        channel4.tick_frequency();

        ++frame_sequencer_counter;

        if (frame_sequencer_counter >= 8192) {
            frame_sequencer_counter -= 8192;
            frame_sequencer_step = (frame_sequencer_step + 1) & 7;

            if ((frame_sequencer_step & 1) == 0) {
                channel1.clock_length();
                channel2.clock_length();
                channel3.clock_length();
                channel4.clock_length();
            }

            if (frame_sequencer_step == 7) {
                channel1.clock_envelope();
                channel2.clock_envelope();
                channel4.clock_envelope();
            }

            if (frame_sequencer_step == 2 || frame_sequencer_step == 6) {
                clock_sweep();
            }
        }
    }
}

bool APU::next_step_clocks_length() const {
    const uint8_t next_step = (frame_sequencer_step + 1) & 7;
    return (next_step & 1) == 0;
}

void APU::generate_sample() {
    int left = 0;
    int right = 0;
    if (master_enabled) {
        const int channel1_sample = channel1.output();
        const int channel2_sample = channel2.output();
        const int channel3_sample = channel3.output();
        const int channel4_sample = channel4.output();
        if (nr51 & 0x10) left += channel1_sample;
        if (nr51 & 0x20) left += channel2_sample;
        if (nr51 & 0x01) right += channel1_sample;
        if (nr51 & 0x02) right += channel2_sample;
        if (nr51 & 0x40) left += channel3_sample;
        if (nr51 & 0x04) right += channel3_sample;
        if (nr51 & 0x80) left += channel4_sample;
        if (nr51 & 0x08) right += channel4_sample;

        const int left_volume = ((nr50 >> 4) & 0x07) + 1;
        const int right_volume = (nr50 & 0x07) + 1;

        left = left * left_volume / 8;
        right = right * right_volume / 8;
    }

    constexpr int output_gain = 512;
    const int left_sample = left * output_gain;
    const int right_sample = right * output_gain;
    audio_buffer[audio_buffer_frames * channels] =
        clamp_sample(left_sample);
    audio_buffer[audio_buffer_frames * channels + 1] =
        clamp_sample(right_sample);
    ++audio_buffer_frames;

    if (audio_buffer_frames == audio_chunk_frames) {
        if (out) {
            const int queued_bytes = SDL_GetAudioStreamQueued(out);
            const int queued_frames = queued_bytes >= 0
                ? queued_bytes / (sizeof(int16_t) * channels)
                : max_frames;

            if (queued_frames > max_frames) {
                SDL_ClearAudioStream(out);
            }

            if (!SDL_PutAudioStreamData(
                out,
                audio_buffer.data(),
                static_cast<int>(
                    audio_buffer_frames * channels * sizeof(int16_t)
                )
            )) {
                SDL_Log("Could not submit audio: %s", SDL_GetError());
            }
        }

        audio_buffer_frames = 0;
    }
}

void APU::write(uint16_t address, uint8_t value) {
    switch (address) {
        case 0xFF10:
            nr10 = value;
            // TODO
            break;
        case 0xFF11:
            nr11 = value;
            channel1.duty = (value >> 6) & 3;
            channel1.length_counter = 64 - (value & 0x3F);
            break;
        case 0xFF12:
            nr12 = value;
            channel1.envelope_increase = (value & 0x08) != 0;
            channel1.envelope_period = value & 0x07;
            break;
        case 0xFF13:
            nr13 = value;
            channel1.frequency = (channel1.frequency & 0x0700) | value;
            break;
        case 0xFF14:
            nr14 = value;
            channel1.frequency = (channel1.frequency & 0x00FF) |
                                 ((value & 0x07) << 8);
            {
                const bool was_length_enabled = channel1.length_enabled;
                const bool enabling_length = !was_length_enabled &&
                                             (value & 0x40) != 0;
            channel1.length_enabled = (value & 0x40) != 0;
                if (enabling_length && !next_step_clocks_length()) {
                    channel1.clock_length();
                }
            }
            if (value & 0x80) {
                const bool length_was_zero = channel1.length_counter == 0;
                channel1.trigger(master_enabled, nr12);
                if (length_was_zero && channel1.length_enabled &&
                    next_step_clocks_length()) {
                    channel1.clock_length();
                }

                shadow_frequency = channel1.frequency;
                sweep_period = (nr10 >> 4) & 0x07;
                sweep_decrease = (nr10 & 0x08) != 0;
                sweep_shift = nr10 & 0x07;
                sweep_timer = sweep_period == 0 ? 8 : sweep_period;
                sweep_enabled = sweep_period != 0 || sweep_shift != 0;
            }
            break;
        case 0xFF24:
            nr50 = value;
            break;
        case 0xFF25:
            nr51 = value;
            break;
        case 0xFF26:
            master_enabled = (value & 0x80) != 0;
            nr52 = master_enabled ? 0x80 : 0x00;
            if (!master_enabled) {
                channel1.enabled = false;
                channel2.enabled = false;
                channel3.enabled = false;
                channel4.enabled = false;
                left_filter.reset();
                right_filter.reset();
            }
            break;
        case 0xFF16:
            nr21 = value;
            channel2.duty = (value >> 6) & 0x03;
            channel2.length_counter = 64 - (value & 0x3F);
            break;
        case 0xFF17:
            nr22 = value;
            channel2.envelope_increase = (value & 0x08) != 0;
            channel2.envelope_period = value & 0x07;
            break;
        case 0xFF18:
            nr23 = value;
            channel2.frequency = (channel2.frequency & 0x0700) | value;
            break;
        case 0xFF19:
            nr24 = value;
            channel2.frequency = (channel2.frequency & 0x00FF) |
                                 ((value & 0x07) << 8);
            {
                const bool was_length_enabled = channel2.length_enabled;
                const bool enabling_length = !was_length_enabled &&
                                             (value & 0x40) != 0;
            channel2.length_enabled = (value & 0x40) != 0;
                if (enabling_length && !next_step_clocks_length()) {
                    channel2.clock_length();
                }
            }
            if (value & 0x80) {
                const bool length_was_zero = channel2.length_counter == 0;
                channel2.trigger(master_enabled, nr22);
                if (length_was_zero && channel2.length_enabled &&
                    next_step_clocks_length()) {
                    channel2.clock_length();
                }
            }
            break;
        case 0xFF1A:
            nr30 = value;
            channel3.dac_enabled = (value & 0x80) != 0;

            if (!channel3.dac_enabled) {
                channel3.enabled = false;
            }
            break;
        case 0xFF1B:
            nr31 = value;
            channel3.length_counter = 256 - value;
            break;
        case 0xFF1C:
            nr32 = value;
            channel3.volume_code = (value >> 5) & 0x03;
            break;
        case 0xFF1D:
            nr33 = value;
            channel3.frequency = (channel3.frequency & 0x0700) | value;
            break;
        case 0xFF1E:
            nr34 = value;
            channel3.frequency = (channel3.frequency & 0x00FF) |
                ((value & 0x07) << 8);
            {
                const bool was_length_enabled = channel3.length_enabled;
                const bool enabling_length = !was_length_enabled &&
                                             (value & 0x40) != 0;
            channel3.length_enabled = (value & 0x40) != 0;
                if (enabling_length && !next_step_clocks_length()) {
                    channel3.clock_length();
                }
            }
            if (value & 0x80) {
                const bool length_was_zero = channel3.length_counter == 0;
                channel3.trigger(master_enabled);
                if (length_was_zero && channel3.length_enabled &&
                    next_step_clocks_length()) {
                    channel3.clock_length();
                }
            }
            break;
        case 0xFF20:
            nr41 = value;
            channel4.length_counter = 64 - (value & 0x3F);
            break;
        case 0xFF21:
            nr42 = value;
            channel4.volume = (value >> 4) & 0x0F;
            channel4.envelope_increase = (value & 0x08) != 0;
            channel4.envelope_period = value & 0x07;
            break;
        case 0xFF22:
            nr43 = value;
            channel4.clock_shift = value >> 4;
            channel4.width_mode = (value & 0x08) != 0;
            channel4.divisor_code = value & 0x07;
            break;
        case 0xFF23:
            nr44 = value;
            {
                const bool was_length_enabled = channel4.length_enabled;
                const bool enabling_length = !was_length_enabled &&
                                             (value & 0x40) != 0;
            channel4.length_enabled = (value & 0x40) != 0;
                if (enabling_length && !next_step_clocks_length()) {
                    channel4.clock_length();
                }
            }
            if (value & 0x80) {
                const bool length_was_zero = channel4.length_counter == 0;
                channel4.trigger(master_enabled);
                if (length_was_zero && channel4.length_enabled &&
                    next_step_clocks_length()) {
                    channel4.clock_length();
                }
            }
            break;


    }
}

uint8_t APU::read(uint16_t addr) const {
    switch (addr) {
        case 0xFF10: return nr10 | 0x80;
        case 0xFF11: return nr11;
        case 0xFF12: return nr12;
        case 0xFF13: return nr13;
        case 0xFF14: return nr14 | 0x40;

        case 0xFF16: return nr21;
        case 0xFF17: return nr22;
        case 0xFF18: return nr23;
        case 0xFF19: return nr24;

        case 0xFF1A: return nr30;
        case 0xFF1B: return nr31;
        case 0xFF1C: return nr32 | 0x9F;
        case 0xFF1D: return nr33;
        case 0xFF1E: return nr34;

        case 0xFF20: return nr41 | 0xC0;
        case 0xFF21: return nr42;
        case 0xFF22: return nr43;
        case 0xFF23: return nr44 | 0x3F;

        case 0xFF24: return nr50;
        case 0xFF25: return nr51;
        case 0xFF26: {
            uint8_t status = master_enabled ? 0x80 : 0x00;
            if (channel1.enabled) status |= 0x01;
            if (channel2.enabled) status |= 0x02;
            if (channel3.enabled) status |= 0x04;
            if (channel4.enabled) status |= 0x08;
            return status | 0x70;
        }
        default:     return 0xFF;
    }
}

void APU::clock_sweep() {
    if (!sweep_enabled || sweep_period == 0 || !channel1.enabled) {
        return;
    }

    if (--sweep_timer != 0) {
        return;
    }

    sweep_timer = sweep_period == 0 ? 8 : sweep_period;

    const uint16_t delta = shadow_frequency >> sweep_shift;
    const int next_frequency = sweep_decrease
        ? static_cast<int>(shadow_frequency) - delta
        : static_cast<int>(shadow_frequency) + delta;

    if (next_frequency < 0 || next_frequency > 2047) {
        channel1.enabled = false;
        return;
    }

    if (sweep_shift != 0) {
        shadow_frequency = static_cast<uint16_t>(next_frequency);
        channel1.frequency = shadow_frequency;

        uint16_t second_delta = shadow_frequency >> sweep_shift;
        int overflow_check = sweep_decrease
            ? shadow_frequency - second_delta
            : shadow_frequency + second_delta;

        if (overflow_check < 0 || overflow_check > 2047) {
            channel1.enabled = false;
        }
    }
}

uint8_t WaveChannel::output() const {
    if (!enabled || !dac_enabled) {
        return 0;
    }

    const uint8_t packed = wave_ram[position / 2];
    uint8_t sample = (position & 1)
        ? packed & 0x0F
        : packed >> 4;

    switch (volume_code) {
        case 0: sample = 0; break;
        case 1: break;
        case 2: sample >>= 1; break;
        case 3: sample >>= 2; break;
    }

    return sample;
}


void WaveChannel::trigger(bool master_enabled) {
    if (length_counter == 0) {
        length_counter = 256;
    }

    enabled = master_enabled && dac_enabled;
    position = 0;
    frequency_timer = (2048 - frequency) * 2;
}

void WaveChannel::clock_length() {
    if (!length_enabled || length_counter == 0) {
        return;
    }

    --length_counter;

    if (length_counter == 0) {
        enabled = false;
    }
}

void WaveChannel::tick_frequency() {
    if (!enabled) {
        return;
    }

    if (--frequency_timer <= 0) {
        frequency_timer = (2048 - frequency) * 2;
        position = (position + 1) & 31;
    }
}

uint8_t NoiseChannel::output() const {
    if (!enabled) {
        return 0;
    }
    return (lfsr & 1) ? 0 : volume;
}

void NoiseChannel::trigger(bool master_enabled) {
    if (length_counter == 0) {
        length_counter = 64;
    }

    enabled = master_enabled && volume != 0;
    lfsr = 0x7FFF;
    frequency_timer = divisors[divisor_code] << clock_shift;
    envelope_timer = envelope_period == 0 ? 8 : envelope_period;
}

void NoiseChannel::tick_frequency() {
    if (!enabled) {
        return;
    }

    if (--frequency_timer <= 0) {
        frequency_timer =
            divisors[divisor_code] << clock_shift;

        const uint16_t xor_bit =
            (lfsr & 1) ^ ((lfsr >> 1) & 1);

        lfsr >>= 1;
        lfsr |= xor_bit << 14;

        if (width_mode) {
            lfsr &= ~(1 << 6);
            lfsr |= xor_bit << 6;
        }
    }
}

void NoiseChannel::clock_length() {
    if (!length_enabled || length_counter == 0) {
        return;
    }

    --length_counter;

    if (length_counter == 0) {
        enabled = false;
    }
}

void NoiseChannel::clock_envelope() {
    if (envelope_period == 0) {
        return;
    }

    if (envelope_timer > 0 && --envelope_timer == 0) {
        envelope_timer = envelope_period;

        if (envelope_increase && volume < 15) {
            ++volume;
        } else if (!envelope_increase && volume > 0) {
            --volume;
        }
    }
}
