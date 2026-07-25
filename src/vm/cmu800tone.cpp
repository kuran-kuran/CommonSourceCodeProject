#include "Cmu800Tone.h"

#include <algorithm>

Cmu800Tone::Cmu800Tone(const std::int8_t* table) :
    table_(table), phase_(0), phaseStep_(0)
{
}

void Cmu800Tone::SetWaveTable(const std::int8_t* table)
{
    table_ = table;
}

void Cmu800Tone::Initialize()
{
    phase_ = 0;
    phaseStep_ = 0;
}

std::uint32_t Cmu800Tone::MakePhaseStepFrom8253(std::uint16_t divider)
{
    constexpr std::uint32_t k8253ClockHz = 1269866u;
    constexpr std::uint32_t kSampleRate = 48000u;
    if (divider == 0) return 0;

    // phaseStep = (8253Clock / divider) * 2^32 / sampleRate.
    // Called only when a note changes, never in the audio-sample routine.
    return static_cast<std::uint32_t>(
        (static_cast<std::uint64_t>(k8253ClockHz) << 32) /
        (static_cast<std::uint64_t>(kSampleRate) * divider));
}

void Cmu800Tone::Set8253(std::uint16_t divider)
{
    phaseStep_ = MakePhaseStepFrom8253(divider);
}

std::int32_t Cmu800Tone::GetData(std::int32_t volumeQ15)
{
    constexpr std::uint32_t kTableIndexShift = 20u; // 32 - log2(4096)
    if (table_ == nullptr) return 0;

    const std::int32_t sample = table_[phase_ >> kTableIndexShift];
    phase_ += phaseStep_;
    if (volumeQ15 <= 0) return 0;
    return (sample * std::min(volumeQ15, 32767)) >> 15;
}
