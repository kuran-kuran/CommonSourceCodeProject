#include "Cmu800Tone.h"

#include <algorithm>

Cmu800Tone::Cmu800Tone(const int8_t* table) :
    table_(table), phase_(0), phaseStep_(0), sampleRate_(48000u), divider_(0)
{
}

void Cmu800Tone::SetWaveTable(const int8_t* table)
{
    table_ = table;
}

void Cmu800Tone::Initialize()
{
    phase_ = 0;
    phaseStep_ = 0;
    divider_ = 0;
    envelope_.Initialize();
}

void Cmu800Tone::Initialize(uint32_t sampleRate)
{
    SetSampleRate(sampleRate);
    Initialize();
}

void Cmu800Tone::SetSampleRate(uint32_t sampleRate)
{
    sampleRate_ = sampleRate != 0 ? sampleRate : 48000u;
    envelope_.SetSampleRate(sampleRate_);
    phaseStep_ = MakePhaseStepFrom8253(divider_);
}

uint32_t Cmu800Tone::MakePhaseStepFrom8253(uint16_t divider) const
{
    constexpr uint32_t k8253ClockHz = 1269866u;
    if (divider == 0) return 0;

    // phaseStep = (8253Clock / divider) * 2^32 / sampleRate.
    // Called only when a note changes, never in the audio-sample routine.
    return static_cast<uint32_t>(
        (static_cast<uint64_t>(k8253ClockHz) << 32) /
        (static_cast<uint64_t>(sampleRate_) * divider));
}

void Cmu800Tone::Set8253(uint16_t divider)
{
    divider_ = divider;
    phaseStep_ = MakePhaseStepFrom8253(divider);
}

void Cmu800Tone::SetDecay(uint8_t value)
{
    envelope_.SetDecay(value);
}

void Cmu800Tone::SetDecayFactorQ31(uint32_t factorQ31)
{
    envelope_.SetDecayFactorQ31(factorQ31);
}

void Cmu800Tone::EnableSustain(bool enabled)
{
    envelope_.EnableSustain(enabled);
}

void Cmu800Tone::SetSustain(uint8_t value)
{
    envelope_.SetSustain(value);
}

void Cmu800Tone::SetGate(bool gateIsOn)
{
    envelope_.SetGate(gateIsOn);
}

void Cmu800Tone::Trigger()
{
    envelope_.Trigger();
}

void Cmu800Tone::Stop()
{
    envelope_.Stop();
}

bool Cmu800Tone::IsPlaying() const
{
    return envelope_.IsActive();
}

int32_t Cmu800Tone::GetData()
{
    return GetDataWithVolume(32767);
}

int32_t Cmu800Tone::GetDataWithVolume(int32_t volumeQ15)
{
    const int32_t envelopeVolumeQ15 = envelope_.GetVolumeQ15AndAdvance();
    const int32_t externalVolumeQ15 =
        std::min(std::max(volumeQ15, 0), 32767);
    const int32_t combinedVolumeQ15 =
        (envelopeVolumeQ15 * externalVolumeQ15 + (1 << 14)) >> 15;
    return GetData(combinedVolumeQ15);
}

int32_t Cmu800Tone::GetData(int32_t volumeQ15)
{
    constexpr uint32_t kTableIndexShift = 20u; // 32 - log2(4096)
    if (table_ == NULL) return 0;

    const int32_t sample = table_[phase_ >> kTableIndexShift];
    phase_ += phaseStep_;
    if (volumeQ15 <= 0) return 0;
    return (sample * std::min(volumeQ15, 32767)) >> 15;
}
