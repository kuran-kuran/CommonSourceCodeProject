/*
SHARP MZ-2200 Emulator 'EmuZ-2200'

Author : kuran_kuran
Date   : 2024.05.28-

[ MZ2000_SD (SD Storage) ]
*/

#include "mz2000sd.h"

void MZ2000SD::initialize()
{
	memset(boot_rom, 0xff, sizeof(boot_rom));
	FILEIO* fio = new FILEIO();
	if(fio->Fopen(create_local_path(_T("MZ2000SD.ROM")), FILEIO_READ_BINARY)) {
		fio->Fread(boot_rom, sizeof(boot_rom), 1);
		fio->Fclose();
	}
	delete fio;
	address = 0;
	read_write_flag = 0;
	file_position = 0;
	chk = false; // Z80‘¤‚©‚çŒ©‚½chk
	flg = false; // Z80‘¤‚©‚çŒ©‚½flg
}

void MZ2000SD::release()
{
}

void MZ2000SD::write_io8(uint32_t addr, uint32_t data)
{
	switch(addr & 0xff) {
	case 0xa2:
		// b7 CHK handshake
		break;
	case 0xa3:
		// 8255 setting
		break;
	case 0xf8:
		address = (address & 0x00ff) | (data << 8);
		break;
	case 0xf9:
		address = (address & 0xff00) | (data << 0);
		break;
	}
}

uint32_t MZ2000SD::read_io8(uint32_t addr)
{
	if(address >= 0x8000) {
		switch(addr & 0xff) {
		case 0xa0:
			// send data (low 4bit) 
			break;
		case 0xa1:
			// receive data (8bit) 
			break;
		case 0xa2:
			// b2 FLG handshake
			break;
		case 0xf9:
			return boot_rom[address & 0x7fff];
		}
	}
	return 0xff;
}

#define STATE_VERSION	1

bool MZ2000SD::process_state(FILEIO* state_fio, bool loading)
{
	if(!state_fio->StateCheckUint32(STATE_VERSION)) {
		return false;
	}
	if(!state_fio->StateCheckInt32(this_device_id)) {
		return false;
	}
	state_fio->StateValue(address);
	state_fio->StateValue(read_write_flag);
	state_fio->StateValue(file_position);
	return true;
}
