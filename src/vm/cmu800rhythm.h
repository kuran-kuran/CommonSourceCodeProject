#pragma once

#include <stddef.h>
#include <stdint.h>

class Cmu800Rhythm {
public:
    Cmu800Rhythm(const int8_t* sample_data = NULL,
                 uint32_t sample_count = 0);

    void set_sample(const int8_t* sample_data, uint32_t sample_count);
    void initialize();
    void set_sample_rate(uint32_t sample_rate);
    void trigger();
    void stop();
    int32_t get_data(int32_t volume_q15);

private:
    const int8_t* sample_data;
    uint32_t sample_count;
    uint64_t position_q32;
    uint64_t step_q32;
    uint32_t sample_rate;
    bool playing;
};
