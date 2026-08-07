#include "Cmu800Envelope.h"

#include <algorithm>

Cmu800Envelope::Cmu800Envelope()
    : levelQ31_(0),
      decayFactorQ31_(kDecayMiddleQ31)
{
}

void Cmu800Envelope::Initialize()
{
    levelQ31_ = 0;
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

void Cmu800Envelope::Trigger()
{
    levelQ31_ = 0x7fffffffu;
}

void Cmu800Envelope::Stop()
{
    levelQ31_ = 0;
}

bool Cmu800Envelope::IsActive() const
{
    return levelQ31_ != 0;
}

std::int32_t Cmu800Envelope::GetVolumeQ15AndAdvance()
{
    const std::int32_t volumeQ15 = static_cast<std::int32_t>(levelQ31_ >> 16);

    // Q31 x Q31 -> Q31.  uint64_t is important on Pico as well: the
    // intermediate product does not fit in 32 bits.
    levelQ31_ = static_cast<std::uint32_t>(
        (static_cast<std::uint64_t>(levelQ31_) * decayFactorQ31_ +
         (1ull << 30)) >> 31);

    // Avoid spending time on inaudible tail samples forever.
    if (levelQ31_ < (1u << 12)) {
        levelQ31_ = 0;
    }

    return volumeQ15;
}
