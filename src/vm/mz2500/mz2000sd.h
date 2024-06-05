/*
SHARP MZ-2200 Emulator 'EmuZ-2200'

Author : kuran_kuran
Date   : 2024.05.28-

[ MZ2000_SD (SD Storage) ]
*/

#ifndef _MZ2000SD_H_
#define _MZ2000SD_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

class MZ2000SD : public DEVICE
{
private:
	uint8_t boot_rom[0x8000];
	uint16_t address;
	uint8_t read_write_flag; // 0: not busy, 1: reading, 2: writeing
	uint64_t file_position;
	bool chk;
	bool flg;

public:
	MZ2000SD(VM_TEMPLATE* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu)
	{
		set_device_name(_T("MZ2000_SD (SD Card reader/writer)"));
	}

	// common functions
	void initialize();
	void release();
	void write_io8(uint32_t addr, uint32_t data);
	uint32_t read_io8(uint32_t addr);
	bool process_state(FILEIO* state_fio, bool loading);
};

#endif