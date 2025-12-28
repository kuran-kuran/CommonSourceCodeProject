/*
	Hudson HuX-883 Emulator 'eHuX-883'

	Author : Takeda.Toshiya
	Date   : 2025.11.03-

	[ memory ]
*/

#include "memory.h"
#include "../i8255.h"
#include "../tms9918a.h"

void MEMORY::initialize()
{
	memset(ram, 0x00, sizeof(ram));
	memset(rom, 0xff, sizeof(rom));
	
	// load rom images
	FILEIO* fio = new FILEIO();
	if(fio->Fopen(create_local_path(_T("USER.ROM")), FILEIO_READ_BINARY)) {
		fio->Fread(rom, 0x2000, 1);
		fio->Fclose();
	}
	if(fio->Fopen(create_local_path(_T("BASIC.ROM")), FILEIO_READ_BINARY)) {
		fio->Fread(rom + 0x2000, 0x2000, 1);
		fio->Fclose();
	}
	delete fio;
}

void MEMORY::write_data8(uint32_t addr, uint32_t data)
{
	addr &= 0xffff;
	if(addr < 0x8000) {
		ram[addr] = data;
	} else if(addr < 0xa000) {
		d_vdp->write_io8(addr, data);
	} else if(addr < 0xa800) {
		d_pio->write_io8(addr, data);
	}
}

uint32_t MEMORY::read_data8(uint32_t addr)
{
	addr &= 0xffff;
	if(addr < 0x8000) {
		return ram[addr];
	} else if(addr < 0xa000) {
		return d_vdp->read_io8(addr);
	} else if(addr < 0xa800) {
		const int table[8][8] = {
			0xc0, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,	// @ABVDEFG
			0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,	// HIJKLMNO
			0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,	// PQRSTUVW
			0x58, 0x59, 0x5a, 0xdb, 0xdc, 0xdd, 0xde, 0xe2,	// XYZ[\]^_
			0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,	// 01234567
			0x38, 0x39, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,	// 89:;,-./
			0x26, 0x28, 0x27, 0x25, 0x00, 0x00, 0x00, 0x00,	// UP,DOWN,RIGHT,LEFT
			0x20, 0x10, 0x11, 0x09, 0x1b, 0x0d, 0x2e, 0x24,	// SPACE,SHIFT,CTRL,TAB,ESC,RET,DEL,CLS
		};
		if((addr & 3) == 1) {
			const uint8_t *key_stat = emu->get_key_buffer();
			int data = 0xff;
			for(int i = 0; i < 8; i++) {
				if(key_stat[table[(addr >> 2) & 7][i]]) {
					data ^= 1 << i;
				}
			}
			d_pio->write_signal(SIG_I8255_PORT_B, data, 0xff);
		}
		return d_pio->read_io8(addr);
	} else if(addr < 0xc000) {
		// user i/o
	} else {
		return rom[addr & 0x3fff];
	}
	return 0xff;
}

#define STATE_VERSION	1

bool MEMORY::process_state(FILEIO* state_fio, bool loading)
{
	if(!state_fio->StateCheckUint32(STATE_VERSION)) {
		return false;
	}
	if(!state_fio->StateCheckInt32(this_device_id)) {
		return false;
	}
	state_fio->StateArray(ram, sizeof(ram), 1);
	return true;
}

