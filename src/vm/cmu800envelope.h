#pragma once

#include <cstdint>

class Cmu800Envelope {
public:
    static constexpr std::uint32_t decay_minimum_q31 = 2147349333u;
    static constexpr std::uint32_t decay_middle_q31 = 2147437047u;
    static constexpr std::uint32_t decay_maximum_q31 = 2147442976u;

    Cmu800Envelope();

    void initialize();
    void initialize(std::uint32_t sample_rate);
    void set_sample_rate(std::uint32_t sample_rate);
    void set_decay_factor_q31(std::uint32_t factor_q31);
    void set_decay(std::uint8_t value);
    void enable_sustain(bool enabled);
    void set_sustain(std::uint8_t value);
    void set_gate(bool gate_is_on);
    void trigger();
    void stop();
    bool is_active() const;
    std::int32_t get_volume_q15_and_advance();

private:
    enum class stage_type : std::uint8_t {
        inactive,
        one_shot_decay,
        decay_to_sustain,
        sustain_hold,
        release
    };

    void advance(std::uint32_t factor_q31);
    void advance_one_reference_sample();

    std::uint32_t level_q31;
    std::uint32_t decay_factor_q31;
    std::uint32_t release_factor_q31;
    std::uint32_t sustain_level_q31;
    std::uint32_t sample_rate;
    std::uint64_t reference_clock_accumulator;
    bool sustain_enabled;
    bool gate_is_on;
    stage_type stage;
};
