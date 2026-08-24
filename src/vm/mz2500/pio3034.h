/*
	SHARP MZ-80B Emulator 'EmuZ-80B'
	SHARP MZ-2200 Emulator 'EmuZ-2200'

	Author : Takeda.Toshiya
	Date   : 2025.12.08 -

	[ PIO-3034 ]
*/

#ifndef _PIO3034_H_
#define _PIO3034_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

class PIO3034 : public DEVICE
{
private:
	uint8_t *data_buffer;
	uint32_t data_addr;
	bool modified;
	
public:
	PIO3034(VM_TEMPLATE* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu)
	{
		set_device_name(_T("PIO-3034"));
	}
	~PIO3034() {}
	
	// common functions
	void initialize();
	void release();
	void reset();
	void write_io8(uint32_t addr, uint32_t data);
	uint32_t read_io8(uint32_t addr);
	bool process_state(FILEIO* state_fio, bool loading);
};

#endif

