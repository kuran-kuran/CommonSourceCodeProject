/*
	SHARP MZ-80B Emulator 'EmuZ-80B'
	SHARP MZ-2200 Emulator 'EmuZ-2200'

	Author : Takeda.Toshiya
	Date   : 2025.12.08 -

	[ PIO-3034 ]
*/

#include "pio3034.h"

#define DATA_SIZE	0x1000000
#define ADDR_MASK	(DATA_SIZE - 1)

void PIO3034::initialize()
{
	// init memory
	data_buffer = (uint8_t *)malloc(DATA_SIZE);
	memset(data_buffer, 0xff, DATA_SIZE);
	modified = false;
	
	// load emm image
	FILEIO* fio = new FILEIO();
	if(fio->Fopen(create_local_path(_T("PIO3034.ROM")), FILEIO_READ_BINARY)) {
		fio->Fread(data_buffer, DATA_SIZE, 1);
		fio->Fclose();
	} else if(fio->Fopen(create_local_path(_T("PIO3034.BIN")), FILEIO_READ_BINARY)) {
		fio->Fread(data_buffer, DATA_SIZE, 1);
		fio->Fclose();
	}
	delete fio;
}

void PIO3034::release()
{
	// save emm image
	if(modified) {
		FILEIO* fio = new FILEIO();
		if(fio->Fopen(create_local_path(_T("PIO3034.BIN")), FILEIO_WRITE_BINARY)) {
			fio->Fwrite(data_buffer, DATA_SIZE, 1);
			fio->Fclose();
		}
		delete fio;
	}
	
	// release memory
	free(data_buffer);
}

void PIO3034::reset()
{
	data_addr = 0;
}

void PIO3034::write_io8(uint32_t addr, uint32_t data)
{
	switch(addr & 0xff) {
	case 0xa0:
		data_addr = (data_addr & 0xffff00) | data;
		break;
	case 0xa1:
		data_addr = (data_addr & 0xff00ff) | (data << 8);
		break;
	case 0xa2:
		data_addr = (data_addr & 0x00ffff) | (data << 16);
		break;
	case 0xa3:
		if(data_buffer[data_addr & ADDR_MASK] != (uint8_t)data) {
			data_buffer[data_addr & ADDR_MASK] = data;
			modified = true;
		}
		data_addr++;
		break;
	}
}

uint32_t PIO3034::read_io8(uint32_t addr)
{
	switch(addr & 0xff) {
	case 0xa0:
		return data_addr & 0xff;
	case 0xa1:
		return (data_addr >> 8) & 0xff;
	case 0xa2:
		return (data_addr >> 16) & 0xff;
	case 0xa3:
		return data_buffer[(data_addr++) & ADDR_MASK];
	}
	return 0xff;
}

#define STATE_VERSION	1

bool PIO3034::process_state(FILEIO* state_fio, bool loading)
{
	if(!state_fio->StateCheckUint32(STATE_VERSION)) {
		return false;
	}
	if(!state_fio->StateCheckInt32(this_device_id)) {
		return false;
	}
	state_fio->StateArray(data_buffer, DATA_SIZE, 1);
	state_fio->StateValue(data_addr);
	state_fio->StateValue(modified);
	return true;
}

