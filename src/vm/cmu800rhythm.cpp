#include "Cmu800Rhythm.h"

#include <algorithm>

Cmu800Rhythm::Cmu800Rhythm(const std::int8_t* sample,
                           std::uint32_t sampleCount) :
    sample_(sample), sampleCount_(sampleCount), positionQ32_(0),
    stepQ32_(1ull << 32), sampleRate_(48000u), playing_(false)
{
}

void Cmu800Rhythm::SetSample(const std::int8_t* sample,
                             std::uint32_t sampleCount)
{
    sample_ = sample;
    sampleCount_ = sampleCount;
    Initialize();
}

void Cmu800Rhythm::Initialize()
{
    positionQ32_ = 0;
    playing_ = false;
}

void Cmu800Rhythm::Initialize(std::uint32_t sampleRate)
{
    SetSampleRate(sampleRate);
    Initialize();
}

void Cmu800Rhythm::SetSampleRate(std::uint32_t sampleRate)
{
    sampleRate_ = sampleRate != 0 ? sampleRate : 48000u;
    stepQ32_ = (static_cast<std::uint64_t>(48000u) << 32) / sampleRate_;
}

void Cmu800Rhythm::Trigger()
{
    positionQ32_ = 0;
    playing_ = sample_ != NULL && sampleCount_ != 0;
}

void Cmu800Rhythm::Stop()
{
    positionQ32_ = 0;
    playing_ = false;
}

bool Cmu800Rhythm::IsPlaying() const
{
    return playing_;
}

std::int32_t Cmu800Rhythm::GetData(std::int32_t volumeQ15)
{
    const std::uint64_t position = positionQ32_ >> 32;
    if (!playing_ || sample_ == NULL || position >= sampleCount_) {
        playing_ = false;
        return 0;
    }

    const std::uint32_t index = static_cast<std::uint32_t>(position);
    const std::uint32_t nextIndex =
        index + 1u < sampleCount_ ? index + 1u : index;
    const std::uint32_t fraction =
        static_cast<std::uint32_t>((positionQ32_ >> 16) & 0xffffu);
    const std::int32_t weighted =
        static_cast<std::int32_t>(sample_[index]) *
            static_cast<std::int32_t>(65536u - fraction) +
        static_cast<std::int32_t>(sample_[nextIndex]) *
            static_cast<std::int32_t>(fraction);
    const std::int32_t sample = weighted / 65536;

    positionQ32_ += stepQ32_;
    if ((positionQ32_ >> 32) >= sampleCount_) playing_ = false;
    if (volumeQ15 <= 0) return 0;
    return (sample * std::min(volumeQ15, 32767)) >> 15;
}
