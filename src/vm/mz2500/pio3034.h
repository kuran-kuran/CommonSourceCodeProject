/*
	SHARP MZ-80B Emulator 'EmuZ-80B'
	SHARP MZ-2200 Emulator 'EmuZ-2200'

	Author : Takeda.Toshiya
	Date   : 2013.03.17-

	Author : kuran_kuran
	Date   : 2025.11.30-

	[ PIO-3034 EMM ]
*/

#ifndef _PIO3034_H_
#define _PIO3034_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

class PIO3034 : public DEVICE
{
private:
	static size_t size_tbl[USE_EMM_SIZE];
	uint8_t base_port_number;
	uint8_t ram[16 * 1024 * 1024];
	uint32_t address;
	uint32_t crc32;
	
public:
	PIO3034(VM_TEMPLATE* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu)
	{
		base_port_number = 0xa0;
		address = 0;
		set_device_name(_T("PIO-3034 EMM"));
	}
	~PIO3034() {}
	
	// common functions
	void initialize();
	void release();
	void reset();
	void write_io8(uint32_t addr, uint32_t data);
	uint32_t read_io8(uint32_t addr);
	bool process_state(FILEIO* state_fio, bool loading);

	// unique function
	void set_base_port_number(uint32_t port_number)
	{
		base_port_number = port_number;
	}
};

#endif
