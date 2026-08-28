#include "Cmu800Envelope.h"

#include <algorithm>
#include <cmath>

namespace {

// Release time uses a perceptually smoother squared curve: Sustain 1 is
// approximately 3 ms and Sustain 255 is approximately 1.10 s.  Keeping the
// precomputed Q31 factors in Flash avoids floating-point exp() on Pico.
constexpr uint32_t release_factor_q31_table[256] = {
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

Cmu800Envelope::Cmu800Envelope() :
	level_q31(0),
	decay_factor_q31(decay_middle_q31),
	release_factor_q31(decay_minimum_q31),
	sample_rate(48000u),
	reference_clock_accumulator(0),
	sustain_enabled(false),
	gate_is_on(false),
	stage(stage_type::inactive)
{
}

void Cmu800Envelope::initialize()
{
	level_q31 = 0;
	reference_clock_accumulator = 0;
	gate_is_on = false;
	stage = stage_type::inactive;
}

void Cmu800Envelope::set_sample_rate(uint32_t sample_rate)
{
	this->sample_rate = sample_rate != 0 ? sample_rate : 48000u;
	reference_clock_accumulator = 0;
}

void Cmu800Envelope::set_decay_factor_q31(uint32_t factor_q31)
{
	decay_factor_q31 = std::min(factor_q31, 0x7fffffffu);
}

void Cmu800Envelope::set_decay(uint8_t value)
{
    // Use one common curve for Melody, Bass and Chord.  These three anchors
    // come from the measured Chord recording; values between them are
    // linearly interpolated so they remain easy to adjust by ear.
	constexpr double decay_0_tau = 0.227;
	constexpr double decay_5_tau = 1.054;
	constexpr double decay_10_tau = 1.068;

	double tau;
	if(value <= 128u) {
		tau = decay_0_tau +
			(decay_5_tau - decay_0_tau) * static_cast<double>(value) / 128.0;
	} else {
		tau = decay_5_tau +
			(decay_10_tau - decay_5_tau) * static_cast<double>(value - 128u) / 127.0;
	}

	const double factor = std::exp(-1.0 / (48000.0 * tau)) * 2147483648.0;
	set_decay_factor_q31(static_cast<uint32_t>(factor + 0.5));
}

void Cmu800Envelope::enable_sustain(bool enabled)
{
	sustain_enabled = enabled;
}

void Cmu800Envelope::set_sustain(uint8_t value)
{
    // CMU-800 WebSynth's Melody SUSTAIN is not a level to hold.  It selects
    // the NoteOff release time: 39 ms .. 1.039 s, linearly with the knob.
    // kReleaseFactorQ31 is an existing flash lookup with 3 ms .. 1.10 s and
    // a squared index curve, so map the desired linear time onto that table
    // using integers only.  This keeps Pico's audio loop free of float math.
    constexpr uint32_t minimum_ms = 39u;
    constexpr uint32_t range_ms = 1000u;
    constexpr uint32_t table_minimum_ms = 3u;
    constexpr uint32_t table_range_ms = 1097u;
    constexpr uint32_t table_max_index = 255u;
    const uint32_t target_ms = minimum_ms +
        (range_ms * value + 127u) / 255u;
    const uint64_t scaled = static_cast<uint64_t>(
        target_ms - table_minimum_ms) * table_max_index * table_max_index /
        table_range_ms;
    uint32_t table_index = 0;
    while(static_cast<uint64_t>(table_index + 1u) * (table_index + 1u) <=
        scaled && table_index < table_max_index) {
		++table_index;
	}
	release_factor_q31 = release_factor_q31_table[table_index];
}

void Cmu800Envelope::set_gate(bool gate_is_on)
{
    if(gate_is_on == this->gate_is_on) {
		return;
	}

	this->gate_is_on = gate_is_on;
	if(this->gate_is_on) {
		trigger();
	} else if(sustain_enabled && level_q31 != 0) {
		stage = stage_type::release;
	}
}

void Cmu800Envelope::trigger()
{
    level_q31 = 0x7fffffffu;
    reference_clock_accumulator = 0;
    // Decay always starts at NoteOn.  Melody Sustain only replaces this
    // decay with Release when its GATE falls; there is no sustain-level hold.
    stage = stage_type::one_shot_decay;
}

void Cmu800Envelope::stop()
{
    level_q31 = 0;
    gate_is_on = false;
    stage = stage_type::inactive;
}

bool Cmu800Envelope::is_active() const
{
    return level_q31 != 0;
}

void Cmu800Envelope::advance(uint32_t factor_q31)
{
    level_q31 = static_cast<uint32_t>(
        (static_cast<uint64_t>(level_q31) * factor_q31 +
         (1ull << 30)) >> 31);
}

void Cmu800Envelope::advance_one_reference_sample()
{
    switch(stage) {
    case stage_type::one_shot_decay:
		advance(decay_factor_q31);
		break;

    case stage_type::release:
		advance(release_factor_q31);
		break;

    case stage_type::inactive:
		break;
	}
}

int32_t Cmu800Envelope::get_volume_q15_and_advance()
{
    const int32_t volume_q15 = static_cast<int32_t>(level_q31 >> 16);

    // The measured factors are defined at 48 kHz.  Advance them on a virtual
    // 48 kHz clock so envelope times remain unchanged at any output rate.
    reference_clock_accumulator += 48000u;
    while(reference_clock_accumulator >= sample_rate) {
		reference_clock_accumulator -= sample_rate;
		advance_one_reference_sample();
	}

    // WebSynth stops Melody about seven release time constants after NoteOff.
    // The former 1<<12 cutoff was around thirteen time constants, which made
    // key-off tails noticeably longer than the reference.
    const uint32_t silent_threshold = stage == stage_type::release ?
        500000u : (1u << 12);
    if(level_q31 < silent_threshold) {
		level_q31 = 0;
		stage = stage_type::inactive;
	}

    return volume_q15;
}
