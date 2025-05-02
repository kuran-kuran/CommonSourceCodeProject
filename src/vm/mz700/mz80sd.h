/*
SHARP MZ-700 Emulator 'EmuZ-700'

Author : kuran_kuran
Date   : 2024.08.069-

[ MZ80_SD (SD Storage) ]
*/

#ifndef _MZ80SD_H_
#define _MZ80SD_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"
#include "../mz80k_sd.h"

class MZ80_SD : public DEVICE
{
private:
	uint64_t file_position;
	MZ80K_SD* d_mz80ksd;
	bool initD8Port;
	bool initDAPort;

public:
	MZ80_SD(VM_TEMPLATE* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu)
	{
		d_mz80ksd = NULL;
		set_device_name(_T("MZ80_SD (SD Card reader/writer)"));
	}

	// common functions
	void initialize();
	void release();
	void reset();
	void write_io8(uint32_t addr, uint32_t data);
	uint32_t read_io8(uint32_t addr);
	bool process_state(FILEIO* state_fio, bool loading);

	// unique function
	void set_context_mz80k_sd(DEVICE* device)
	{
		d_mz80ksd = (MZ80K_SD*)device;
	}
};

#endif
