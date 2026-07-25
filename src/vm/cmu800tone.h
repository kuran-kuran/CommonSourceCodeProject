#pragma once

#include <cstdint>

// One CMU-800 style voice using a 4096-sample signed 8-bit wavetable at
// a 48 kHz output rate.  The returned sample is signed and ready for mixing.
class Cmu800Tone {
public:
    // table points to 4096 bytes.  On Pico it can point directly to Flash.
    explicit Cmu800Tone(const std::int8_t* table = nullptr);

    void SetWaveTable(const std::int8_t* table);
    void Initialize();

    // Set a CMU-800 8253 divider.  For example, 0x0B39 is A=442 Hz.
    void Set8253(std::uint16_t divider);

    // Get one 48 kHz sample.  volumeQ15 is 0..32767; 32767 is full level.
    std::int32_t GetData(std::int32_t volumeQ15);

private:
    static std::uint32_t MakePhaseStepFrom8253(std::uint16_t divider);

    const std::int8_t* table_;
    std::uint32_t phase_;
    std::uint32_t phaseStep_;
};
