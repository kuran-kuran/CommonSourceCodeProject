/*
SHARP MZ-1500 Emulator 'EmuZ-1500'

Author : kuran_kuran
Date   : 2024.07.09-

[ MZ1500_SD (SD Storage) ]
*/

#ifndef _MZ1500SD_H_
#define _MZ1500SD_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"
#include "../mz80k_sd.h"

class MZ1500_SD : public DEVICE
{
private:
	uint64_t file_position;
	MZ80K_SD* d_mz80ksd;
	bool initA0Port;
	bool initA2Port;

public:
	MZ1500_SD(VM_TEMPLATE* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu)
	{
		d_mz80ksd = NULL;
		set_device_name(_T("MZ1500_SD (SD Card reader/writer)"));
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
