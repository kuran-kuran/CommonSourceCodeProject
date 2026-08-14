#include "Cmu800Envelope.h"

#include <algorithm>

namespace {

// Release time uses a perceptually smoother squared curve: Sustain 1 is
// approximately 3 ms and Sustain 255 is approximately 1.10 s.  Keeping the
// precomputed Q31 factors in Flash avoids floating-point exp() on Pico.
constexpr uint32_t kReleaseFactorQ31[256] = {
    0u, 2132622229u, 2132705698u, 2132950571u, 2133341145u, 2133853958u, 2134461078u, 2135133454u,
    2135843714u, 2136568048u, 2137287160u, 2137986439u, 2138655599u, 2139288045u, 2139880132u, 2140430451u,
    2140939204u, 2141407693u, 2141837921u, 2142232296u, 2142593417u, 2142923925u, 2143226408u, 2143503334u,
    2143757020u, 2143989611u, 2144203074u, 2144399206u, 2144579633u, 2144745828u, 2144899117u, 2145040696u,
    2145171640u, 2145292914u, 2145405389u, 2145509844u, 2145606982u, 2145697436u, 2145781776u, 2145860516u,
    2145934119u, 2146003004u, 2146067552u, 2146128104u, 2146184974u, 2146238444u, 2146288772u, 2146336191u,
    2146380915u, 2146423139u, 2146463041u, 2146500784u, 2146536517u, 2146570378u, 2146602492u, 2146632974u,
    2146661932u, 2146689463u, 2146715657u, 2146740598u, 2146764364u, 2146787026u, 2146808650u, 2146829298u,
    2146849026u, 2146867888u, 2146885932u, 2146903205u, 2146919750u, 2146935606u, 2146950811u, 2146965399u,
    2146979403u, 2146992853u, 2147005778u, 2147018205u, 2147030157u, 2147041660u, 2147052735u, 2147063403u,
    2147073683u, 2147083594u, 2147093153u, 2147102376u, 2147111279u, 2147119877u, 2147128182u, 2147136209u,
    2147143969u, 2147151473u, 2147158733u, 2147165760u, 2147172563u, 2147179151u, 2147185533u, 2147191719u,
    2147197715u, 2147203529u, 2147209169u, 2147214641u, 2147219952u, 2147225108u, 2147230115u, 2147234979u,
    2147239706u, 2147244299u, 2147248765u, 2147253107u, 2147257331u, 2147261440u, 2147265439u, 2147269332u,
    2147273121u, 2147276812u, 2147280407u, 2147283909u, 2147287322u, 2147290648u, 2147293891u, 2147297054u,
    2147300138u, 2147303147u, 2147306083u, 2147308947u, 2147311744u, 2147314474u, 2147317140u, 2147319743u,
    2147322286u, 2147324771u, 2147327199u, 2147329571u, 2147331891u, 2147334158u, 2147336376u, 2147338544u,
    2147340665u, 2147342740u, 2147344771u, 2147346758u, 2147348703u, 2147350606u, 2147352470u, 2147354295u,
    2147356082u, 2147357833u, 2147359548u, 2147361228u, 2147362875u, 2147364488u, 2147366070u, 2147367620u,
    2147369140u, 2147370630u, 2147372092u, 2147373525u, 2147374931u, 2147376310u, 2147377664u, 2147378992u,
    2147380295u, 2147381574u, 2147382829u, 2147384062u, 2147385272u, 2147386460u, 2147387627u, 2147388773u,
    2147389899u, 2147391004u, 2147392091u, 2147393158u, 2147394207u, 2147395238u, 2147396251u, 2147397247u,
    2147398226u, 2147399188u, 2147400135u, 2147401065u, 2147401980u, 2147402880u, 2147403766u, 2147404636u,
    2147405493u, 2147406336u, 2147407165u, 2147407981u, 2147408784u, 2147409574u, 2147410352u, 2147411118u,
    2147411872u, 2147412614u, 2147413345u, 2147414064u, 2147414773u, 2147415471u, 2147416158u, 2147416835u,
    2147417502u, 2147418159u, 2147418806u, 2147419443u, 2147420072u, 2147420691u, 2147421301u, 2147421902u,
    2147422495u, 2147423079u, 2147423655u, 2147424223u, 2147424782u, 2147425334u, 2147425878u, 2147426415u,
    2147426944u, 2147427466u, 2147427981u, 2147428488u, 2147428989u, 2147429483u, 2147429970u, 2147430451u,
    2147430925u, 2147431394u, 2147431855u, 2147432311u, 2147432761u, 2147433205u, 2147433643u, 2147434075u,
    2147434502u, 2147434924u, 2147435339u, 2147435750u, 2147436156u, 2147436556u, 2147436951u, 2147437342u,
    2147437727u, 2147438108u, 2147438484u, 2147438855u, 2147439222u, 2147439584u, 2147439942u, 2147440296u,
    2147440645u, 2147440990u, 2147441331u, 2147441668u, 2147442001u, 2147442330u, 2147442655u, 2147442976u
};

} // namespace

Cmu800Envelope::Cmu800Envelope()
    : levelQ31_(0),
      decayFactorQ31_(kDecayMiddleQ31),
      sustainLevelQ31_(0),
      releaseFactorQ31_(kDecayMinimumQ31),
      sampleRate_(48000u),
      referenceClockAccumulator_(0),
      sustainEnabled_(false),
      gateIsOn_(false),
      stage_(Stage::Inactive)
{
}

void Cmu800Envelope::Initialize()
{
    levelQ31_ = 0;
    referenceClockAccumulator_ = 0;
    gateIsOn_ = false;
    stage_ = Stage::Inactive;
}

void Cmu800Envelope::Initialize(uint32_t sampleRate)
{
    SetSampleRate(sampleRate);
    Initialize();
}

void Cmu800Envelope::SetSampleRate(uint32_t sampleRate)
{
    sampleRate_ = sampleRate != 0 ? sampleRate : 48000u;
    referenceClockAccumulator_ = 0;
}

void Cmu800Envelope::SetDecayFactorQ31(uint32_t factorQ31)
{
    decayFactorQ31_ = std::min(factorQ31, 0x7fffffffu);
}

void Cmu800Envelope::SetDecay(uint8_t value)
{
    // This control path runs only when the user changes the Decay setting,
    // never in the 48 kHz audio loop.  Keep the three measured anchor points
    // exact; more measured points can later replace this with a lookup table.
    if (value <= 128u) {
        const uint32_t distance = kDecayMiddleQ31 - kDecayMinimumQ31;
        const uint32_t factor = kDecayMinimumQ31 +
            static_cast<uint32_t>(
                (static_cast<uint64_t>(distance) * value + 64u) / 128u);
        SetDecayFactorQ31(factor);
        return;
    }

    const uint32_t distance = kDecayMaximumQ31 - kDecayMiddleQ31;
    const uint32_t factor = kDecayMiddleQ31 +
        static_cast<uint32_t>(
            (static_cast<uint64_t>(distance) * (value - 128u) + 63u) / 127u);
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

void Cmu800Envelope::SetSustain(uint8_t value)
{
    // The ch1 recording reaches about -16 dB at the highest Sustain setting.
    // Keep this as an isolated calibration constant until all 11 measured
    // points are converted into a more exact lookup table.
    constexpr uint32_t kMaximumSustainLevelQ31 = 0x15000000u;
    sustainLevelQ31_ = static_cast<uint32_t>(
        (static_cast<uint64_t>(kMaximumSustainLevelQ31) * value + 127u) /
        255u);

    // Use a precomputed time-domain curve rather than interpolating Q31
    // multipliers.  Interpolating multipliers made Sustain 0 stop immediately
    // while Sustain 1 unexpectedly had a roughly 0.33-second release.
    releaseFactorQ31_ = kReleaseFactorQ31[value];
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
    referenceClockAccumulator_ = 0;
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

void Cmu800Envelope::Advance(uint32_t factorQ31)
{
    levelQ31_ = static_cast<uint32_t>(
        (static_cast<uint64_t>(levelQ31_) * factorQ31 +
         (1ull << 30)) >> 31);
}

void Cmu800Envelope::AdvanceOneReferenceSample()
{
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
}

int32_t Cmu800Envelope::GetVolumeQ15AndAdvance()
{
    const int32_t volumeQ15 = static_cast<int32_t>(levelQ31_ >> 16);

    // The measured factors are defined at 48 kHz.  Advance them on a virtual
    // 48 kHz clock so envelope times remain unchanged at any output rate.
    referenceClockAccumulator_ += 48000u;
    while (referenceClockAccumulator_ >= sampleRate_) {
        referenceClockAccumulator_ -= sampleRate_;
        AdvanceOneReferenceSample();
    }

    // Avoid spending time on inaudible tail samples forever.
    if (stage_ != Stage::SustainHold && levelQ31_ < (1u << 12)) {
        levelQ31_ = 0;
        stage_ = Stage::Inactive;
    }

    return volumeQ15;
}
