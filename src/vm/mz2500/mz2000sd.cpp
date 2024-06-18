/*
SHARP MZ-2200 Emulator 'EmuZ-2200'

Author : kuran_kuran
Date   : 2024.05.28-

[ MZ2000_SD (SD Storage) ]
*/

#include "mz2000sd.h"

void MZ2000_SD::initialize()
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
	initA0Port = false;
	initA2Port = false;
}

void MZ2000_SD::release()
{
}

void MZ2000_SD::reset()
{
}

void MZ2000_SD::write_io8(uint32_t addr, uint32_t data)
{
	unsigned int write_data = 0;
	unsigned int write_bit = 0;
	switch(addr & 0xff) {
	case 0xa0:
		if(initA0Port == false) {
			initA0Port = true;
			return;
		}
		// send data (low 4bit) 
		d_mz80ksd->digitalWrite(PA0PIN, data & 1);
		d_mz80ksd->digitalWrite(PA1PIN, (data >> 1) & 1);
		d_mz80ksd->digitalWrite(PA2PIN, (data >> 2) & 1);
		d_mz80ksd->digitalWrite(PA3PIN, (data >> 3) & 1);
		*d_mz80ksd->buffer = '0'; ++ d_mz80ksd->buffer;
		break;
	case 0xa2:
		if(initA2Port == false) {
			initA2Port = true;
			return;
		}
		// b2 FLG handshake
		d_mz80ksd->setFlg((data >> 2) & 1);
		*d_mz80ksd->buffer = '1'; ++ d_mz80ksd->buffer;
		break;
	case 0xa3:
		// 8255 setting or set bit
		if(data < 128) {
			// set bit. b2 FLG handshake
			if(((data >> 1) & 7) == 2) {
				d_mz80ksd->setFlg(data & 1);
				*d_mz80ksd->buffer = '2'; ++ d_mz80ksd->buffer;
				*d_mz80ksd->buffer = '['; ++ d_mz80ksd->buffer;
				*d_mz80ksd->buffer = (data & 1) ? '1' : '0'; ++ d_mz80ksd->buffer;
				*d_mz80ksd->buffer = ']'; ++ d_mz80ksd->buffer;
			}
		} else {
			// 8255 setting
			initA0Port = false;
			initA2Port = false;
		}
		break;
	case 0xf8:
		address = (address & 0x00ff) | (data << 8);
		break;
	case 0xf9:
		address = (address & 0xff00) | (data << 0);
		break;
	}
}

uint32_t MZ2000_SD::read_io8(uint32_t addr)
{
	uint32_t result = 0xff;
	switch(addr & 0xff) {
	case 0xa1:
		// receive data (8bit) 
		result = d_mz80ksd->digitalRead(PB0PIN);
		result |= (d_mz80ksd->digitalRead(PB1PIN) << 1);
		result |= (d_mz80ksd->digitalRead(PB2PIN) << 2);
		result |= (d_mz80ksd->digitalRead(PB3PIN) << 3);
		result |= (d_mz80ksd->digitalRead(PB4PIN) << 4);
		result |= (d_mz80ksd->digitalRead(PB5PIN) << 5);
		result |= (d_mz80ksd->digitalRead(PB6PIN) << 6);
		result |= (d_mz80ksd->digitalRead(PB7PIN) << 7);
		*d_mz80ksd->buffer = '3'; ++ d_mz80ksd->buffer;
		break;
	case 0xa2:
		// b7 CHK handshake
		result = 0;
		result |= d_mz80ksd->getChk() << 7;
		*d_mz80ksd->buffer = '4'; ++ d_mz80ksd->buffer;
		*d_mz80ksd->buffer = '['; ++ d_mz80ksd->buffer;
		*d_mz80ksd->buffer = result != 0 ? '1' : '0'; ++ d_mz80ksd->buffer;
		*d_mz80ksd->buffer = ']'; ++ d_mz80ksd->buffer;
		break;
	case 0xf9:
		if(address >= 0x8000) {
			return boot_rom[address & 0x7fff];
		}
	}
	return result;
}

#define STATE_VERSION	1

bool MZ2000_SD::process_state(FILEIO* state_fio, bool loading)
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
