/*
	Hudson HuX-883 Emulator 'eHuX-883'

	Author : Takeda.Toshiya
	Date   : 2025.11.03-

	[ memory ]
*/

#ifndef _MEMORY_H_
#define _MEMORY_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

#define SIG_KEYBOARD_COLUMN	0

class MEMORY : public DEVICE
{
private:
	DEVICE *d_pio;
	DEVICE *d_vdp;
	
	uint8_t ram[0x8000];
	uint8_t rom[0x4000];
	
public:
	MEMORY(VM_TEMPLATE* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu)
	{
		set_device_name(_T("Memory Bus"));
	}
	~MEMORY() {}
	
	// common functions
	void initialize();
	void write_data8(uint32_t addr, uint32_t data);
	uint32_t read_data8(uint32_t addr);
	bool process_state(FILEIO* state_fio, bool loading);
	
	// unique functions
	void set_context_pio(DEVICE* device)
	{
		d_pio = device;
	}
	void set_context_vdp(DEVICE* device)
	{
		d_vdp = device;
	}
};

#endif

