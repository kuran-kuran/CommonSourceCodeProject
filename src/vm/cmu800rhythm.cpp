#include "Cmu800Rhythm.h"

#include <algorithm>

Cmu800Rhythm::Cmu800Rhythm(const std::int8_t* sample_data,
	std::uint32_t sample_count) :
	sample_data(sample_data), sample_count(sample_count), position_q32(0),
	step_q32(1ull << 32), sample_rate(48000u), playing(false)
{
}

void Cmu800Rhythm::set_sample(const std::int8_t* sample_data,
	std::uint32_t sample_count)
{
	this->sample_data = sample_data;
	this->sample_count = sample_count;
	initialize();
}

void Cmu800Rhythm::initialize()
{
	position_q32 = 0;
	playing = false;
}

void Cmu800Rhythm::initialize(std::uint32_t sample_rate)
{
	set_sample_rate(sample_rate);
	initialize();
}

void Cmu800Rhythm::set_sample_rate(std::uint32_t sample_rate)
{
	this->sample_rate = sample_rate != 0 ? sample_rate : 48000u;
	step_q32 = (static_cast<std::uint64_t>(48000u) << 32) / this->sample_rate;
}

void Cmu800Rhythm::trigger()
{
	position_q32 = 0;
	playing = sample_data != NULL && sample_count != 0;
}

void Cmu800Rhythm::stop()
{
	position_q32 = 0;
	playing = false;
}

bool Cmu800Rhythm::is_playing() const
{
	return playing;
}

std::int32_t Cmu800Rhythm::get_data(std::int32_t volume_q15)
{
	const std::uint64_t position = position_q32 >> 32;
	if(!playing || sample_data == NULL || position >= sample_count) {
		playing = false;
		return 0;
	}

	const std::uint32_t index = static_cast<std::uint32_t>(position);
	const std::uint32_t next_index =
		index + 1u < sample_count ? index + 1u : index;
	const std::uint32_t fraction =
		static_cast<std::uint32_t>((position_q32 >> 16) & 0xffffu);
	const std::int32_t weighted =
		static_cast<std::int32_t>(sample_data[index]) *
			static_cast<std::int32_t>(65536u - fraction) +
		static_cast<std::int32_t>(sample_data[next_index]) *
			static_cast<std::int32_t>(fraction);
	const std::int32_t sample = weighted / 65536;

	position_q32 += step_q32;
	if((position_q32 >> 32) >= sample_count) {
		playing = false;
	}
	if(volume_q15 <= 0) {
		return 0;
	}
	return (sample * std::min(volume_q15, 32767)) >> 15;
}
