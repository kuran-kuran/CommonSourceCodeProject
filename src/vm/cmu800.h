/*
	Skelton for retropc emulator

	Author : kuran_kuran
	Date   : 2024.05.25-

	[ AMDEK/RolandDG CMU-800 (MIDI) ]
*/

#ifndef _CMU800_H_
#define _CMU800_H_

#include "vm.h"
#include "../emu.h"
#include "device.h"
#include "cmu800tone.h"
#include "cmu800rhythm.h"

class CMU800 : public DEVICE
{
private:
	DEVICE* d_midi;
	Cmu800Tone tone[6];
	Cmu800Rhythm rhythm[8];
	uint8_t regs[16];
	uint8_t toggle[6];
	uint16_t counter[6];
	uint16_t before_counter[6];
	uint8_t cv;
	bool note_on;
	uint8_t cv_key[8];
	uint8_t note_on_flag[8];
	uint8_t before_tone[8];
	uint8_t before_rhythm;
	bool is_reset;
	int tempo_freq, tempo_new, tempo_id;
	int melody_sustain, melody_decay, bass_decay, chord_decay;
	bool key_on[8];
	bool use_midi;
	bool enable_portbase10_mode;
	uint8_t* b7BD_wave;
	int8_t* b7BD_data;
	uint8_t* b6SD_wave;
	int8_t* b6SD_data;
	uint8_t* b5LT_wave;
	int8_t* b5LT_data;
	uint8_t* b4HT_wave;
	int8_t* b4HT_data;
	uint8_t* b3CY_wave;
	int8_t* b3CY_data;
	uint8_t* b2OH_wave;
	int8_t* b2OH_data;
	uint8_t* b1CH_wave;
	int8_t* b1CH_data;
	uint8_t* b0User_wave;
	int8_t* b0User_data;
	int volume_l[5];
	int volume_r[5];
	void reset_midi();
	void note_on_midi8253(int channel);
	void note_on_midi(int channel);
	bool get_wave_data(uint8_t* wave, uint8_t** data_ptr, uint32_t* data_size);
public:
	CMU800(VM_TEMPLATE* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu)
	{
		set_device_name(_T("CMU-800 (MIDI)"));
	}
	~CMU800() {}
	
	// common functions
	void reset();
	void initialize();
	void release();
	void write_io8(uint32_t addr, uint32_t data);
	uint32_t read_io8(uint32_t addr);
	void event_callback(int event_id, int err);
	void mix(int32_t* buffer, int cnt);
	void set_volume(int ch, int decibel_l, int decibel_r);
	void update_config();
	bool process_state(FILEIO* state_fio, bool loading);

	// unique function
	//void initialize_sound(int rate, int volume);
	void set_context_midi(DEVICE* device)
	{
		d_midi = device;
	}
	void adjust_tempo(int delta);
	void set_sample_rate(uint32_t sample_rate);
	void enable_midi(bool enabled);
	void enable_portbase10(bool enabled);
};

#endif
