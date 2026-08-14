#pragma once

#include <cstdint>

// One-shot exponential decay envelope for the 48 kHz CMU-800 players.
// The internal level is Q31 so even the long Decay setting can be represented
// smoothly.  GetVolumeQ15AndAdvance() returns the Q15 value accepted by
// Cmu800Tone::GetData() and Cmu800Rhythm::GetData().
class Cmu800Envelope {
public:
    // Values measured approximately from 260806_0282_Decay.WAV.
    static constexpr uint32_t kDecayMinimumQ31 = 2147349333u; // tau about 0.31 s
    static constexpr uint32_t kDecayMiddleQ31 = 2147437047u;  // tau about 0.96 s
    // Measured from CMU-800_Decay_100per_ch1.WAV: tau about 1.1 s.
    static constexpr uint32_t kDecayMaximumQ31 = 2147442976u;

    Cmu800Envelope();

    // Reset the envelope to silence while preserving the selected decay rate.
    void Initialize();
    void Initialize(uint32_t sampleRate);
    void SetSampleRate(uint32_t sampleRate);

    // Select the per-sample Q31 multiplier.  0 is immediate silence after the
    // current sample; 0x7fffffff is the slowest representable decay.
    void SetDecayFactorQ31(uint32_t factorQ31);

    // Select decay with a convenient 0..255 control value.  0, 128 and 255
    // correspond to the currently measured minimum, middle and maximum
    // settings.  The Q31 details stay inside this class.
    void SetDecay(uint8_t value);

    // Sustain is used by CMU-800 ch1 only.  It is disabled by default so
    // ch2..ch6 retain the original one-shot Decay behavior.
    void EnableSustain(bool enabled);
    void SetSustain(uint8_t value);

    // Set the logical GATE state.  A false -> true transition starts a note;
    // a true -> false transition starts Release when Sustain is enabled.
    void SetGate(bool gateIsOn);

    // Start a new note at full level.  Call this once on the GATE transition,
    // not continuously while the GATE remains active.
    void Trigger();
    void Stop();

    bool IsActive() const;

    // Return one Q15 volume value (0..32767), then advance by one output
    // sample.  This must be called once per generated audio sample.
    int32_t GetVolumeQ15AndAdvance();

private:
    enum class Stage : uint8_t {
        Inactive,
        OneShotDecay,
        DecayToSustain,
        SustainHold,
        Release
    };

    void Advance(uint32_t factorQ31);
    void AdvanceOneReferenceSample();

    uint32_t levelQ31_;
    uint32_t decayFactorQ31_;
    uint32_t releaseFactorQ31_;
    uint32_t sustainLevelQ31_;
    uint32_t sampleRate_;
    uint64_t referenceClockAccumulator_;
    bool sustainEnabled_;
    bool gateIsOn_;
    Stage stage_;
};
