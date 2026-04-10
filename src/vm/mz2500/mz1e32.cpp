/*
	SHARP MZ-2500 Emulator 'EmuZ-2500'

	Author : youkan700
	Date   : 2024.04.30 -

	[ MZ-1E32 (Parallel I/F board: Emulate Kanji ROM part only) ]
*/

#include "mz1e32.h"

void MZ1E32::initialize()
{
	// rom file
	FILEIO* fio = new FILEIO();
	if(fio->Fopen(create_local_path(_T("MZ1E32.ROM" )), FILEIO_READ_BINARY) ||
	   fio->Fopen(create_local_path(_T("MZ-1E32.ROM")), FILEIO_READ_BINARY)) {
		fio->Fseek(0, FILEIO_SEEK_END);
		if((rom_size = fio->Ftell()) == 0xA0000) {
			rom_buffer = (uint8_t*)malloc(rom_size);
			fio->Fseek(0, FILEIO_SEEK_SET);
			fio->Fread(rom_buffer, rom_size, 1);
		}
		fio->Fclose();
	} else {
		rom_size = 0;
		rom_buffer = (uint8_t*)malloc(1);
	}
	delete fio;
	rom_address = 0;
}

void MZ1E32::release()
{
	if(rom_buffer != NULL) {
		free(rom_buffer);
	}
}

void MZ1E32::reset()
{
	rom_address = 0;
}

void MZ1E32::write_io8(uint32_t addr, uint32_t data)
{
	if ((addr & 0xff) == 0x9A) {
		rom_address = ((rom_address << 8) | data) & 0x000FFFFF;
	}
}

uint32_t MZ1E32::read_io8(uint32_t addr)
{
	if ((addr & 0xff) == 0x9B) {
		// rom file
		if(rom_address < rom_size) {
			return rom_buffer[rom_address++];
		}
	}
	return 0xff;
}

#define STATE_VERSION	1

bool MZ1E32::process_state(FILEIO* state_fio, bool loading)
{
	if(!state_fio->StateCheckUint32(STATE_VERSION)) {
		return false;
	}
	if(!state_fio->StateCheckInt32(this_device_id)) {
		return false;
	}
	state_fio->StateValue(rom_address);
	return true;
}

