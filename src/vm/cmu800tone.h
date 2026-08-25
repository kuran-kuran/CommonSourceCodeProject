#pragma once

#include <cstddef>
#include <cstdint>

#include "Cmu800Envelope.h"

class Cmu800Tone {
public:
    static constexpr std::size_t wave_table_sample_count = 4096u;
    static constexpr std::uint32_t default_sample_rate = 48000u;

    enum class wave_type { melody, bass };

    Cmu800Tone();
    explicit Cmu800Tone(wave_type wave);
    void set_wave(wave_type wave);
    void initialize();
    void initialize(std::uint32_t sample_rate);
    void set_sample_rate(std::uint32_t sample_rate);
    void set_8253(std::uint16_t divider);
    void set_decay(std::uint8_t value);
    void set_decay_factor_q31(std::uint32_t factor_q31);
    void enable_sustain(bool enabled);
    void set_sustain(std::uint8_t value);
    void set_gate(bool gate_is_on);
    void stop();
    bool is_playing() const;
    std::int32_t get_data();
    std::int32_t get_data_with_volume(std::int32_t volume_q15);
    std::int32_t get_data(std::int32_t volume_q15);

    static const std::int8_t melody_table[wave_table_sample_count];
    static const std::int8_t bass_table[wave_table_sample_count];

private:
    const std::int8_t* wave_data;
    std::uint32_t phase;
    std::uint32_t phase_step;
    std::uint32_t sample_rate;
    std::uint16_t divider;
    Cmu800Envelope envelope;
};
