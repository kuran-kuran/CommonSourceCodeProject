/*
	Skelton for retropc emulator

	Author : kuran_kuran
	Date   : 2024.05.25-

	[ AMDEK/RolandDG CMU-800 (MIDI) ]
*/

#include "cmu800.h"
#include "midi.h"

#define EVENT_TEMPO	0

const uint8_t rhythm_table[8] = {0, 42, 46, 49, 48, 41, 38, 35};

const int counterTable[] =
{
	0x9741, 0x8EBE, 0x86BB, 0x7F2E, 0x780B, 0x714E, 0x6AED, 0x64EC, 0x5F41, 0x59E8, 0x54D9, 0x5015,
	0x4B95, 0x4754, 0x4353, 0x3F8D, 0x3BFC, 0x389E, 0x356E, 0x326E, 0x2F99, 0x2CED, 0x2A66, 0x2805,
	0x25C5, 0x23A5, 0x21A5, 0x1FC3, 0x1DFB, 0x1C4C, 0x1AB4, 0x1934, 0x17CA, 0x1674, 0x1531, 0x1401,
	0x12E1, 0x11D1, 0x10D1, 0x0FE1, 0x0EFD, 0x0E26, 0x0D59, 0x0C99, 0x0BE4, 0x0B39, 0x0A98, 0x0A00,
	0x0970, 0x08E8, 0x0868, 0x07F0, 0x077E, 0x0712, 0x06AC, 0x064C, 0x05F2, 0x059C, 0x054C, 0x0500,
	0x04B8, 0x0474, 0x0434, 0x03F8, 0x03BF, 0x0389, 0x0356, 0x0326, 0x02F9, 0x02CE, 0x02A6, 0x0280,
	0x025C, 0x023A, 0x021A, 0x01FC, 0x01DF, 0x01C4, 0x01AB, 0x0193, 0x017C, 0x0167, 0x0153, 0x0140,
	0x012E, 0x011D, 0x010D, 0x00FE, 0x00F0, 0x00E2, 0x00D5, 0x00C9, 0x00BE, 0x00B3, 0x00A9, 0x00A0,
	0x0097, 0x008E, 0x0086, 0x007F, 0x0078, 0x0071, 0x006A, 0x0064, 0x005F, 0x0059, 0x0054, 0x0050,
	0x004B, 0x0047, 0x0043, 0x003F, 0x003C
};

#define GENERAL_PARAM_CMU800_TEMPO   0
#define GENERAL_PARAM_CMU800_SUSTAIN 1
#define GENERAL_PARAM_CMU800_DECAY1  2
#define GENERAL_PARAM_CMU800_DECAY2  3
#define GENERAL_PARAM_CMU800_DECAY3  4
#define TEMPO_INI 160
#define TEMPO_MIN 10
#define TEMPO_MAX 500

void CMU800::initialize()
{
	config.option_switch &= ~OPTION_SWITCH_CMU800_TEMPO_INC_10;
	config.option_switch &= ~OPTION_SWITCH_CMU800_TEMPO_DEC_10;
	config.option_switch &= ~OPTION_SWITCH_CMU800_TEMPO_INC_5;
	config.option_switch &= ~OPTION_SWITCH_CMU800_TEMPO_DEC_5;
	config.option_switch &= ~OPTION_SWITCH_CMU800_TEMPO_INC_1;
	config.option_switch &= ~OPTION_SWITCH_CMU800_TEMPO_DEC_1;
	config.option_switch &= ~OPTION_SWITCH_CMU800_TEMPO_160;
	config.option_switch &= ~OPTION_SWITCH_CMU800_MELODY_SUSTAIN_INC_1;
	config.option_switch &= ~OPTION_SWITCH_CMU800_MELODY_SUSTAIN_DEC_1;
	config.option_switch &= ~OPTION_SWITCH_CMU800_MELODY_DECAY_INC_1;
	config.option_switch &= ~OPTION_SWITCH_CMU800_MELODY_DECAY_DEC_1;
	config.option_switch &= ~OPTION_SWITCH_CMU800_BASS_DECAY_INC_1;
	config.option_switch &= ~OPTION_SWITCH_CMU800_BASS_DECAY_DEC_1;
	config.option_switch &= ~OPTION_SWITCH_CMU800_CHORD_DECAY_INC_1;
	config.option_switch &= ~OPTION_SWITCH_CMU800_CHORD_DECAY_DEC_1;
	use_midi = false;
	memset(regs, 0, sizeof(regs));
	is_reset = false;

	if((tempo_new = config.general_param[GENERAL_PARAM_CMU800_TEMPO]) <= 0)
	{
		tempo_new = TEMPO_INI;
		config.general_param[GENERAL_PARAM_CMU800_TEMPO] = tempo_new;
	}
	tempo_freq = tempo_new;
	register_event(this, EVENT_TEMPO, 1000000.0 / (tempo_freq * 80 / 100), true, &tempo_id);
	emu->out_message(_T("CMU-800: Tempo = %d"), tempo_freq);
	// CMU-800 wave
	memset(melody_data, 0, sizeof(melody_data));
	memset(bass_data, 0, sizeof(bass_data));
	melody_wave = NULL;
	bass_wave = NULL;
	b7BD_wave = NULL;
	b6SD_wave = NULL;
	b5LT_wave = NULL;
	b4HT_wave = NULL;
	b3CY_wave = NULL;
	b2OH_wave = NULL;
	b1CH_wave = NULL;
	b0User_wave = NULL;
	if(!use_midi)
	{
		uint8_t* data_ptr;
		uint32_t data_size;
		FILEIO* fio = new FILEIO();
		if(fio->Fopen(create_local_path(_T("CMU800Merody.wav")), FILEIO_READ_BINARY))
		{
			long fileLength = fio->FileLength();
			melody_wave = (uint8_t*)malloc(fileLength);
			fio->Fread(melody_wave, fileLength, 1);
			fio->Fclose();
			bool result = get_wave_data(melody_wave, &data_ptr, &data_size);
			if(result)
			{
				memcpy(melody_data, data_ptr, sizeof(melody_data));
			}
		}
		if(fio->Fopen(create_local_path(_T("CMU800Bass.wav")), FILEIO_READ_BINARY))
		{
			long fileLength = fio->FileLength();
			bass_wave = (uint8_t*)malloc(fileLength);
			fio->Fread(bass_wave, fileLength, 1);
			fio->Fclose();
			bool result = get_wave_data(bass_wave, &data_ptr, &data_size);
			if(result)
			{
				memcpy(bass_data, data_ptr, sizeof(bass_data));
			}
		}
		if(fio->Fopen(create_local_path(_T("CMU800BassDrum.wav")), FILEIO_READ_BINARY))
		{
			long fileLength = fio->FileLength();
			b7BD_wave = (uint8_t*)malloc(fileLength);
			fio->Fread(b7BD_wave, fileLength, 1);
			fio->Fclose();
			bool result = get_wave_data(b7BD_wave, &data_ptr, &data_size);
			if(result)
			{
				b7BD_data = (int8_t*)data_ptr;
				rhythm[7].SetSample(b7BD_data, data_size);
			}
		}
		if(fio->Fopen(create_local_path(_T("CMU800SnareDrum.wav")), FILEIO_READ_BINARY))
		{
			long fileLength = fio->FileLength();
			b6SD_wave = (uint8_t*)malloc(fileLength);
			fio->Fread(b6SD_wave, fileLength, 1);
			fio->Fclose();
			bool result = get_wave_data(b6SD_wave, &data_ptr, &data_size);
			if(result)
			{
				b6SD_data = (int8_t*)data_ptr;
				rhythm[6].SetSample(b6SD_data, data_size);
			}
		}
		if(fio->Fopen(create_local_path(_T("CMU800LowTom.wav")), FILEIO_READ_BINARY))
		{
			long fileLength = fio->FileLength();
			b5LT_wave = (uint8_t*)malloc(fileLength);
			fio->Fread(b5LT_wave, fileLength, 1);
			fio->Fclose();
			bool result = get_wave_data(b5LT_wave, &data_ptr, &data_size);
			if(result)
			{
				b5LT_data = (int8_t*)data_ptr;
				rhythm[5].SetSample(b5LT_data, data_size);
			}
		}
		if(fio->Fopen(create_local_path(_T("CMU800HighTom.wav")), FILEIO_READ_BINARY))
		{
			long fileLength = fio->FileLength();
			b4HT_wave = (uint8_t*)malloc(fileLength);
			fio->Fread(b4HT_wave, fileLength, 1);
			fio->Fclose();
			bool result = get_wave_data(b4HT_wave, &data_ptr, &data_size);
			if(result)
			{
				b4HT_data = (int8_t*)data_ptr;
				rhythm[4].SetSample(b4HT_data, data_size);
			}
		}
		if(fio->Fopen(create_local_path(_T("CMU800Cymbal.wav")), FILEIO_READ_BINARY))
		{
			long fileLength = fio->FileLength();
			b3CY_wave = (uint8_t*)malloc(fileLength);
			fio->Fread(b3CY_wave, fileLength, 1);
			fio->Fclose();
			bool result = get_wave_data(b3CY_wave, &data_ptr, &data_size);
			if(result)
			{
				b3CY_data = (int8_t*)data_ptr;
				rhythm[3].SetSample(b3CY_data, data_size);
			}
		}
		if(fio->Fopen(create_local_path(_T("CMU800OpenHiHat.wav")), FILEIO_READ_BINARY))
		{
			long fileLength = fio->FileLength();
			b2OH_wave = (uint8_t*)malloc(fileLength);
			fio->Fread(b2OH_wave, fileLength, 1);
			fio->Fclose();
			bool result = get_wave_data(b2OH_wave, &data_ptr, &data_size);
			if(result)
			{
				b2OH_data = (int8_t*)data_ptr;
				rhythm[2].SetSample(b2OH_data, data_size);
			}
		}
		if(fio->Fopen(create_local_path(_T("CMU800ClosedHiHat.wav")), FILEIO_READ_BINARY))
		{
			long fileLength = fio->FileLength();
			b1CH_wave = (uint8_t*)malloc(fileLength);
			fio->Fread(b1CH_wave, fileLength, 1);
			fio->Fclose();
			bool result = get_wave_data(b1CH_wave, &data_ptr, &data_size);
			if(result)
			{
				b1CH_data = (int8_t*)data_ptr;
				rhythm[1].SetSample(b1CH_data, data_size);
			}
		}
		if(fio->Fopen(create_local_path(_T("CMU800User.wav")), FILEIO_READ_BINARY))
		{
			long fileLength = fio->FileLength();
			b0User_wave = (uint8_t*)malloc(fileLength);
			fio->Fread(b0User_wave, fileLength, 1);
			fio->Fclose();
			bool result = get_wave_data(b0User_wave, &data_ptr, &data_size);
			if(result)
			{
				b0User_data = (int8_t*)data_ptr;
				rhythm[0].SetSample(b0User_data, data_size);
			}
		}
		int melody_channels[] = { 0, 2, 3, 4, 5 };
		uint8_t decayValue = 127;
		uint8_t sustainValue = 127;
		for(int ch : melody_channels)
		{
			tone[ch].Initialize();
			tone[ch].SetWaveTable(melody_data);
			tone[ch].SetDecay(decayValue);  // 0～255
		}
		tone[0].EnableSustain(true);
		tone[0].SetSustain(sustainValue);   // 0～255
		tone[1].Initialize();
		tone[1].SetWaveTable(bass_data);
		tone[1].SetDecay(decayValue);       // 0～255
		for (int i = 0; i < 5; ++ i)
		{
			volume_l[i] = 1024;
			volume_r[i] = 1024;
		}
		melody_sustain = 5;
		melody_decay = 5;
		bass_decay = 5;
		chord_decay = 5;
	}
}

void CMU800::release()
{
	if (melody_wave != NULL) {
		free(melody_wave);
		melody_wave = NULL;
	}
	if (bass_wave != NULL) {
		free(bass_wave);
		bass_wave = NULL;
	}
	if(b7BD_wave != NULL) {
		free(b7BD_wave);
		b7BD_wave = NULL;
	}
	if(b6SD_wave != NULL) {
		free(b6SD_wave);
		b6SD_wave = NULL;
	}
	if(b5LT_wave != NULL) {
		free(b5LT_wave);
		b5LT_wave = NULL;
	}
	if(b4HT_wave != NULL) {
		free(b4HT_wave);
		b4HT_wave = NULL;
	}
	if(b3CY_wave != NULL) {
		free(b3CY_wave);
		b3CY_wave = NULL;
	}
	if(b2OH_wave != NULL) {
		free(b2OH_wave);
		b2OH_wave = NULL;
	}
	if(b1CH_wave != NULL) {
		free(b1CH_wave);
		b1CH_wave = NULL;
	}
	if(b0User_wave != NULL) {
		free(b0User_wave);
		b0User_wave = NULL;
	}
}

void CMU800::reset()
{
	reset_midi();
	memset(toggle, 0, sizeof(toggle));
	memset(counter, 0, sizeof(counter));
	memset(before_counter, 0, sizeof(before_counter));
	cv = 0;
	note_on = false;
	memset(cv_key, 0, sizeof(cv_key));
	memset(note_on_flag, 0, sizeof(note_on_flag));
	memset(before_tone, 0, sizeof(before_tone));
	for (int i = 0; i < 8; ++i)
	{
		key_on[i] = false;
	}
	before_rhythm = 0xFE;
	for (int ch = 0; ch < 6; ++ ch)
	{
		tone[ch].Stop();
	}
	for (int i = 0; i < 8; ++ i)
	{
		rhythm[i].Stop();
	}
}

void CMU800::update_config()
{
	// Tempo
	if (config.option_switch & OPTION_SWITCH_CMU800_TEMPO_INC_10) {
		tempo_new += 10;
		config.option_switch &= ~OPTION_SWITCH_CMU800_TEMPO_INC_10;
	}
	if (config.option_switch & OPTION_SWITCH_CMU800_TEMPO_DEC_10) {
		tempo_new -= 10;
		config.option_switch &= ~OPTION_SWITCH_CMU800_TEMPO_DEC_10;
	}
	if (config.option_switch & OPTION_SWITCH_CMU800_TEMPO_INC_5) {
		tempo_new += 5;
		config.option_switch &= ~OPTION_SWITCH_CMU800_TEMPO_INC_5;
	}
	if (config.option_switch & OPTION_SWITCH_CMU800_TEMPO_DEC_5) {
		tempo_new -= 5;
		config.option_switch &= ~OPTION_SWITCH_CMU800_TEMPO_DEC_5;
	}
	if (config.option_switch & OPTION_SWITCH_CMU800_TEMPO_INC_1) {
		tempo_new++;
		config.option_switch &= ~OPTION_SWITCH_CMU800_TEMPO_INC_1;
	}
	if (config.option_switch & OPTION_SWITCH_CMU800_TEMPO_DEC_1) {
		tempo_new--;
		config.option_switch &= ~OPTION_SWITCH_CMU800_TEMPO_DEC_1;
	}
	if (config.option_switch & OPTION_SWITCH_CMU800_TEMPO_160) {
		tempo_new = 160;
		config.option_switch &= ~OPTION_SWITCH_CMU800_TEMPO_160;
	}
	if (tempo_new > TEMPO_MAX) {
		tempo_new = TEMPO_MAX;
	}
	else if (tempo_new < TEMPO_MIN) {
		tempo_new = TEMPO_MIN;
	}
	config.general_param[GENERAL_PARAM_CMU800_TEMPO] = tempo_new;
	// Melody Sustain
	bool update = false;
	if (config.option_switch & OPTION_SWITCH_CMU800_MELODY_SUSTAIN_INC_1) {
		melody_sustain++;
		config.option_switch &= ~OPTION_SWITCH_CMU800_MELODY_SUSTAIN_INC_1;
		update = true;
	}
	if (config.option_switch & OPTION_SWITCH_CMU800_MELODY_SUSTAIN_DEC_1) {
		melody_sustain--;
		config.option_switch &= ~OPTION_SWITCH_CMU800_MELODY_SUSTAIN_DEC_1;
		update = true;
	}
	if (melody_sustain > 10) {
		melody_sustain = 10;
	}
	else if (melody_sustain < 0) {
		melody_sustain = 0;
	}
	config.general_param[GENERAL_PARAM_CMU800_SUSTAIN] = melody_sustain;
	if(update) {
		int value255 = static_cast<uint8_t>((melody_sustain * 255 + 5) / 10);
		tone[0].SetSustain(value255);
		emu->out_message(_T("CMU-800: Melody Sustain = %d/10"), melody_sustain);
	}
	// Melody Decay
	update = false;
	if (config.option_switch & OPTION_SWITCH_CMU800_MELODY_DECAY_INC_1) {
		melody_decay ++;
		config.option_switch &= ~OPTION_SWITCH_CMU800_MELODY_DECAY_INC_1;
		update = true;
	}
	if (config.option_switch & OPTION_SWITCH_CMU800_MELODY_DECAY_DEC_1) {
		melody_decay --;
		config.option_switch &= ~OPTION_SWITCH_CMU800_MELODY_DECAY_DEC_1;
		update = true;
	}
	if(melody_decay > 10) {
		melody_decay = 10;
	} else if(melody_decay < 0) {
		melody_decay = 0;
	}
	config.general_param[GENERAL_PARAM_CMU800_DECAY1] = melody_decay;
	if(update) {
		int value255 = static_cast<uint8_t>((melody_decay * 255 + 5) / 10);
		tone[0].SetDecay(value255);
		emu->out_message(_T("CMU-800: Melody Decay = %d/10"), melody_decay);
	}
	// Bass Decay
	update = false;
	if (config.option_switch & OPTION_SWITCH_CMU800_BASS_DECAY_INC_1) {
		bass_decay ++;
		config.option_switch &= ~OPTION_SWITCH_CMU800_BASS_DECAY_INC_1;
		update = true;
	}
	if (config.option_switch & OPTION_SWITCH_CMU800_BASS_DECAY_DEC_1) {
		bass_decay --;
		config.option_switch &= ~OPTION_SWITCH_CMU800_BASS_DECAY_DEC_1;
		update = true;
	}
	if(bass_decay > 10) {
		bass_decay = 10;
	} else if(bass_decay < 0) {
		bass_decay = 0;
	}
	config.general_param[GENERAL_PARAM_CMU800_DECAY2] = bass_decay;
	if(update) {
		int value255 = static_cast<uint8_t>((bass_decay * 255 + 5) / 10);
		tone[1].SetDecay(value255);
		emu->out_message(_T("CMU-800: Bass Decay = %d/10"), bass_decay);
	}
	// Chord Decay
	update = false;
	if (config.option_switch & OPTION_SWITCH_CMU800_CHORD_DECAY_INC_1) {
		chord_decay ++;
		config.option_switch &= ~OPTION_SWITCH_CMU800_CHORD_DECAY_INC_1;
		update = true;
	}
	if (config.option_switch & OPTION_SWITCH_CMU800_CHORD_DECAY_DEC_1) {
		chord_decay --;
		config.option_switch &= ~OPTION_SWITCH_CMU800_CHORD_DECAY_DEC_1;
		update = true;
	}
	if(chord_decay > 10) {
		chord_decay = 10;
	} else if(chord_decay < 0) {
		chord_decay = 0;
	}
	config.general_param[GENERAL_PARAM_CMU800_DECAY3] = chord_decay;
	if(update) {
		int value255 = static_cast<uint8_t>((chord_decay * 255 + 5) / 10);
		tone[2].SetDecay(value255);
		tone[3].SetDecay(value255);
		tone[4].SetDecay(value255);
		tone[5].SetDecay(value255);
		emu->out_message(_T("CMU-800: Chord Decay = %d/10"), chord_decay);
	}
}

void CMU800::event_callback(int event_id, int err)
{
	if(event_id == EVENT_TEMPO) {
		if(tempo_freq != tempo_new) {
			tempo_freq = tempo_new;
			cancel_event(this, tempo_id);
			register_event(this, EVENT_TEMPO, 1000000.0 / (tempo_freq * 80 / 100), true, &tempo_id);
			emu->out_message(_T("CMU-800: Tempo = %d"), tempo_freq);
		}
		regs[0x0A] ^= 0xF0;
	}
}

void CMU800::reset_midi()
{
	if(is_reset == true)
	{
		return;
	}
	if(use_midi)
	{
		// GM reset
		d_midi->write_signal(SIG_MIDI_OUT, 0xF0, 0xFF);
		d_midi->write_signal(SIG_MIDI_OUT, 0x7E, 0xFF);
		d_midi->write_signal(SIG_MIDI_OUT, 0x7F, 0xFF);
		d_midi->write_signal(SIG_MIDI_OUT, 0x09, 0xFF);
		d_midi->write_signal(SIG_MIDI_OUT, 0x01, 0xFF);
		d_midi->write_signal(SIG_MIDI_OUT, 0xF7, 0xFF);
		for (int channel = 0; channel < 11; ++channel)
		{
			// all sound off
			d_midi->write_signal(SIG_MIDI_OUT, 0xB0 + channel, 0xFF);
			d_midi->write_signal(SIG_MIDI_OUT, 0x78, 0xFF);
			d_midi->write_signal(SIG_MIDI_OUT, 0, 0xFF);
		}
		// 全チャンネルクラビネットに変更
		d_midi->write_signal(SIG_MIDI_OUT, 0xC0, 0xFF);
		d_midi->write_signal(SIG_MIDI_OUT, 0x07, 0xFF);
		d_midi->write_signal(SIG_MIDI_OUT, 0xC1, 0xFF);
		d_midi->write_signal(SIG_MIDI_OUT, 0x07, 0xFF);
		d_midi->write_signal(SIG_MIDI_OUT, 0xC2, 0xFF);
		d_midi->write_signal(SIG_MIDI_OUT, 0x07, 0xFF);
		d_midi->write_signal(SIG_MIDI_OUT, 0xC3, 0xFF);
		d_midi->write_signal(SIG_MIDI_OUT, 0x07, 0xFF);
		d_midi->write_signal(SIG_MIDI_OUT, 0xC4, 0xFF);
		d_midi->write_signal(SIG_MIDI_OUT, 0x07, 0xFF);
		d_midi->write_signal(SIG_MIDI_OUT, 0xC5, 0xFF);
		d_midi->write_signal(SIG_MIDI_OUT, 0x07, 0xFF);
	}
	is_reset = true;
}

void CMU800::note_on_midi8253(int channel)
{
	if(key_on[channel] == false)
	{
		return;
	}
	note_on_midi(channel);
	key_on[channel] = false;
}

void CMU800::note_on_midi(int channel)
{
	// 8253設定値からcvを求める
	int val = counter[channel];
	int back = -1;
	for(int i = 0; i < array_length(counterTable); ++ i)
	{
		if(val >= counterTable[i])
		{
			back = i;
			break;
		}
	}
	if(back != -1)
	{
		int front = back - 1;
		int x = counterTable[front] - val;
		int y = counterTable[back] - val;
		if(x * x < y * y)
		{
			cv = front;
		}
		else
		{
			cv = back;
		}
	}
	// note on
	uint8_t key = cv + 24;
	// cvを112以内に抑える。下げるときは一度に1オクターブ下げる
	while(key > 112)
	{
		key -= 12;
	}
	if(use_midi)
	{
		d_midi->write_signal(SIG_MIDI_OUT, 0x90 + channel, 0xFF);
		d_midi->write_signal(SIG_MIDI_OUT, key, 0xFF);
		d_midi->write_signal(SIG_MIDI_OUT, 0x7F, 0xFF);
	}
	else
	{
		tone[channel].Set8253(val);
		if(channel == 0)
		{
			tone[channel].SetGate(true);
		}
		else
		{
			tone[channel].Trigger();
		}
	}
	note_on_flag[channel] = 1;
	cv_key[channel] = key;
}

// waveからdataの位置とサイズを取得する
bool CMU800::get_wave_data(uint8_t* wave, uint8_t** data_ptr, uint32_t* data_size)
{
	uint8_t* p = wave;
	wav_header_t* hdr = (wav_header_t*)p;
	if (memcmp(hdr->riff_chunk.id, "RIFF", 4) != 0)
	{
		return false;
	}
	if (memcmp(hdr->wave, "WAVE", 4) != 0)
	{
		return false;
	}
	if (hdr->channels != 1)
	{
		return false;
	}
	if (hdr->format_id != 1)
	{
		return false;
	}
	p += sizeof(wav_header_t);
	while (1)
	{
		wav_chunk_t* chunk = (wav_chunk_t*)p;
		if (memcmp(chunk->id, "data", 4) == 0)
		{
			*data_ptr = p + sizeof(wav_chunk_t);
			*data_size = chunk->size;
			if (hdr->sample_bits == 16)
			{
				// s16 to s8
				int16_t* src = (int16_t*)(*data_ptr);
				uint32_t samples = *data_size / 2;
				for (uint32_t i = 0; i < samples; ++i)
				{
					int16_t s16 = src[i];
					int8_t s8 = (int8_t)(s16 >> 8);
					(*data_ptr)[i] = s8;
				}
				*data_size = samples;
			}
			else
			{
				// u8 to s8
				for (uint32_t i = 0; i < *data_size; ++i)
				{
					(*data_ptr)[i] = ((*data_ptr)[i] - 128);
				}
			}
			return true;
		}
		p += sizeof(wav_chunk_t) + chunk->size;
	}
	return false;
}

void CMU800::write_io8(uint32_t addr, uint32_t data)
{
	unsigned int a = addr & 0xFF;
	unsigned int d = data & 0xFF;
	//char temp[256];
	//sprintf(temp, "Port: %02X, Data: %02X\n", a, d);
	//OutputDebugStringA(temp);

	switch(addr & 0xFF) {
	case 0x90:
	case 0x91:
	case 0x92:
		// 8253-1 counter setting (not use)
		{
			int port_number = addr & 0x0F;
			uint16_t data8 = data & 0xFF;
			counter[port_number] &= (toggle[port_number] == 0 ? 0xFF00 : 0xFF);
			counter[port_number] |= data8 * (toggle[port_number] == 0 ? 1 : 256);
			toggle[port_number] = 1 - toggle[port_number];
			is_reset = false;
			if(toggle[port_number] == 0)
			{
				if(note_on_flag[port_number] == 1)
				{
					if(before_counter[port_number] != counter[port_number])
					{
						key_on[port_number] = true;
						// 0x90h、Volocity 0でのnote off (Volocity 0で消音するとスラー、タイになるらしい)
						if(use_midi)
						{
							d_midi->write_signal(SIG_MIDI_OUT, 0x90 + port_number, 0xFF);
							d_midi->write_signal(SIG_MIDI_OUT, cv_key[port_number], 0xFF);
							d_midi->write_signal(SIG_MIDI_OUT, 0, 0xFF);
						}
					}
				}
				note_on_midi8253(addr & 0xF);
				before_counter[port_number] = counter[port_number];
			}
			break;
		}
	case 0x93:
		// 8253-1 setting
		break;
	case 0x94:
	case 0x95:
	case 0x96:
		// 8253-2 counter setting (not use)
		{
			int port_number = (addr & 0x0F) - 1;
			uint16_t data8 = data & 0xFF;
			counter[port_number] &= (toggle[port_number] == 0 ? 0xFF00 : 0xFF);
			counter[port_number] |= data8 * (toggle[port_number] == 0 ? 1 : 256);
			toggle[port_number] = 1 - toggle[port_number];
			is_reset = false;
			if(toggle[port_number] == 0)
			{
				if(note_on_flag[port_number] == 1)
				{
					if(before_counter[port_number] != counter[port_number])
					{
						key_on[port_number] = true;
						// 0x90h、Volocity 0でのnote off (Volocity 0で消音するとスラー、タイになるらしい)
						if(use_midi)
						{
							d_midi->write_signal(SIG_MIDI_OUT, 0x90 + port_number, 0xFF);
							d_midi->write_signal(SIG_MIDI_OUT, cv_key[port_number], 0xFF);
							d_midi->write_signal(SIG_MIDI_OUT, 0, 0xFF);
						}
					}
				}
				note_on_midi8253((addr & 0xF) - 1);
				before_counter[port_number] = counter[port_number];
			}
			break;
		}
		break;
	case 0x97:
		// 8253-2 setting
		break;
	case 0x98:
		// 8255 Port A
		// b0-5 CV-OUT data (not use built-in sound)
		// b6 TUNE-GATE (unknown)
		// b7 GATE-DATA (0 = play, 1 = stop)
		note_on = ((data & 0x80) == 0);
		if(note_on == true)
		{
			cv = data & 0x3F;
		}
		is_reset = false;
		break;
	case 0x99:
		// 8255 Port B
		// built-in rhythm, 1 -> 0 = play the drum
		// b7 BD (bass drum)
		// b6 SD (snare drum)
		// b5 LT (low tom)
		// b4 HT (high tom)
		// b3 CY (cymbal)
		// b2 OH (open hihat)
		// b1 CH (close hihat)
		// b0 Reserve (not use)
		{
			uint8_t bitMask = 0x01; //0b00000010;
			for(int32_t i = 0; i < 8; ++ i)
			{
				uint8_t beforeBit = before_rhythm & bitMask;
				uint8_t bit = data & bitMask;
				if((beforeBit != 0) && (bit == 0))
				{
					// sound a rhythm
					if(use_midi)
					{
						if(i > 0) {
							d_midi->write_signal(SIG_MIDI_OUT, 0x99, 0xFF);
							d_midi->write_signal(SIG_MIDI_OUT, rhythm_table[i], 0xFF);
							d_midi->write_signal(SIG_MIDI_OUT, 0x7F, 0xFF);
						}
					}
					else
					{
						rhythm[i].Trigger();
					}
				}
				bitMask <<= 1;
			}
			before_rhythm = data & 0xFF;
			is_reset = false;
			break;
		}
	case 0x9A:
		// 8255 Port C
		// b0 (1 -> 0 = Setup complete, note on or note off)
		// b1-3 = channel (0-7)
		{
			int channel = ((data >> 1) & 7);
			if(((before_tone[channel] & 1) == 1) && ((data & 1) == 0))
			{
				if(note_on == false && note_on_flag[channel] == 1)
				{
					// note off
					if(use_midi)
					{
						d_midi->write_signal(SIG_MIDI_OUT, 0x80 + channel, 0xFF);
						d_midi->write_signal(SIG_MIDI_OUT, cv_key[channel], 0xFF);
						d_midi->write_signal(SIG_MIDI_OUT, 0x7F, 0xFF);
					}
					else if(channel == 0)
					{
						tone[channel].SetGate(false);
					}
					cv_key[channel] = 0;
					note_on_flag[channel] = 0;
				}
				else if(note_on == true && note_on_flag[channel] == 0)
				{
					if(cv == 0)
					{
						// bugfireさんのプレイヤーは既に8253設定済みでcvが0なのですぐに音を鳴らす
						note_on_midi(channel);
					}
					else
					{
						// CMU-800シーケンサーはまだ8253の設定が終わっていないので8253設定時に音を鳴らすためのフラグをセット
						key_on[channel] = true;
					}
				}
			}
			before_tone[channel] = data & 0xFF;
			is_reset = false;
			break;
		}
	case 0x9B:
		// 8255 setting
		if(!(data & 0x80)) {
			uint32_t val = regs[0x0A];
			int bit = (data >> 1) & 7;
			if(data & 1) {
				val |= 1 << bit;
			} else {
				val &= ~(1 << bit);
			}
			write_io8(0x9A, val);
		}
		break;
	case 0x9C:
		// dummy port (Nothing to do.)
		break;
	}
	if((addr & 0xFF) == 0x9A) {
		regs[addr & 0x0F] &= 0xF0;
		regs[addr & 0x0F] |= data & 0x0F;
	} else {
		regs[addr & 0x0F] = data & 0xFF;
	}
}

uint32_t CMU800::read_io8(uint32_t addr)
{
	switch(addr & 0xFF) {
	case 0x98:
	case 0x99:
	case 0x9A:
		// 8255 Port
		return regs[addr & 0x0F];
	}
	return 0;
}

void CMU800::adjust_tempo(int delta)
{
	tempo_new += delta;
	if(tempo_new > TEMPO_MAX) {
		tempo_new = TEMPO_MAX;
	} else if(tempo_new < TEMPO_MIN) {
		tempo_new = TEMPO_MIN;
	}
	config.general_param[GENERAL_PARAM_CMU800_TEMPO] = tempo_new;
}

void CMU800::set_sample_rate(uint32_t sample_rate)
{
	for(int i = 0; i < 6; ++ i) {
		tone[i].SetSampleRate(sample_rate);
	}
	for(int i = 0; i < 8; ++ i) {
		rhythm[i].SetSampleRate(sample_rate);
	}
}

void CMU800::mix(int32_t* buffer, int cnt)
{
	if (use_midi)
	{
		return;
	}
	for (int i = 0; i < cnt; ++i)
	{
		int32_t cmu800MixedL = 0;
		int32_t cmu800MixedR = 0;
		// Tone
		// 音量ch 0: Melody
		// 音量ch 1: Bass
		// 音量ch 2: Chord
		for (int channel = 0; channel < 6; ++channel)
		{
			const bool shouldMix =
				(channel == 0) ?
				tone[channel].IsPlaying() :
				(note_on_flag[channel] == 1);
			if (shouldMix)
			{
				int volumeChannel;
				if (channel == 0)
				{
					// Melody
					volumeChannel = 0;
				}
				else if (channel == 1)
				{
					// Bass
					volumeChannel = 1;
				}
				else
				{
					// Chord 1～4
					volumeChannel = 2;
				}
				int32_t sample =
					tone[channel].GetDataWithVolume(32767);
				sample *= 16; // Tone gain up
				cmu800MixedL +=
					apply_volume(sample, volume_l[volumeChannel]);
				cmu800MixedR +=
					apply_volume(sample, volume_r[volumeChannel]);
			}
		}
		// 音量ch 3: Rhythm
		int32_t rhythmMixed = 0;
		for (int channel = 0; channel < 8; ++channel)
		{
			rhythmMixed += rhythm[channel].GetData(32767);
		}
		rhythmMixed *= 256; // Rhythm gain up
		cmu800MixedL += apply_volume(rhythmMixed, volume_l[3]);
		cmu800MixedR += apply_volume(rhythmMixed, volume_r[3]);
		// 音量ch 4: Master
		cmu800MixedL = apply_volume(cmu800MixedL, volume_l[4]);
		cmu800MixedR = apply_volume(cmu800MixedR, volume_r[4]);
		*buffer++ += cmu800MixedL;
		*buffer++ += cmu800MixedR;
	}
}

// 音量ch 0: Merody
//        1: Bass
//        2: Chord
//        3: Rhythm
//        4: Master
void CMU800::set_volume(int ch, int decibel_l, int decibel_r)
{
	volume_l[ch] = (decibel_l <= -40) ? 0 : decibel_to_volume(decibel_l);
	volume_r[ch] = (decibel_r <= -40) ? 0 : decibel_to_volume(decibel_r);
}

#define STATE_VERSION	0x000000003

bool CMU800::process_state(FILEIO* state_fio, bool loading)
{
	if(!state_fio->StateCheckUint32(STATE_VERSION)) {
		return false;
	}
	if(!state_fio->StateCheckInt32(this_device_id)) {
		return false;
	}
	// save/load status
	state_fio->StateArray(regs, sizeof(regs), 1);
	state_fio->StateArray(toggle, sizeof(toggle), 1);
	state_fio->StateArray(counter, sizeof(counter), 1);
	state_fio->StateValue(cv);
	state_fio->StateValue(note_on);
	state_fio->StateArray(cv_key, sizeof(cv_key), 1);
	state_fio->StateArray(note_on_flag, sizeof(note_on_flag), 1);
	state_fio->StateArray(before_tone, sizeof(before_tone), 1);
	state_fio->StateArray(key_on, sizeof(key_on), 1);
	state_fio->StateValue(before_rhythm);
	state_fio->StateValue(is_reset);
//	state_fio->StateValue(tempo_freq);
	tempo_freq = 0;
	state_fio->StateValue(tempo_new);
	state_fio->StateValue(tempo_id);
	state_fio->StateValue(melody_sustain);
	state_fio->StateValue(melody_decay);
	state_fio->StateValue(bass_decay);
	state_fio->StateValue(chord_decay);
	return true;
}
