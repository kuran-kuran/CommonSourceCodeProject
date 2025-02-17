/*
	Skelton for retropc emulator

	Author : @yanatoku
	Date	 : 2024.11.30 -

	Modify : kuran_kuran
	Date	 : 2025.02.14-

	[ SD_dongle ]
*/

#ifndef _SD_DONGLE_H_
#define _SD_DONGLE_H_

#include "vm.h"
#include "../emu.h"
#include "device.h"

typedef unsigned char byte;
#define LOW 0
#define HIGH 1
//#define CABLESELECTPIN  (10)
#define CHKPIN          (15)
#define PB2PIN          (4)
#define PB3PIN          (5)
#define FLGPIN          (14)
#define PA0PIN          (16)
#define PA1PIN          (17)
#define GPIO_CNT        (18)

class SDDONGLE : public DEVICE
{
private:
	outputs_t outputs;
	char f_name[40];
	char c_name[40];
	char buf1[10],buf2[10];
	char sdir[10][40];
	FILEIO* file;
	unsigned long f_length,f_length2,f_length1;
	// ファイル名は、ロングファイルネーム形式対応
	bool eflg;
	uint8_t gpio[GPIO_CNT];

	// thread
	HANDLE hSD_dongleThread;
	bool initialized;
	CRITICAL_SECTION cs[GPIO_CNT];
	HANDLE signalEmuToThread;
	HANDLE signalThreadToEmu;
	HANDLE signalTransfer;
	bool rcvComplete;

	_TCHAR sdcard_path[_MAX_PATH];

	DEVICE* d_midi;

	void sdinit(void);
	void setup(void);
	byte rcv2bit(void);
	byte rcv1byte(void);
	void snd2bit(byte j_data);
	void snd1byte(byte i_data);
	void f_load(void);
	void f_save(void);
	void receive_name(char *f_name);
	bool f_match(char *f_name,char *c_name);
	char upper(char c);
	_TCHAR* create_tchar_text(char* text);
	char* create_char_text(const _TCHAR* text);
	_TCHAR* create_sdcard_path(char* f_name);
	void dirlist(void);
	void loop(void);
	void usleep(DWORD waitTime);
	static unsigned __stdcall loop_thread(void* param);
public:
	SDDONGLE(VM_TEMPLATE* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu)
	{
		initialize_output_signals(&outputs);
		set_device_name(_T("SD_dongle"));
		initialized = false;
	}
	~SDDONGLE() {}

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
	int sendMode;
	void set_context_midi(DEVICE* device)
	{
		d_midi = device;
	}
};

#endif
