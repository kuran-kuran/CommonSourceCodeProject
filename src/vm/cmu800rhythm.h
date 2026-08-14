#pragma once

#include <cstddef>
#include <cstdint>

// One CMU-800 style rhythm voice using a signed 8-bit, 48 kHz PCM sample.
// Unlike Cmu800Tone, this is a one-shot player: Trigger() starts at
// the first byte and playback stops automatically at the end of the sample.
// The returned value is signed and ready to add to an int32_t mixer.
class Cmu800Rhythm {
public:
    Cmu800Rhythm(const std::int8_t* sample = NULL,
                 std::uint32_t sampleCount = 0);

    void SetSample(const std::int8_t* sample, std::uint32_t sampleCount);
    void Initialize();

    // Start the drum sound from its beginning.  Calling Trigger() again
    // restarts the same sound, as the CMU-800 rhythm bits do.
    void Trigger();
    void Stop();

    bool IsPlaying() const;

    // Get one 48 kHz sample. volumeQ15 is 0..32767; 32767 is full level.
    // After the final sample this returns zero until the next Trigger().
    std::int32_t GetData(std::int32_t volumeQ15);

private:
    const std::int8_t* sample_;
    std::uint32_t sampleCount_;
    std::uint32_t position_;
    bool playing_;
};
