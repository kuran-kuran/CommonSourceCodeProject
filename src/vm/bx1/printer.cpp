/*
	CANON BX-1 Emulator 'eBX-1'

	Author : Takeda.Toshiya
	Date   : 2021.02.06-

	[ printer ]
*/

#include "printer.h"

const uint8_t table[] = {
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
	0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x9d, 0x5d, 0x5e, 0x5f,
	0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0xb1, // 7Fh -> B1h
	0x20, 0x18, 0x19, 0x1a, 0x1b, 0xd8, 0xf3, 0xf2, 0x78, 0xf6, 0xfb, 0xfc, 0xe3, 0xe4, 0xc0, 0xb1,
	0xfa, 0xf9, 0x78, 0x2b, 0xf9, 0xc4, 0xb3, 0xda, 0xbf, 0xd9, 0xb0, 0x1e, 0xf8, 0x27, 0x9c, 0xb2,
};

void PRINTER::initialize()
{
	fio = new FILEIO();
	emu->get_osd()->open_console(82, 30, create_string(_T("Printer - %s"), _T(DEVICE_NAME)));
	emu->get_osd()->set_console_code_page(437);
	emu->get_osd()->set_console_cursor_position(0, 8);
	
	register_vline_event(this);
}

void PRINTER::release()
{
	if(fio->IsOpened()) {
		fio->Fclose();
	}
	delete fio;
	emu->get_osd()->close_console();
}

void PRINTER::reset()
{
	if(fio->IsOpened()) {
		fio->Fclose();
	}
	column = func = 0;
	
	strobe = outdata = 0;
}

/*
$E210: read / write
$E211: read（してからreti) / write ($0193の指すアドレスから転送)
$E212: read (bit操作して$E210にwrite)
*/

void PRINTER::write_io8(uint32_t addr, uint32_t data)
{
	switch(addr & 0xffff) {
	case 0xe210:
		// C1 -> A1で$E211のデータが出力される？
		if((strobe & 0xe1) == 0xc1 && (data & 0xe1) == 0xa1) {
//			output(outdata);
		}
		strobe = data;
		break;
	case 0xe211:
		outdata = data;
		break;
	}
}

uint32_t PRINTER::read_io8(uint32_t addr)
{
	uint32_t value = 0xff;
	
	switch(addr & 0xffff) {
	case 0xe210:
		// 0x00, 0x80, 0x81 ?
		// bit7: 1=AUTO PRINT ?
		// bit0: PE or BUSY ?
		value = (config.dipswitch & 1) ? 0 : 0x80;
		break;
	case 0xe211:
		value = outdata;
		break;
	case 0xe212:
		value = strobe;
		break;
	}
	return value;
}

void PRINTER::event_vline(int v, int clock)
{
	// ugly patch for printer :-(
	// this code should be called from interrupt handler
#if 0
	0000BDF8  FE0265            ldx  $0265
	0000BDFB  8D0B              bsr  $BE08
	0000BDFD  BC0263            cmpx $0263
	0000BE00  2705              beq  $BE07
	0000BE02  E600              ldb  (x+$00)
	0000BE04  FF0265            stx  $0265
	0000BE07  39                rts  
	0000BE08  08                inx  
	0000BE09  8C0295            cmpx #$0295
	0000BE0C  2603              bne  $BE11
	0000BE0E  CE026E            ldx  #$026E
	0000BE11  39                rts  
#endif
	pair16_t p1, p2;
	p1.read_2bytes_be_from(ram + 0x265);
	p2.read_2bytes_be_from(ram + 0x263);
	p1.w++;
	if(p1.w == 0x295) {
		p1.w = 0x26e;
	}
	if(p1.w != p2.w) {
		uint8_t b = ram[p1.w];
		if(func) {
			if(func == 0xf0) {
				if(column < b) {
					for(int i = 0; i < b - column; i++) {
						output_file(0x20);
					}
					int x, y;
					emu->get_osd()->get_console_cursor_position(&x, &y);
					emu->get_osd()->set_console_cursor_position(x + b - column, y);
					column = b;
				}
			} else if(func == 0xf2) {
				for(int i = 0; i < b; i++) {
					output_char(0x20);
				}
				column += b;
			} else if(func == 0xf4) {
				int x, y;
				emu->get_osd()->get_console_cursor_position(&x, &y);
				emu->get_osd()->set_console_cursor_position(0, y + b);
				column = 0;
			} else if(func == 0xf5) {
				int x, y;
				emu->get_osd()->get_console_cursor_position(&x, &y);
				emu->get_osd()->set_console_cursor_position(0, max(y - b, 0));
				column = 0;
			}
			func = 0;
		} else if((b & 0xf0) == 0xf0) {
			func = b;
		} else if(b == 0x0d) {
			output_char(0x0d);
			column = 0;
		} else if(b == 0x12) {
			output_char(0x0a);
		} else if(b == 0x85) {
			// not equal to
			output_char(0x3c);
			output_char(0x3e);
			column++;
		} else if(b >= 0x20 && b <= 0x9f) {
			output_char(b);
			column++;
		}
		p1.write_2bytes_be_to(ram + 0x265);
	}
}

void PRINTER::output_char(uint8_t value)
{
	if(value == 0x0d || value == 0x0a || (value >= 0x20 && value < 0x7f)) {
		output_file(value);
	}
	if(value >= 0x20 && value <= 0x9f) {
		value = table[value - 0x20];
	}
	char temp[2];
	temp[0] = (char)value;
	temp[1] = 0;
	emu->get_osd()->write_console_char(temp, 1);
}

void PRINTER::output_file(uint8_t value)
{
	if(1/*config.printer_type == 0*/) {
		if(!fio->IsOpened()) {
			_TCHAR file_path[_MAX_PATH];
			create_date_file_path(file_path, _MAX_PATH, _T("txt"));
			fio->Fopen(file_path, FILEIO_WRITE_BINARY);
		}
		fio->Fputc(value);
	}
}

void PRINTER::key_down(int code)
{
	// ugly patch for PAPER FEED
	if(code == 0x91) {
		output_char(0x0d);
		output_char(0x0a);
	}
}

void PRINTER::key_up(int code)
{
}

#define STATE_VERSION	1

bool PRINTER::process_state(FILEIO* state_fio, bool loading)
{
	if(!state_fio->StateCheckUint32(STATE_VERSION)) {
		return false;
	}
	if(!state_fio->StateCheckInt32(this_device_id)) {
		return false;
	}
	state_fio->StateValue(column);
	state_fio->StateValue(func);
	state_fio->StateValue(strobe);
	state_fio->StateValue(outdata);
	return true;
}

