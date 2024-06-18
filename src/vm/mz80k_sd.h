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

#include <vector>
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
	unsigned long m_lop=128;
	char m_name[40];
	byte s_data[260];
	char f_name[40];
	char c_name[40];
	char new_name[40];
	// concat file
	char concatName[40];
	int isConcatState = 0; // 0:not use, 1: opened
	FILEIO* concatFile = NULL;
	unsigned long concatPos = 0;
	unsigned long concatSize = 0;
	bool eflg;
	unsigned char gpio[GPIO_CNT];

	// thread
	HANDLE hMz80kSdThread;
	bool initialized;
	CRITICAL_SECTION cs[GPIO_CNT];
	HANDLE signalEmuToThread;
	HANDLE signalThreadToEmu;
	HANDLE signalTransfer;
	bool rcvComplete;

	void setup();
	byte rcv4bit(bool wait);
	byte rcv1byte(void);
	void snd1byte(byte i_data);
	char upper(char c);
	void addmzt(char *f_name);
	void f_save(void);
	void f_load(void);
	void astart(void);
	void dirlist(void);
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
	void ConcatFileOpen();
	void ConcatFileRead();
	void ConcatFileSkip();
	void ConcatFileFind();
	void ConcatFileTop();
	void ConcatFileClose();
	void ConcatFileState(void);
	void SendMidi(void);
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
	void Report(const char* text, ...);
	bool terminate;

	HANDLE globalMemoryHandle;
	unsigned char* buffer;
};

#endif
