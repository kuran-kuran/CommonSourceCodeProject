/*
	SHARP MZ-80B Emulator 'EmuZ-80B'
	SHARP MZ-2200 Emulator 'EmuZ-2200'

	Author : kuran_kuran
	Date   : 2024.05.25-

	[ AMDEK/RolandDG CMU-800 (MIDI) ]
*/

#include <algorithm>
#include "cmu800.h"
#include "midi.h"

uint8_t CMU800::rythm_table[7] = {42, 46, 49, 48, 41, 38, 35};

std::vector<int> CMU800::counterTable =
{
	0x9741, 0x8EBE, 0x86BB, 0x7F2E, 0x780B, 0x714E, 0x6AED, 0x64EC, 0x5F41, 0x59E8, 0x54D9, 0x5015,
	0x4B95, 0x4754, 0x4353, 0x3F8D, 0x3BFC, 0x389E, 0x356E, 0x326E, 0x2F99, 0x2CED, 0x2A66, 0x2805,
	0x25C5, 0x23A5, 0x21A5, 0x1FC3, 0x1DFB, 0x1C4C, 0x1AB4, 0x1934, 0x17CA, 0x1674, 0x1531, 0x1401,
	0x12E1, 0x11D1, 0x10D1, 0x0FE1, 0x0EFD, 0x0E26, 0x0D59, 0x0C99, 0x0BE4, 0x0B39, 0x0A98, 0x0A00,
	0x0970, 0x08E8, 0x0868, 0x07F0, 0x077E, 0x0712, 0x06AC, 0x064C, 0x05F2, 0x059C, 0x054C, 0x0500,
	0x04B8, 0x0474, 0x0434, 0x03F8, 0x03BF, 0x0389, 0x0356, 0x0326, 0x02F9, 0x02CE, 0x02A6, 0x0280,
	0x025C, 0x023A, 0x021A, 0x01FC, 0x01DF, 0x01C4, 0x01AB, 0x0193, 0x017C, 0x0167, 0x0153, 0x0140,
	0x012E, 0x011D, 0x010D, 0x00FE, 0x00F0, 0x00E2, 0x00D5, 0x00C9, 0x00BE, 0x00B3, 0x00A9, 0x00A0,
	0x0097, 0x008E, 0x0086, 0x007F, 0x0078, 0x0071, 0x006A, 0x0064, 0x005F, 0x0059, 0x0054, 0x0050,
	0x004B, 0x0047, 0x0043, 0x003F, 0x003C
};

void CMU800::initialize()
{
	is_reset = false;
}

void CMU800::release()
{
}

void CMU800::reset_midi()
{
	if(is_reset == true)
	{
		return;
	}
	// GM reset
	d_midi->write_signal(SIG_MIDI_OUT, 0xF0, 0xFF);
	d_midi->write_signal(SIG_MIDI_OUT, 0x7E, 0xFF);
	d_midi->write_signal(SIG_MIDI_OUT, 0x7F, 0xFF);
	d_midi->write_signal(SIG_MIDI_OUT, 0x09, 0xFF);
	d_midi->write_signal(SIG_MIDI_OUT, 0x01, 0xFF);
	d_midi->write_signal(SIG_MIDI_OUT, 0xF7, 0xFF);
	for(int channel = 0; channel < 11; ++ channel)
	{
		// all sound off
		d_midi->write_signal(SIG_MIDI_OUT, 0xB0 + channel, 0xFF);
		d_midi->write_signal(SIG_MIDI_OUT, 0x78, 0xFF);
		d_midi->write_signal(SIG_MIDI_OUT, 0, 0xFF);
	}
	base_clock = 0;
	memset(toggle, 0, sizeof(toggle));
	memset(counter, 0, sizeof(counter));
	cv = 0;
	note_on = false;
	memset(cv_key, 0, sizeof(cv_key));
	memset(note_on_flag, 0, sizeof(note_on_flag));
	memset(before_tone, 0, sizeof(before_tone));
	before_rythm = 0;
	is_reset = true;
}

void CMU800::write_io8(uint32_t addr, uint32_t data)
{
	if(!config.cmu800) {
		return;
	}
	switch(addr & 0xff) {
		// 8253-1 counter setting (not use)
	case 0x90:
	case 0x91:
	case 0x92:
		{
			int port_number = addr & 0x0F;
			uint16_t data8 = data & 0xFF;
			counter[port_number] &= (toggle[port_number] == 0 ? 0xFF00 : 0xFF);
			counter[port_number] |= data8 * (toggle[port_number] == 0 ? 1 : 256);
			toggle[port_number] = 1 - toggle[port_number];
			is_reset = false;
			break;
		}
		// 8253-1 setting
	case 0x93:
		reset_midi();
		break;
		// 8253-2 counter setting (not use)
	case 0x94:
	case 0x95:
	case 0x96:
		{
			int port_number = (addr & 0x0F) - 1;
			uint16_t data8 = data & 0xFF;
			counter[port_number] &= (toggle[port_number] == 0 ? 0xFF00 : 0xFF);
			counter[port_number] |= data8 * (toggle[port_number] == 0 ? 1 : 256);
			toggle[port_number] = 1 - toggle[port_number];
			is_reset = false;
			break;
		}
		break;
		// 8253-2 setting
	case 0x97:
		reset_midi();
		break;
	case 0x98:
		// b0-5 CV-OUT data (not use built-in sound)
		// b6 TUNE-GATE (unknown)
		// b7 GATE-DATA (0 = play, 1 = stop)
		note_on = ((data & 0x80) == 0);
		if(note_on == true)
		{
			cv = data & 0x3F;
		}
		is_reset = false;
		break;
	case 0x99:
		// built-in rhythm, 1 -> 0 = play the drum
		// b7 BD (bass drum)
		// b6 SD (snare drum)
		// b5 LT (low tom)
		// b4 HT (high tom)
		// b3 CY (cymbal)
		// b2 OH (open hihat)
		// b1 CH (close hihat)
		// b0 Reserve (not use)
		{
			uint8_t bitMask = 0b00000010;
			for(int32_t i = 0; i < 7; ++ i)
			{
				uint8_t beforeBit = before_rythm & bitMask;
				uint8_t bit = data & bitMask;
				if((beforeBit != 0) && (bit == 0))
				{
					// sound a rhythm
					d_midi->write_signal(SIG_MIDI_OUT, 0x99, 0xFF);
					d_midi->write_signal(SIG_MIDI_OUT, rythm_table[i], 0xFF);
					d_midi->write_signal(SIG_MIDI_OUT, 0x7F, 0xFF);
				}
				bitMask <<= 1;
			}
			before_rythm = data & 0xFF;
			is_reset = false;
			break;
		}
	case 0x9A:
		// b0 (1 -> 0 = Setup complete, note on or note off)
		// b1-3 = channel (0-7)
		{
			int channel = ((data >> 1) & 7);
			if(((before_tone[channel] & 1) == 1) && ((data & 1) == 0))
			{
				if(note_on == false && note_on_flag[channel] == 1)
				{
					// note off
					d_midi->write_signal(SIG_MIDI_OUT, 0x80 + channel, 0xFF);
					d_midi->write_signal(SIG_MIDI_OUT, cv_key[channel], 0xFF);
					d_midi->write_signal(SIG_MIDI_OUT, 0x7F, 0xFF);
					cv_key[channel] = 0;
					note_on_flag[channel] = 0;
				}
				else if(note_on == true && note_on_flag[channel] == 0)
				{
					if(cv == 0)
					{
						// cv‚ª0‚Ìê‡‚Íü”g”‚©‚çcv‚ğ‹‚ß‚é
						int val = counter[channel];
						int back = -1;
						for(int i = 0; i < counterTable.size(); ++ i)
						{
							if(val >= counterTable[i])
							{
								back = i;
								break;
							}
						}
						if(back != -1)
						{
							int front = back - 1;
							int x = counterTable[front] - val;
							int y = counterTable[back] - val;
							if(x * x < y * y)
							{
								cv = front;
							}
							else
							{
								cv = back;
							}
						}
					}
					// note on
					uint8_t key = cv + 24;
					d_midi->write_signal(SIG_MIDI_OUT, 0x90 + channel, 0xFF);
					d_midi->write_signal(SIG_MIDI_OUT, key, 0xFF);
					d_midi->write_signal(SIG_MIDI_OUT, 0x7F, 0xFF);
					note_on_flag[channel] = 1;
					cv_key[channel] = key;
				}
			}
			before_tone[channel] = data & 0xFF;
			is_reset = false;
			break;
		}
	case 0x9B:
		// 8255 setting
		reset_midi();
		break;
		// dummy port (Nothing to do.)
	case 0x9C:
		break;
	}
}

uint32_t CMU800::read_io8(uint32_t addr)
{
	if(!config.cmu800) {
		return 0;
	}
	if((addr & 0xff) == 0x9A)
	{
		// tempo
		if(base_clock == 0)
		{
			base_clock = d_event->get_current_clock();
		}
		uint32_t cpu_clock = d_event->get_event_clocks();
		uint32_t half = cpu_clock / config.cmu800_tempo;
		uint32_t length = half * 2;
		uint32_t clock = d_event->get_current_clock() - base_clock;
		return (clock % length) >= half ? 0xF0 : 0;
	}
	return 0;
}

#define STATE_VERSION	1

bool CMU800::process_state(FILEIO* state_fio, bool loading)
{
	if(!state_fio->StateCheckUint32(STATE_VERSION)) {
		return false;
	}
	if(!state_fio->StateCheckInt32(this_device_id)) {
		return false;
	}
	// save/load status
	state_fio->StateValue(base_clock);
	state_fio->StateArray(toggle, sizeof(toggle), 1);
	state_fio->StateArray(counter, sizeof(counter), 1);
	state_fio->StateValue(cv);
	state_fio->StateValue(note_on);
	state_fio->StateArray(cv_key, sizeof(cv_key), 1);
	state_fio->StateArray(note_on_flag, sizeof(note_on_flag), 1);
	state_fio->StateArray(before_tone, sizeof(before_tone), 1);
	state_fio->StateValue(before_rythm);
	return true;
}
