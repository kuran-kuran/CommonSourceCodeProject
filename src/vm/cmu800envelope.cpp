#include "Cmu800Envelope.h"

#include <algorithm>

Cmu800Envelope::Cmu800Envelope()
    : levelQ31_(0),
      decayFactorQ31_(kDecayMiddleQ31),
      sustainLevelQ31_(0),
      releaseFactorQ31_(kDecayMinimumQ31),
      sustainEnabled_(false),
      gateIsOn_(false),
      stage_(Stage::Inactive)
{
}

void Cmu800Envelope::Initialize()
{
    levelQ31_ = 0;
    gateIsOn_ = false;
    stage_ = Stage::Inactive;
}

void Cmu800Envelope::SetDecayFactorQ31(std::uint32_t factorQ31)
{
    decayFactorQ31_ = std::min(factorQ31, 0x7fffffffu);
}

void Cmu800Envelope::SetDecay(std::uint8_t value)
{
    // This control path runs only when the user changes the Decay setting,
    // never in the 48 kHz audio loop.  Keep the three measured anchor points
    // exact; more measured points can later replace this with a lookup table.
    if (value <= 128u) {
        const std::uint32_t distance = kDecayMiddleQ31 - kDecayMinimumQ31;
        const std::uint32_t factor = kDecayMinimumQ31 +
            static_cast<std::uint32_t>(
                (static_cast<std::uint64_t>(distance) * value + 64u) / 128u);
        SetDecayFactorQ31(factor);
        return;
    }

    const std::uint32_t distance = kDecayMaximumQ31 - kDecayMiddleQ31;
    const std::uint32_t factor = kDecayMiddleQ31 +
        static_cast<std::uint32_t>(
            (static_cast<std::uint64_t>(distance) * (value - 128u) + 63u) / 127u);
    SetDecayFactorQ31(factor);
}

void Cmu800Envelope::EnableSustain(bool enabled)
{
    sustainEnabled_ = enabled;

    // Disabling Sustain during a note returns to the original one-shot mode.
    if (!sustainEnabled_ && levelQ31_ != 0) {
        stage_ = Stage::OneShotDecay;
    }
}

void Cmu800Envelope::SetSustain(std::uint8_t value)
{
    // The ch1 recording reaches about -16 dB at the highest Sustain setting.
    // Keep this as an isolated calibration constant until all 11 measured
    // points are converted into a more exact lookup table.
    constexpr std::uint32_t kMaximumSustainLevelQ31 = 0x15000000u;
    sustainLevelQ31_ = static_cast<std::uint32_t>(
        (static_cast<std::uint64_t>(kMaximumSustainLevelQ31) * value + 127u) /
        255u);

    // The immediate-key-off recordings also show a longer tail at higher
    // Sustain settings.  Use an independent Release multiplier so a short
    // Decay setting does not force Release to end immediately.
    const std::uint32_t releaseRange = kDecayMaximumQ31 - kDecayMinimumQ31;
    releaseFactorQ31_ = kDecayMinimumQ31 + static_cast<std::uint32_t>(
        (static_cast<std::uint64_t>(releaseRange) * value + 127u) / 255u);
}

void Cmu800Envelope::SetGate(bool gateIsOn)
{
    if (gateIsOn == gateIsOn_) return;

    gateIsOn_ = gateIsOn;
    if (gateIsOn_) {
        Trigger();
    } else if (sustainEnabled_ && levelQ31_ != 0) {
        // At the minimum Sustain setting, key-off cuts the ch1 sound
        // immediately.  Do not run the shortest Decay setting here: it is
        // still audible for a while and makes Sustain=0 behave incorrectly.
        if (sustainLevelQ31_ == 0) {
            Stop();
        } else {
            stage_ = Stage::Release;
        }
    }
}

void Cmu800Envelope::Trigger()
{
    levelQ31_ = 0x7fffffffu;
    stage_ = sustainEnabled_ ? Stage::DecayToSustain : Stage::OneShotDecay;
}

void Cmu800Envelope::Stop()
{
    levelQ31_ = 0;
    gateIsOn_ = false;
    stage_ = Stage::Inactive;
}

bool Cmu800Envelope::IsActive() const
{
    return levelQ31_ != 0;
}

void Cmu800Envelope::Advance(std::uint32_t factorQ31)
{
    levelQ31_ = static_cast<std::uint32_t>(
        (static_cast<std::uint64_t>(levelQ31_) * factorQ31 +
         (1ull << 30)) >> 31);
}

std::int32_t Cmu800Envelope::GetVolumeQ15AndAdvance()
{
    const std::int32_t volumeQ15 = static_cast<std::int32_t>(levelQ31_ >> 16);

    switch (stage_) {
    case Stage::OneShotDecay:
        Advance(decayFactorQ31_);
        break;

    case Stage::Release:
        Advance(releaseFactorQ31_);
        break;

    case Stage::DecayToSustain:
        Advance(decayFactorQ31_);
        if (levelQ31_ <= sustainLevelQ31_) {
            levelQ31_ = sustainLevelQ31_;
            stage_ = gateIsOn_ ? Stage::SustainHold : Stage::Release;
        }
        break;

    case Stage::SustainHold:
    case Stage::Inactive:
        break;
    }

    // Avoid spending time on inaudible tail samples forever.
    if (stage_ != Stage::SustainHold && levelQ31_ < (1u << 12)) {
        levelQ31_ = 0;
        stage_ = Stage::Inactive;
    }

    return volumeQ15;
}
