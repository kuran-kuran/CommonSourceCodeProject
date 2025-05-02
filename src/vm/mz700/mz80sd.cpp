/*
SHARP MZ-700 Emulator 'EmuZ-700'

Author : kuran_kuran
Date   : 2024.08.06-

[ MZ80_SD (SD Storage) ]
*/

#include "mz80sd.h"

void MZ80_SD::initialize()
{
	file_position = 0;
	initD8Port = false;
	initDAPort = false;
}

void MZ80_SD::release()
{
}

void MZ80_SD::reset()
{
	if(d_mz80ksd != NULL)
	{
		d_mz80ksd->reset();
	}
}

void MZ80_SD::write_io8(uint32_t addr, uint32_t data)
{
	unsigned int write_data = 0;
	unsigned int write_bit = 0;
	switch(addr & 0xff) {
	case 0xd8:
		if(initD8Port == false) {
			initD8Port = true;
			return;
		}
		// send data (low 4bit) 
		d_mz80ksd->digitalWrite(PA0PIN, data & 1);
		d_mz80ksd->digitalWrite(PA1PIN, (data >> 1) & 1);
		d_mz80ksd->digitalWrite(PA2PIN, (data >> 2) & 1);
		d_mz80ksd->digitalWrite(PA3PIN, (data >> 3) & 1);
		break;
	case 0xda:
		if(initDAPort == false) {
			initDAPort = true;
			return;
		}
		// b2 FLG handshake
		d_mz80ksd->setFlg((data >> 2) & 1);
		break;
	case 0xdb:
		// 8255 setting or set bit
		if(data < 128) {
			// set bit. b2 FLG handshake
			if(((data >> 1) & 7) == 2) {
				d_mz80ksd->setFlg(data & 1);
			}
		} else {
			// 8255 setting
			initD8Port = false;
			initDAPort = false;
		}
		break;
	}
}

uint32_t MZ80_SD::read_io8(uint32_t addr)
{
	uint32_t result = 0xff;
	switch(addr & 0xff) {
	case 0xd8:
		result = 0;
	case 0xd9:
		// receive data (8bit) 
		result = d_mz80ksd->digitalRead(PB0PIN);
		result |= (d_mz80ksd->digitalRead(PB1PIN) << 1);
		result |= (d_mz80ksd->digitalRead(PB2PIN) << 2);
		result |= (d_mz80ksd->digitalRead(PB3PIN) << 3);
		result |= (d_mz80ksd->digitalRead(PB4PIN) << 4);
		result |= (d_mz80ksd->digitalRead(PB5PIN) << 5);
		result |= (d_mz80ksd->digitalRead(PB6PIN) << 6);
		result |= (d_mz80ksd->digitalRead(PB7PIN) << 7);
		break;
	case 0xda:
		// b7 CHK handshake
		result = 0;
		result |= d_mz80ksd->getChk() << 7;
		break;
	}
	return result;
}

#define STATE_VERSION	1

bool MZ80_SD::process_state(FILEIO* state_fio, bool loading)
{
	if(!state_fio->StateCheckUint32(STATE_VERSION)) {
		return false;
	}
	if(!state_fio->StateCheckInt32(this_device_id)) {
		return false;
	}
	state_fio->StateValue(file_position);
	return true;
}
