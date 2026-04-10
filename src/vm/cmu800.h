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

class CMU800 : public DEVICE
{
private:
	DEVICE* d_midi;
	uint8_t regs[16];
	uint8_t toggle[6];
	uint16_t counter[6];
	uint8_t cv;
	bool note_on;
	uint8_t cv_key[8];
	uint8_t note_on_flag[8];
	uint8_t before_tone[8];
	uint8_t before_rythm;
	bool is_reset;
	int tempo_freq, tempo_new, tempo_id;
	void reset_midi();
public:
	CMU800(VM_TEMPLATE* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu)
	{
		set_device_name(_T("CMU-800 (MIDI)"));
	}
	~CMU800() {}
	
	// common functions
	void initialize();
	void release();
	void write_io8(uint32_t addr, uint32_t data);
	uint32_t read_io8(uint32_t addr);
	void event_callback(int event_id, int err);
	void update_config();
	bool process_state(FILEIO* state_fio, bool loading);
	
	// unique function
	void set_context_midi(DEVICE* device)
	{
		d_midi = device;
	}
};

#endif
