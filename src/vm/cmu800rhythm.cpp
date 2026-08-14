#include "Cmu800Rhythm.h"

#include <algorithm>

Cmu800Rhythm::Cmu800Rhythm(const std::int8_t* sample,
                           std::uint32_t sampleCount) :
    sample_(sample), sampleCount_(sampleCount), position_(0), playing_(false)
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
    position_ = 0;
    playing_ = false;
}

void Cmu800Rhythm::Trigger()
{
    position_ = 0;
    playing_ = sample_ != NULL && sampleCount_ != 0;
}

void Cmu800Rhythm::Stop()
{
    position_ = 0;
    playing_ = false;
}

bool Cmu800Rhythm::IsPlaying() const
{
    return playing_;
}

std::int32_t Cmu800Rhythm::GetData(std::int32_t volumeQ15)
{
    if (!playing_ || sample_ == NULL || position_ >= sampleCount_) {
        playing_ = false;
        return 0;
    }

    const std::int32_t sample = sample_[position_++];
    if (position_ == sampleCount_) playing_ = false;
    if (volumeQ15 <= 0) return 0;
    return (sample * std::min(volumeQ15, 32767)) >> 15;
}
