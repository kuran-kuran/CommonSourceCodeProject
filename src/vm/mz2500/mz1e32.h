/*
	SHARP MZ-2500 Emulator 'EmuZ-2500'

	Author : youkan700
	Date   : 2024.04.30 -

	[ MZ-1E32 (Parallel I/F board: Emulate Kanji ROM part only) ]
*/

#ifndef _MZ1E32_H_
#define _MZ1E32_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

class MZ1E32 : public DEVICE
{
private:
	DEVICE *d_host;
	
	// rom file
	uint8_t *rom_buffer;
	uint32_t rom_address, rom_size;
	
public:
	MZ1E32(VM_TEMPLATE* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu)
	{
		set_device_name(_T("MZ-1E32 (Parallel I/F)"));
	}
	~MZ1E32() {}
	
	// common functions
	void initialize();
	void release();
	void reset();
	void write_io8(uint32_t addr, uint32_t data);
	uint32_t read_io8(uint32_t addr);
	bool process_state(FILEIO* state_fio, bool loading);
	
	// unique function
	void set_context_host(DEVICE* device)
	{
		d_host = device;
	}
};

#endif

