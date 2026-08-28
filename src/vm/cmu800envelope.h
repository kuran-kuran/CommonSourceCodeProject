#pragma once

#include <stdint.h>

class Cmu800Envelope {
public:
    // Common measured anchors: Decay 0 = 227 ms, Decay 5 = 1.054 s.
    static constexpr uint32_t decay_minimum_q31 = 2147286568u;
    static constexpr uint32_t decay_middle_q31 = 2147441201u;
    Cmu800Envelope();

    void initialize();
    void set_sample_rate(uint32_t sample_rate);
    void set_decay_factor_q31(uint32_t factor_q31);
    void set_decay(uint8_t value);
    void enable_sustain(bool enabled);
    void set_sustain(uint8_t value);
    void set_gate(bool gate_is_on);
    void trigger();
    void stop();
    bool is_active() const;
    int32_t get_volume_q15_and_advance();

private:
    enum class stage_type : uint8_t {
        inactive,
        one_shot_decay,
        release
    };

    void advance(uint32_t factor_q31);
    void advance_one_reference_sample();

    uint32_t level_q31;
    uint32_t decay_factor_q31;
    uint32_t release_factor_q31;
    uint32_t sample_rate;
    uint64_t reference_clock_accumulator;
    bool sustain_enabled;
    bool gate_is_on;
    stage_type stage;
};
