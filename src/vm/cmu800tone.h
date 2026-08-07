#pragma once

#include "Cmu800Envelope.h"

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

    // Envelope control.  Trigger() is intended for a GATE transition.
    void SetDecay(std::uint8_t value);
    void SetDecayFactorQ31(std::uint32_t factorQ31);
    void EnableSustain(bool enabled);
    void SetSustain(std::uint8_t value);
    void SetGate(bool gateIsOn);
    void Trigger();
    void Stop();
    bool IsPlaying() const;

    // Get one 48 kHz sample using the built-in CMU-800 decay envelope.
    // Call once per audio sample while mixing, including inaudible tails.
    std::int32_t GetData();

    // Get one 48 kHz sample using both the built-in envelope and an external
    // Q15 volume (0..32767).  The envelope advances even when volumeQ15 is 0.
    std::int32_t GetDataWithVolume(std::int32_t volumeQ15);

    // Get one 48 kHz sample.  volumeQ15 is 0..32767; 32767 is full level.
    // This overload leaves the built-in envelope unchanged and is useful for
    // an externally controlled volume or a sustain sound.
    std::int32_t GetData(std::int32_t volumeQ15);

private:
    static std::uint32_t MakePhaseStepFrom8253(std::uint16_t divider);

    const std::int8_t* table_;
    std::uint32_t phase_;
    std::uint32_t phaseStep_;
    Cmu800Envelope envelope_;
};
