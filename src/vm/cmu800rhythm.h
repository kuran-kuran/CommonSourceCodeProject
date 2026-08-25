#pragma once

#include <cstddef>
#include <cstdint>

class Cmu800Rhythm {
public:
    Cmu800Rhythm(const std::int8_t* sample_data = NULL,
                 std::uint32_t sample_count = 0);

    void set_sample(const std::int8_t* sample_data, std::uint32_t sample_count);
    void initialize();
    void initialize(std::uint32_t sample_rate);
    void set_sample_rate(std::uint32_t sample_rate);
    void trigger();
    void stop();
    bool is_playing() const;
    std::int32_t get_data(std::int32_t volume_q15);

private:
    const std::int8_t* sample_data;
    std::uint32_t sample_count;
    std::uint64_t position_q32;
    std::uint64_t step_q32;
    std::uint32_t sample_rate;
    bool playing;
};
