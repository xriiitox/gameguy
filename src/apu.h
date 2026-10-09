#pragma once
#include <SDL3/SDL.h>
#include <SDL3/SDL_audio.h>
#include <array>

struct SquareChannel {
    bool enabled = false;
    uint8_t duty = 0;
    uint8_t duty_position = 0;
    uint8_t volume = 0;
    uint16_t frequency = 0;
    int frequency_timer = 0;

    uint8_t length_counter = 0;
    bool length_enabled = false;

    uint8_t envelope_period = 0;
    uint8_t envelope_timer = 0;
    bool envelope_increase = false;

    void tick_frequency();
    void clock_length();
    void clock_envelope();
    void trigger(bool master_enabled, int value);
    uint8_t output() const;
};

struct WaveChannel {
    bool enabled = false;
    bool dac_enabled = false;
    uint16_t frequency = 0;
    int frequency_timer = 0;

    uint16_t length_counter = 0;
    bool length_enabled = false;

    uint8_t position = 0;
    uint8_t volume_code = 0;
    std::array<uint8_t, 16> wave_ram{};

    void tick_frequency();
    void clock_length();
    void trigger(bool master_enabled);
    uint8_t output() const;
};

struct NoiseChannel {
    bool enabled = false;
    uint8_t volume = 0;

    uint8_t envelope_period = 0;
    uint8_t envelope_timer = 0;
    bool envelope_increase = false;

    uint8_t length_counter = 0;
    bool length_enabled = false;

    uint16_t lfsr = 0x7FFF;
    uint8_t clock_shift = 0;
    uint8_t divisor_code = 0;
    bool width_mode = false;
    int frequency_timer = 0;

    static constexpr int divisors[8] = {
        8, 16, 32, 48, 64, 80, 96, 112
    };

    void tick_frequency();
    void clock_length();
    void clock_envelope();
    void trigger(bool master_enabled);
    uint8_t output() const;
};

struct HighPass {
    double previous_input = 0.0;
    double previous_output = 0.0;

    int16_t process(int input) {
        constexpr double alpha = 0.995;
        const double output =
            alpha * (previous_output + input - previous_input);
        previous_input = input;
        previous_output = output;

        if (output > 32767.0) return 32767;
        if (output < -32768.0) return -32768;
        return static_cast<int16_t>(output);
    }

    void reset() {
        previous_input = 0.0;
        previous_output = 0.0;
    }
};

class APU {
public:
    static constexpr uint32_t cpu_clock = 4194304;
    static constexpr uint32_t sample_rate = 48000;
    static constexpr int channels = 2;
    static constexpr int target_frames = 512;
    static constexpr int max_frames = 1024;

    static constexpr size_t audio_chunk_frames = 256;
    std::array<int16_t, audio_chunk_frames * 2> audio_buffer{};
    size_t audio_buffer_frames = 0;


    SDL_AudioStream* out = nullptr;
    uint32_t sample_accumulator = 0;
    SquareChannel channel1;
    SquareChannel channel2;
    WaveChannel channel3;
    NoiseChannel channel4;
    HighPass left_filter;
    HighPass right_filter;

    void generate_sample();

    bool master_enabled = true;

    uint16_t shadow_frequency = 0;
    uint8_t sweep_timer = 0;
    uint8_t sweep_period = 0;
    uint8_t sweep_shift = 0;
    bool sweep_decrease = false;
    bool sweep_enabled = false;

    uint8_t nr10 = 0x80;
    uint8_t nr11 = 0xBF;
    uint8_t nr12 = 0xF3;
    uint8_t nr13 = 0xFF;
    uint8_t nr14 = 0xBF;
    uint8_t nr21 = 0x3F;
    uint8_t nr22 = 0;
    uint8_t nr23 = 0xFF;
    uint8_t nr24 = 0xBF;
    uint8_t nr30 = 0x7F;
    uint8_t nr31 = 0xFF;
    uint8_t nr32 = 0x9F;
    uint8_t nr33 = 0xFF;
    uint8_t nr34 = 0xBF;
    uint8_t nr41 = 0xFF;
    uint8_t nr42 = 0;
    uint8_t nr43 = 0;
    uint8_t nr44 = 0xBF;

    uint8_t nr50 = 0x77;
    uint8_t nr51 = 0xF3;
    uint8_t nr52 = 0xF1;

    APU();
    ~APU();
    void tick();
    void write(uint16_t addr, uint8_t val);
    uint8_t read(uint16_t addr) const;
    void clock_sweep();
    bool next_step_clocks_length() const;

    uint32_t frame_sequencer_counter = 0;
    uint8_t frame_sequencer_step = 0;
};
