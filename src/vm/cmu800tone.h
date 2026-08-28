#pragma once

#include <stddef.h>
#include <stdint.h>

#include "Cmu800Envelope.h"

class Cmu800Tone {
public:
    static constexpr size_t wave_table_sample_count = 4096u;
    static constexpr uint32_t default_sample_rate = 48000u;

    enum class wave_type { melody, bass };

    Cmu800Tone();
    void set_wave(wave_type wave);
    void initialize();
    void set_sample_rate(uint32_t sample_rate);
    void set_8253(uint16_t divider);
    void set_decay(uint8_t value);
    void enable_sustain(bool enabled);
    void set_sustain(uint8_t value);
    void set_gate(bool gate_is_on);
    void stop();
    bool is_playing() const;
    int32_t get_data_with_volume(int32_t volume_q15);

    static const int8_t melody_table[wave_table_sample_count];
    static const int8_t bass_table[wave_table_sample_count];

private:
    int32_t get_data(int32_t volume_q15);

    const int8_t* wave_data;
    uint32_t phase;
    uint32_t phase_step;
    uint32_t sample_rate;
    uint16_t divider;
    Cmu800Envelope envelope;
};
