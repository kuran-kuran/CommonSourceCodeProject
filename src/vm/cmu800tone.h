#pragma once

#include <cstddef>
#include <cstdint>

#include "Cmu800Envelope.h"

// Pico-only, no-envelope wavetable oscillator.  Both tables live in Flash.
class Cmu800Tone {
public:
    static constexpr std::size_t kWaveTableSampleCount = 4096u;
    static constexpr std::uint32_t kSampleRate = 48000u;

    enum class Wave { Melody, Bass };

    Cmu800Tone();
    explicit Cmu800Tone(Wave wave);
    void SetWave(Wave wave);
    void Initialize();
    void Initialize(std::uint32_t sampleRate);
    void SetSampleRate(std::uint32_t sampleRate);
    void Set8253(std::uint16_t divider);

    // Same envelope API as Cmu800Tone.  Sustain is intended for Melody only.
    void SetDecay(std::uint8_t value);
    void SetDecayFactorQ31(std::uint32_t factorQ31);
    void EnableSustain(bool enabled);
    void SetSustain(std::uint8_t value);
    void SetGate(bool gateIsOn);
    void Stop();
    bool IsPlaying() const;

    std::int32_t GetData();
    std::int32_t GetDataWithVolume(std::int32_t volumeQ15);
    std::int32_t GetData(std::int32_t volumeQ15);

    static const std::int8_t kMelodyTable[kWaveTableSampleCount];
    static const std::int8_t kBassTable[kWaveTableSampleCount];

private:
    const std::int8_t* table_;
    std::uint32_t phase_ = 0;
    std::uint32_t phaseStep_ = 0;
    std::uint32_t sampleRate_ = kSampleRate;
    std::uint16_t divider_ = 0;
    Cmu800Envelope envelope_;
};
