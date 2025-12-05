/*
	SHARP MZ-80B Emulator 'EmuZ-80B'
	SHARP MZ-2200 Emulator 'EmuZ-2200'

	Author : Takeda.Toshiya
	Date   : 2013.03.17-

	Author : kuran_kuran
	Date   : 2025.11.30-

	[ PIO-3034 EMM ]
*/

#include "pio3034.h"

size_t PIO3034::size_tbl[USE_EMM_SIZE] =
{
	320 * 1024,			// 320KB
	512 * 1024,			// 512KB
	8 * 1024 * 1024,	// 8MB
	16 * 1024 * 1024	// 16MB
};

void PIO3034::initialize()
{
	memset(ram, 0, sizeof(ram));
	address = 0;
	FILEIO* fio = new FILEIO();
	_TCHAR file_name[_MAX_PATH] = _T("PIO3034_0.BIN");
	_TCHAR index = (base_port_number - 0xa0) / 4;
	file_name[8] = '0' + index;
	bool emms[] = {config.emm0, config.emm1, config.emm2, config.emm3};
	bool enable_emm = (index < 4) && emms[index];
	if(enable_emm && fio->Fopen(create_local_path(file_name), FILEIO_READ_BINARY)) {
		fio->Fread(ram, size_tbl[config.emm_size], 1);
		fio->Fclose();
	}
	delete fio;
}

void PIO3034::release()
{
	FILEIO* fio = new FILEIO();
	_TCHAR file_name[_MAX_PATH] = _T("PIO3034_0.BIN");
	_TCHAR index = (base_port_number - 0xa0) / 4;
	file_name[8] = '0' + index;
	bool emms[] = {config.emm0, config.emm1, config.emm2, config.emm3};
	bool enable_emm = (index < 4) && emms[index];
	if(enable_emm) {
		long file_size = 0;
		if(fio->Fopen(create_local_path(file_name), FILEIO_READ_BINARY)) {
			fio->Fseek(0, FILEIO_SEEK_END);
			file_size = fio->Ftell();
			fio->Fclose();
		}
		if(file_size != size_tbl[config.emm_size]) {

			_TCHAR file_rename[_MAX_PATH] = _T("PIO3034_0.BAK");
			_TCHAR index = (base_port_number - 0xa0) / 4;
			file_rename[8] = '0' + index;
			fio->RemoveFile(create_local_path(file_rename));
			fio->RenameFile(create_local_path(file_name), create_local_path(file_rename));
		}
		if(fio->Fopen(create_local_path(file_name), FILEIO_WRITE_BINARY)) {
			fio->Fwrite(ram, size_tbl[config.emm_size], 1);
			fio->Fclose();
		}
	}
	delete fio;
}

void PIO3034::reset()
{
	initialize();
}

void PIO3034::write_io8(uint32_t addr, uint32_t data)
{
	uint8_t port_number = addr & 0xff;
	if(port_number >= 0xac && port_number <= 0xaf  && !config.emm3) {
		return;
	}
	else if(port_number >= 0xa8 && port_number <= 0xab && !config.emm2) {
		return;
	}
	else if(port_number >= 0xa4 && port_number <= 0xa7 && !config.emm1) {
		return;
	}
	else if(port_number >= 0xa0 && port_number <= 0xa3 && !config.emm0) {
		return;
	}
	if(base_port_number == port_number) {
		address = (address & 0xffff00) | data;
	}
	else if(base_port_number + 1 == port_number) {
		address = (address & 0xff00ff) | (data << 8);
	}
	else if(base_port_number + 2 == port_number) {
		address = (address & 0x00ffff) | (data << 16);
	}
	else if(base_port_number + 3 == port_number) {
		ram[address] = data;
		address = (address + 1) % size_tbl[config.emm_size];
	}
}

uint32_t PIO3034::read_io8(uint32_t addr)
{
	uint8_t port_number = addr & 0xff;
	if(port_number >= 0xac && port_number <= 0xaf  && !config.emm3) {
		return 0xff;
	}
	else if(port_number >= 0xa8 && port_number <= 0xab && !config.emm2) {
		return 0xff;
	}
	else if(port_number >= 0xa4 && port_number <= 0xa7 && !config.emm1) {
		return 0xff;
	}
	else if(port_number >= 0xa0 && port_number <= 0xa3 && !config.emm0) {
		return 0xff;
	}
	if(base_port_number + 3 == port_number) {
		uint32_t result = ram[address];
		address = (address + 1) % size_tbl[config.emm_size];
		return result;
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
	state_fio->StateValue(base_port_number);
	state_fio->StateArray(ram, sizeof(ram), 1);
	state_fio->StateValue(address);
	return true;
}
