/*
	SHARP MZ-80B Emulator 'EmuZ-80B'
	SHARP MZ-2200 Emulator 'EmuZ-2200'

	Author : kuran_kuran
	Date   : 2024.05.25-

	[ AMDEK/RolandDG CMU-800 (MIDI) ]
*/

#ifndef _CMU800_H_
#define _CMU800_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"
#include "../event.h"

class CMU800 : public DEVICE
{
private:
	DEVICE* d_midi;
	EVENT* d_event;
	uint32_t base_clock;
	uint8_t toggle[6];
	uint16_t counter[6];
	uint8_t cv;
	bool note_on;
	uint8_t cv_key[8];
	uint8_t note_on_flag[8];
	uint8_t before_tone[8];
	uint8_t before_rythm;
	bool is_reset;
	static uint8_t rythm_table[7];
public:
	CMU800(VM_TEMPLATE* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu)
	{
		set_device_name(_T("CMU-800 (MIDI)"));
	}
	~CMU800() {}
	
	// common functions
	void initialize();
	void release();
	void reset_midi();
	void write_io8(uint32_t addr, uint32_t data);
	uint32_t read_io8(uint32_t addr);
	bool process_state(FILEIO* state_fio, bool loading);

	// unique function
	void set_context_midi(DEVICE* device)
	{
		d_midi = device;
	}
	void set_context_event(EVENT* event)
	{
		d_event = event;
	}
};

#endif
