/*
	Skelton for retropc emulator

	Author : @yanatoku
	Date   : 2022.01.24 -

	Modify : kuran_kuran
	Date   : 2024.06.06-

	[ mz80k_sd ]
*/

#ifndef _MZ80K_SD_H_
#define _MZ80K_SD_H_

#include "vm.h"
#include "../emu.h"
#include "device.h"

typedef unsigned char byte;
#define LOW 0
#define HIGH 1
//#define CABLESELECTPIN  (10)
#define CHKPIN          (15)
#define PB0PIN          (2)
#define PB1PIN          (3)
#define PB2PIN          (4)
#define PB3PIN          (5)
#define PB4PIN          (6)
#define PB5PIN          (7)
#define PB6PIN          (8)
#define PB7PIN          (9)
#define FLGPIN          (14)
#define PA0PIN          (16)
#define PA1PIN          (17)
#define PA2PIN          (18)
#define PA3PIN          (19)
#define GPIO_CNT        (20)

class MZ80K_SD : public DEVICE
{
private:
	outputs_t outputs;
	uint32_t m_lop;
	char m_name[40];
	byte s_data[260];
	char f_name[40];
	char c_name[40];
	char new_name[40];
	// concat file
	char concatName[40];
	int isConcatState; // 0:not use, 1: opened
	FILEIO* concatFile;
	uint32_t concatPos;
	uint32_t concatSize;
	bool eflg;
	uint8_t gpio[GPIO_CNT];
	// D88 image
	char d88Name[40];
	bool isD88State; // 0:未使用, 1:オープンしている
	bool reverse;
	FILEIO* d88File;
	int seekInfoPointer;
	int seekDataPointer;
	int seekDataOffset;
	char c;
	char h;
	char r;
	char n;
	short numberOfSector;
	short sizeOfData;
	short sectorsPerTrack;

	// thread
	HANDLE hMz80kSdThread;
	bool initialized;
	CRITICAL_SECTION cs[GPIO_CNT];
	HANDLE signalEmuToThread;
	HANDLE signalThreadToEmu;
	HANDLE signalTransfer;
	bool rcvComplete;

	_TCHAR sdcard_path[_MAX_PATH];

	DEVICE* d_midi;

	void setup();
	byte rcv4bit(bool wait);
	byte rcv1byte(void);
	void snd1byte(uint8_t i_data);
	char upper(char c);
	void addmzt(char *f_name);
	_TCHAR* create_tchar_text(char* text);
	char* create_char_text(const _TCHAR* text);
	_TCHAR* create_sdcard_path(char* f_name);
	// MZ2000_SD標準Api
	void f_save(void);
	void f_load(void);
	void astart(void);
	void dirlist(char type);
	bool f_match(char *f_name,char *c_name);
	void f_del(void);
	void f_ren(void);
	void f_dump(void);
	void f_copy(void);
	void mon_whead(void);
	void mon_wdata(void);
	void mon_lhead(void);
	void mon_ldata(void);
	void boot(void);
	// Concat Api
	void ConcatFileOpen();
	void ConcatFileRead();
	void ConcatFileSkip();
	void ConcatFileFind();
	void ConcatFileTop();
	void ConcatFileClose();
	void ConcatFileState(void);
	// MIDI Api
	void SendMidi(void);
	// D88 Api
	bool D88Open(const char* path, bool r);
	void D88Close(void);
	void D88SetSectorsPerTrack(short num);
	bool D88Seek(char track, char sector);
	bool D88SeekLba(int lba);
	int D88GetSectorSize(void);
	unsigned char D88Read(void);
	void D88Write(unsigned char data);
	// D88 image
	void d88FileList(void);
	void d88OpenRead(void);
	void d88OpenWrite(void);
	void d88Close(void);
	void d88ReadLba(void);
	void d88WriteLba(void);
	// メインループ
	void loop();
	static unsigned __stdcall loop_thread(void* param);
public:
	MZ80K_SD(VM_TEMPLATE* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu)
	{
		initialize_output_signals(&outputs);
		set_device_name(_T("MZ80K_SD"));
		initialized = false;
	}
	~MZ80K_SD() {}

	// common functions
	void initialize();
	void release();
	void reset();

	// unique function
	void digitalWrite(int pin, int data, int from = 0);
	int digitalRead(int pin, int from = 0);
	void setFlg(bool flag);
	bool getChk();
	bool terminate;
	void set_context_midi(DEVICE* device)
	{
		d_midi = device;
	}
};

#endif
