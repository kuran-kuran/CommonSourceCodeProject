/*
	Skelton for retropc emulator

	Author : @yanatoku
	Date	 : 2022.01.24 -

	Modify : kuran_kuran
	Date	 : 2024.06.06-

	[ mz80k_sd ]
*/

#include <string>
#include <vector>
#include "mz80k_sd.h"

void MZ80K_SD::initialize()
{
	terminate = false;
	initialized = false;
	long m_lop = 128;
	isConcatState = 0; // 0:not use, 1: opened
	concatFile = NULL;
	concatPos = 0;
	concatSize = 0;
}

void MZ80K_SD::release()
{
	if(initialized == false)
	{
		return;
	}
	terminate = true;
	SetEvent(signalEmuToThread);
	SetEvent(signalThreadToEmu);
	WaitForSingleObject(hMz80kSdThread, INFINITE);
	for(int i = 0; i < GPIO_CNT; ++ i)
	{
		DeleteCriticalSection(&cs[i]);
	}
	hMz80kSdThread = NULL;
	if(concatFile != NULL)
	{
		delete concatFile;
		concatFile = NULL;
	}
	initialized = false;
}

void MZ80K_SD::reset()
{
	release();
	terminate = false;
	for(int i = 0; i < GPIO_CNT; ++ i)
	{
		InitializeCriticalSection(&cs[i]);
	}
	signalEmuToThread = CreateEventA(NULL, FALSE, FALSE, NULL); // emu -> thread
	ResetEvent(signalEmuToThread);
	signalThreadToEmu = CreateEventA(NULL, FALSE, FALSE, NULL); // thread -> emu
	ResetEvent(signalThreadToEmu);
	signalTransfer = CreateEventA(NULL, FALSE, FALSE, NULL); // EnableRcv
	ResetEvent(signalTransfer);
	rcvComplete = false;
	setup();
	hMz80kSdThread = (HANDLE)_beginthreadex(NULL, 0, MZ80K_SD::loop_thread, this, 0, NULL);
	concatFile = new FILEIO();
	initialized = true;
}

void MZ80K_SD::digitalWrite(int pin, int data, int from)
{
	EnterCriticalSection(&cs[pin]);
	gpio[pin] = data & 1;
	LeaveCriticalSection(&cs[pin]);
}

int MZ80K_SD::digitalRead(int pin, int from)
{
	if(terminate == true)
	{
		throw "terminate";
	}
	EnterCriticalSection(&cs[pin]);
	int data = gpio[pin];
	LeaveCriticalSection(&cs[pin]);
	return data;
}

void MZ80K_SD::setFlg(bool flag)
{
	digitalWrite(CHKPIN, flag, 1);
	SetEvent(signalEmuToThread);
}

bool MZ80K_SD::getChk()
{
	WaitForSingleObject(signalThreadToEmu, INFINITE);
	bool chk = digitalRead(FLGPIN, 1) == 1;
	if(chk == 0 && rcvComplete == true) {
		SetEvent(signalTransfer);
		rcvComplete = false;
	}
	return chk;
}

void MZ80K_SD::setup(){
//	Serial.begin(31250); // MIDI

////	Serial.begin(9600);
// CS=pin10
// pin10 output

/*
	pinMode(CABLESELECTPIN,OUTPUT);
	pinMode( CHKPIN,INPUT);	//CHK
	pinMode( PB0PIN,OUTPUT); //送信データ 
	pinMode( PB1PIN,OUTPUT); //送信データ 
	pinMode( PB2PIN,OUTPUT); //送信データ 
	pinMode( PB3PIN,OUTPUT); //送信データ 
	pinMode( PB4PIN,OUTPUT); //送信データ 
	pinMode( PB5PIN,OUTPUT); //送信データ 
	pinMode( PB6PIN,OUTPUT); //送信データ 
	pinMode( PB7PIN,OUTPUT); //送信データ 
	pinMode( FLGPIN,OUTPUT); //FLG

	pinMode( PA0PIN,INPUT_PULLUP); //受信データ 
	pinMode( PA1PIN,INPUT_PULLUP); //受信データ 
	pinMode( PA2PIN,INPUT_PULLUP); //受信データ 
	pinMode( PA3PIN,INPUT_PULLUP); //受信データ 
*/
	for(int i = 0; i < GPIO_CNT; ++ i)
	{
		digitalWrite(i, LOW);
	}

/*
	digitalWrite(PB0PIN,LOW);
	digitalWrite(PB1PIN,LOW);
	digitalWrite(PB2PIN,LOW);
	digitalWrite(PB3PIN,LOW);
	digitalWrite(PB4PIN,LOW);
	digitalWrite(PB5PIN,LOW);
	digitalWrite(PB6PIN,LOW);
	digitalWrite(PB7PIN,LOW);
	digitalWrite(FLGPIN,LOW);
*/

// 2022. 2. 4 MZ-1200対策 
//	Sleep(1500);

	// SD初期化 
//	if( !SD.begin(CABLESELECTPIN,8) )
	{
////		Serial.println("Failed : SD.begin");
//		eflg = true;
//	} else {
////		Serial.println("OK : SD.begin");
		eflg = false;
	}
////	Serial.println("START");
}

//4BIT受信 
byte MZ80K_SD::rcv4bit(bool wait){
//HIGHになるまでループ 
	WaitForSingleObject(signalEmuToThread, INFINITE);
	if(terminate == true) {
		throw "terminate";
	}
//	while(digitalRead(CHKPIN) != HIGH){
//	}
//受信 
	byte j_data = digitalRead(PA0PIN)+digitalRead(PA1PIN)*2+digitalRead(PA2PIN)*4+digitalRead(PA3PIN)*8;
//FLGをセット 
	digitalWrite(FLGPIN,HIGH);
	SetEvent(signalThreadToEmu);
//LOWになるまでループ 
	WaitForSingleObject(signalEmuToThread, INFINITE);
	if(terminate == true) {
		throw "terminate";
	}
//	while(digitalRead(CHKPIN) == HIGH){
//	}
//FLGをリセット 
	ResetEvent(signalTransfer);
	rcvComplete = true;
	digitalWrite(FLGPIN,LOW);
	SetEvent(signalThreadToEmu);
	WaitForSingleObject(signalTransfer, INFINITE);
	return(j_data);
}

//1BYTE受信 
byte MZ80K_SD::rcv1byte(void){
	byte i_data = 0;
	i_data=rcv4bit(false)*16;
	i_data=i_data+rcv4bit(true);
	return(i_data);
}

//1BYTE送信 
void MZ80K_SD::snd1byte(byte i_data){
//下位ビットから8ビット分をセット 
	digitalWrite(PB0PIN,(i_data)&0x01);
	digitalWrite(PB1PIN,(i_data>>1)&0x01);
	digitalWrite(PB2PIN,(i_data>>2)&0x01);
	digitalWrite(PB3PIN,(i_data>>3)&0x01);
	digitalWrite(PB4PIN,(i_data>>4)&0x01);
	digitalWrite(PB5PIN,(i_data>>5)&0x01);
	digitalWrite(PB6PIN,(i_data>>6)&0x01);
	digitalWrite(PB7PIN,(i_data>>7)&0x01);
	digitalWrite(FLGPIN,HIGH);
	SetEvent(signalThreadToEmu);
	WaitForSingleObject(signalEmuToThread, INFINITE);
	if(terminate == true) {
		throw "terminate";
	}
//HIGHになるまでループ 
//	while(digitalRead(CHKPIN) != HIGH){
//	}
	digitalWrite(FLGPIN,LOW);
	SetEvent(signalThreadToEmu);
//LOWになるまでループ 
	WaitForSingleObject(signalEmuToThread, INFINITE);
	if(terminate == true) {
		throw "terminate";
	}
//	while(digitalRead(CHKPIN) == HIGH){
//	}
}

//小文字->大文字 
char MZ80K_SD::upper(char c){
	if('a' <= c && c <= 'z'){
		c = c - ('a' - 'A');
	}
	return c;
}

//ファイル名の最後が「.mzt」でなければ付加 
void MZ80K_SD::addmzt(char *f_name){
	unsigned int lp1=0;
	while (f_name[lp1] != 0x0D){
		lp1++;
	}
	if (strlen(f_name) < 4 ||
		f_name[lp1-4]!='.' ||
		( f_name[lp1-3]!='M' &&
			f_name[lp1-3]!='m' ) ||
		( f_name[lp1-2]!='Z' &&
			f_name[lp1-2]!='z' ) ||
		( f_name[lp1-1]!='T' &&
			f_name[lp1-1]!='t' ) ){
				 f_name[lp1++] = '.';
				 f_name[lp1++] = 'm';
				 f_name[lp1++] = 'z';
				 f_name[lp1++] = 't';
	}
	f_name[lp1] = 0x00;
}

//SDカードにSAVE 
void MZ80K_SD::f_save(void){
	char p_name[20];

//保存ファイルネーム取得 
	for (unsigned int lp1 = 0;lp1 <= 32;lp1++){
		f_name[lp1] = rcv1byte();
	}
	addmzt(f_name);
//プログラムネーム取得 
	for (unsigned int lp1 = 0;lp1 <= 16;lp1++){
		p_name[lp1] = rcv1byte();
	}
	p_name[15] =0x0D;
	p_name[16] =0x00;
//スタートアドレス取得 
	int s_adrs1 = rcv1byte();
	int s_adrs2 = rcv1byte();
//スタートアドレス算出 
	unsigned int s_adrs = s_adrs1+s_adrs2*256;
//エンドアドレス取得 
	int e_adrs1 = rcv1byte();
	int e_adrs2 = rcv1byte();
//エンドアドレス算出 
	unsigned int e_adrs = e_adrs1+e_adrs2*256;
//実行アドレス取得 
	int g_adrs1 = rcv1byte();
	int g_adrs2 = rcv1byte();
//実行アドレス算出 
	unsigned int g_adrs = g_adrs1+g_adrs2*256;
//ファイルサイズ算出 
	unsigned int f_length = e_adrs - s_adrs + 1;
	unsigned int f_length1 = f_length % 256;
	unsigned int f_length2 = f_length / 256;
//ファイルが存在すればdelete
	if (FILEIO::IsFileExisting(create_local_path(f_name)) == true){
			FILEIO::RemoveFile(create_local_path(f_name));
	}
//ファイルオープン 
	FILEIO* file = new FILEIO();
	bool result = file->Fopen( create_local_path(f_name), FILEIO_WRITE_BINARY );
	if( true == result ){
//状態コード送信(OK)
		snd1byte(0x00);
//ファイルモード設定(01)
		file->Fputc(char(0x01));
//プログラムネーム 
		file->Fwrite(p_name, sizeof(p_name), 1);
		file->Fputc(char(0x00));
//ファイルサイズ 
		file->Fputc(f_length1);
		file->Fputc(f_length2);
//スタートアドレス 
		file->Fputc(s_adrs1);
		file->Fputc(s_adrs2);
//実行アドレス 
		file->Fputc(g_adrs1);
		file->Fputc(g_adrs2);
//7Fまで00埋め 
		for (unsigned int lp1 = 0;lp1 <= 103;lp1++){
			file->Fputc(char(0x00));
		}
//実データ 
		long lp1 = 0;
		while (lp1 <= (long)f_length-1){
			int i=0;
			while(i<=255 && lp1<=(long)f_length-1){
				s_data[i]=rcv1byte();
				i++;
				lp1++;
			}
			file->Fwrite(s_data, i, 1);
		}
		file->Fclose();
	} else {
//状態コード送信(ERROR)
		snd1byte(0xF1);
	}
	delete file;
}

//SDカードから読込 
void MZ80K_SD::f_load(void){
//ファイルネーム取得 
	for (unsigned int lp1 = 0;lp1 <= 32;lp1++){
		f_name[lp1] = rcv1byte();
	}
	addmzt(f_name);
//ファイルが存在しなければERROR
	if (FILEIO::IsFileExisting(create_local_path(f_name)) == true){
//ファイルオープン 
		FILEIO* file = new FILEIO();
		bool result = file->Fopen( create_local_path(f_name), FILEIO_READ_BINARY );
		if( true == result ){
//ファイル種類コードの判別を撤廃 
//			if( file.read() == 0x01){
//状態コード送信(OK)
				snd1byte(0x00);
				int wk1 = 0;
				wk1 = file->Fgetc();
				for (unsigned int lp1 = 0;lp1 <= 16;lp1++){
					wk1 = file->Fgetc();
					snd1byte(wk1);
				}
//ファイルサイズ取得 
				int f_length2 = file->Fgetc();
				int f_length1 = file->Fgetc();
				unsigned int f_length = f_length1*256+f_length2;
//スタートアドレス取得 
				int s_adrs2 = file->Fgetc();
				int s_adrs1 = file->Fgetc();
				unsigned int s_adrs = s_adrs1*256+s_adrs2;
//実行アドレス取得 
				int g_adrs2 = file->Fgetc();
				int g_adrs1 = file->Fgetc();
				unsigned int g_adrs = g_adrs1*256+g_adrs2;
				snd1byte(s_adrs2);
				snd1byte(s_adrs1);
				snd1byte(f_length2);
				snd1byte(f_length1);
				snd1byte(g_adrs2);
				snd1byte(g_adrs1);
				file->Fseek(128, FILEIO_SEEK_SET);
//データ送信 
				for (unsigned int lp1 = 0;lp1 < f_length;lp1++){
						byte i_data = file->Fgetc();
						snd1byte(i_data);
				}
				file->Fclose();
//			 } else {
//状態コード送信(ERROR)
//				snd1byte(0xF2);
//			}	
		} else {
//状態コード送信(ERROR)
			snd1byte(0xFF);
		}
		delete file;
	 } else {
//状態コード送信(FILE NOT FIND ERROR)
		snd1byte(0xF1);
	}
}

//ASTART 指定されたファイルをファイル名「0000.mzt」としてコピー 
void MZ80K_SD::astart(void){
	char w_name[]="0000.mzt";

//ファイルネーム取得 
	for (unsigned int lp1 = 0;lp1 <= 32;lp1++){
		f_name[lp1] = rcv1byte();
	}
	addmzt(f_name);
//ファイルが存在しなければERROR
	if (FILEIO::IsFileExisting(create_local_path(f_name)) == true){
//0000.mztが存在すればdelete
		if (FILEIO::IsFileExisting(create_local_path(f_name)) == true){
				FILEIO::RemoveFile(create_local_path(f_name));
		}
//ファイルオープン 
		FILEIO* file_r = new FILEIO();
		FILEIO* file_w = new FILEIO();
		bool result_r = file_r->Fopen( create_local_path(f_name), FILEIO_READ_BINARY );
		file_w->Fopen( create_local_path(w_name), FILEIO_WRITE_BINARY );
		if( true == result_r ){
//実データ 
			unsigned int f_length = file_r->FileLength();
			long lp1 = 0;
			while (lp1 <= (long)f_length-1){
				int i=0;
				while(i<=255 && lp1<=(long)f_length-1){
					s_data[i]=file_r->Fgetc();
					i++;
					lp1++;
				}
				file_w->Fwrite(s_data,i,1);
			}
			file_w->Fclose();
			file_r->Fclose();
//状態コード送信(OK)
			snd1byte(0x00);
		} else {
//状態コード送信(ERROR)
			snd1byte(0xF1);
		}
		delete file_r;
		delete file_w;
	} else {
//状態コード送信(ERROR)
		snd1byte(0xF1);
	}	
}

// SD-CARDのFILELIST
void MZ80K_SD::dirlist(void){
//比較文字列取得 32+1文字まで 
	for (unsigned int lp1 = 0;lp1 <= 32;lp1++){
		c_name[lp1] = rcv1byte();
//	Serial.print(c_name[lp1],HEX);
//	Serial.println("");
	}
//
	FILEIO* file = new FILEIO();
	bool entry = file->FindFirst(create_local_path(_T("\\*.mzt")));
	int cntl2 = 0;
	unsigned int br_chk =0;
	int page = 1;
//全件出力の場合には20件出力したところで一時停止、キー入力により継続、打ち切りを選択 
	while (br_chk == 0) {
		if(entry){
			const _TCHAR* name = file->FindFile();
			my_tcscpy_s(f_name, 36, name);
			unsigned int lp1=0;
//一件送信 
//比較文字列でファイルネームを先頭10文字まで比較して一致するものだけを出力 
			if (f_match(f_name,c_name)){
				while (lp1<=36 && f_name[lp1]!=0x00){
				snd1byte(upper(f_name[lp1]));
				lp1++;
				}
				snd1byte(0x0D);
				snd1byte(0x00);
				cntl2++;
			}
		}
		if (!entry || cntl2 > 19){
//継続・打ち切り選択指示要求 
			snd1byte(0xfe);

//選択指示受信(0:継続 B:前ページ 以外:打ち切り)
			br_chk = rcv1byte();
//前ページ処理 
			if (br_chk==0x42){
//先頭ファイルへ 
				file->FindRrewind();
//entry値更新 
				entry = file->FindNext();
//もう一度先頭ファイルへ 
				file->FindRrewind();
				if(page <= 2){
//現在ページが1ページ又は2ページなら1ページ目に戻る処理 
					page = 0;
				} else {
//現在ページが3ページ以降なら前々ページまでのファイルを読み飛ばす 
					page = page -2;
					cntl2=0;
					while(cntl2 < page*20){
						entry =	file->FindNext();
//					if (upper(f_name[0]) == pg0 || pg0 == 0x20){
						if (f_match(f_name,c_name)){
							cntl2++;
						}
					}
				}
				br_chk=0;
			}
			page++;
			cntl2 = 0;
		}
//ファイルがまだあるなら次読み込み、なければ打ち切り指示 
		if (entry){
			entry =	file->FindNext();
		}else{
			br_chk=1;
		}
//FDLの結果が20件未満なら継続指示要求せずにそのまま終了 
		if (!entry && cntl2 < 20 && page ==1){
			break;
		}
	}
	file->FindClose();
	delete file;
//処理終了指示 
	snd1byte(0xFF);
	snd1byte(0x00);
}

//f_nameとc_nameをc_nameに0x00が出るまで比較 
//FILENAME COMPARE
bool MZ80K_SD::f_match(char *f_name,char *c_name){
	bool flg1 = true;
	unsigned int lp1 = 0;
//	Serial.print(f_name);
//	Serial.print(" ");
//	Serial.print(c_name);
//	Serial.print(" ");
	while (lp1 <=32 && c_name[0] != 0x00 && flg1 == true){
//	Serial.print(f_name[lp1],HEX);
//	Serial.print("-");
//	Serial.print(c_name[lp1+1],HEX);
//	Serial.print(" ");
		if (upper(f_name[lp1]) != c_name[lp1+1]){
			flg1 = false;
		}
		lp1++;
		if (c_name[lp1+1]==0x00){
			break;
		}
	}
//	if (flg1){
//		Serial.println("true");
//	}else{
//		Serial.println("false");
//	}
	return flg1;
}

//FILE DELETE
void MZ80K_SD::f_del(void){

//ファイルネーム取得 
	for (unsigned int lp1 = 0;lp1 <= 32;lp1++){
		f_name[lp1] = rcv1byte();
	}
	addmzt(f_name);

//ファイルが存在しなければERROR
	if (FILEIO::IsFileExisting(create_local_path(f_name)) == true){
//状態コード送信(OK)
		snd1byte(0x00);

//処理選択を受信(0:継続してDELETE 0以外:CANSEL)
		if (rcv1byte() == 0x00){
			if (FILEIO::RemoveFile(create_local_path(f_name)) == true){
//状態コード送信(OK)
				snd1byte(0x00);
			}else{
//状態コード送信(Error)
				snd1byte(0xf1);
			}
		} else{
//状態コード送信(Cansel)
			snd1byte(0x01);
		}
	}else{
//状態コード送信(Error)
				snd1byte(0xf1);
	}
}

//FILE RENAME
void MZ80K_SD::f_ren(void){

//現ファイルネーム取得 
	for (unsigned int lp1 = 0;lp1 <= 32;lp1++){
		f_name[lp1] = rcv1byte();
	}
	addmzt(f_name);

//ファイルが存在しなければERROR
	if (FILEIO::IsFileExisting(f_name) == true){
//状態コード送信(OK)
		snd1byte(0x00);

//新ファイルネーム取得 
		for (unsigned int lp1 = 0;lp1 <= 32;lp1++){
			new_name[lp1] = rcv1byte();
		}
		addmzt(new_name);
//状態コード送信(OK)
		snd1byte(0x00);

		if (FILEIO::RenameFile(f_name, new_name)){
 //状態コード送信(OK)
		 snd1byte(0x00);
		} else {
 //状態コード送信(OK)
			snd1byte(0xff);
		}
	}else{
//状態コード送信(Error)
			snd1byte(0xf1);
	}
}

//FILE DUMP
void MZ80K_SD::f_dump(void){
	unsigned int br_chk =0;

//ファイルネーム取得 
	for (unsigned int lp1 = 0;lp1 <= 32;lp1++){
		f_name[lp1] = rcv1byte();
	}
	addmzt(f_name);

//ファイルが存在しなければERROR
	if (FILEIO::IsFileExisting(create_local_path(f_name)) == true){
//状態コード送信(OK)
		snd1byte(0x00);

//ファイルオープン
		FILEIO* file = new FILEIO();
		bool result = file->Fopen( create_local_path(f_name), FILEIO_READ_BINARY );
		if( true == result ){
//実データ送信(1画面:128Byte)
			unsigned int f_length = file->FileLength();
			long lp1 = 0;
			while (lp1 <= (long)f_length-1){
//画面先頭ADRSを送信 
				snd1byte(lp1 % 256);
				snd1byte((byte)(lp1 / 256));
				int i=0;
//実データを送信 
				while(i<128 && lp1<=(long)f_length-1){
					snd1byte(file->Fgetc());
					i++;
					lp1++;
				}
//FILE ENDが128Byteに満たなかったら残りByteに0x00を送信 
				while(i<128){
					snd1byte(0x00);
					i++;
				}
//指示待ち 
				br_chk=rcv1byte();
//BREAKならポインタをFILE ENDとする 
				if (br_chk==0xff){
					lp1 = f_length; 
				}
//B:BACKを受信したらポインタを256Byte戻す。先頭画面なら0に戻してもう一度先頭画面表示 
				if (br_chk==0x42){
					if(lp1>255){
						if (lp1 % 128 == 0){
							lp1 = lp1 - 256;
						} else {
							lp1 = lp1 - 128 - (lp1 % 128);
						}
						file->Fseek(lp1, FILEIO_SEEK_SET);
					} else{
						lp1 = 0;
						file->Fseek(0, FILEIO_SEEK_SET);
					}
				}
			}
//FILE ENDもしくはBREAKならADRSに終了コード0FFFFHを送信 
			if (lp1 > (long)f_length-1){
				snd1byte(0xff);
				snd1byte(0xff);
			};
			file->Fclose();
//状態コード送信(OK)
			snd1byte(0x00);
		} else {
//状態コード送信(ERROR)
			snd1byte(0xF1);
		}
		delete file;
	}else{
//状態コード送信(Error)
		snd1byte(0xf1);
	}
}

//FILE COPY
void MZ80K_SD::f_copy(void){

//現ファイルネーム取得 
	for (unsigned int lp1 = 0;lp1 <= 32;lp1++){
		f_name[lp1] = rcv1byte();
	}
	addmzt(f_name);
//ファイルが存在しなければERROR
	if (FILEIO::IsFileExisting(create_local_path(f_name)) == true){
//状態コード送信(OK)
		snd1byte(0x00);

//新ファイルネーム取得 
		for (unsigned int lp1 = 0;lp1 <= 32;lp1++){
			new_name[lp1] = rcv1byte();
		}
		addmzt(new_name);
//新ファイルネームと同じファイルネームが存在すればERROR
		if (FILEIO::IsFileExisting(create_local_path(new_name)) == false){
//状態コード送信(OK)
			snd1byte(0x00);
//ファイルオープン 
			FILEIO* file_r = new FILEIO();
			FILEIO* file_w = new FILEIO();
			bool result_r = file_r->Fopen( create_local_path(f_name), FILEIO_READ_BINARY );
			file_w->Fopen( new_name, FILEIO_WRITE_BINARY );
			if( true == result_r ){
//実データコピー 
				unsigned int f_length = file_r->FileLength();
				long lp1 = 0;
				while (lp1 <= (long)f_length-1){
					int i=0;
					while(i<=255 && lp1<=(long)f_length-1){
						s_data[i]=file_r->Fgetc();
						i++;
						lp1++;
					}
					file_w->Fwrite(s_data,i,1);
				}
				file_w->Fclose();
				file_r->Fclose();
//状態コード送信(OK)
				snd1byte(0x00);
			}else{
//状態コード送信(Error)
				snd1byte(0xf1);
			}
			delete file_r;
			delete file_w;
		}else{
//状態コード送信(Error)
			snd1byte(0xf3);
		}
	}else{
//状態コード送信(Error)
			snd1byte(0xf1);
	}
}

//91hで0436H MONITOR ライト インフォメーション代替処理 
void MZ80K_SD::mon_whead(void){
	char m_info[130];
//インフォメーションブロック受信 
	for (unsigned int lp1 = 0;lp1 < 128;lp1++){
		m_info[lp1] = rcv1byte();
	}
//S-OS SWORDからは最後が20hのファイルネームが送られて来るため0dhを付加 
//8080用テキスト・エディタ＆アセンブラからファイルネームの後ろに20hが送られて来るため0dhに修正 
//MZ-80K側で対処 
//	int lp2 = 17;
//	while (lp2>0 && (m_info[lp2] ==0x20 || m_info[lp2] ==0x0d)){
//		m_info[lp2]=0x0d;
//		lp2--;
//	}
//ファイルネーム取り出し 
	for (unsigned int lp1 = 0;lp1 < 17;lp1++){
		m_name[lp1] = m_info[lp1+1];
	}
//DOSファイルネーム用に.MZTを付加 
	addmzt(m_name);
	m_info[16] = 0x0d;
//ファイルが存在すればdelete
	if (FILEIO::IsFileExisting(create_local_path(m_name)) == true){
			FILEIO::RemoveFile(create_local_path(m_name));
	}
//ファイルオープン 
	FILEIO* file = new FILEIO();
	bool result = file->Fopen( create_local_path(m_name), FILEIO_WRITE_BINARY );
	if( true == result ){
//状態コード送信(OK)
		snd1byte(0x00);
//インフォメーションブロックwrite
		for (unsigned int lp1 = 0;lp1 < 128;lp1++){
			file->Fputc(m_info[lp1]);
		}
		file->Fclose();
	} else {
//状態コード送信(ERROR)
		snd1byte(0xF1);
	}
	delete file;
}

//92hで0475H MONITOR ライト データ代替処理 
void MZ80K_SD::mon_wdata(void){
//ファイルサイズ取得 
	int f_length1 = rcv1byte();
	int f_length2 = rcv1byte();
//ファイルサイズ算出 
	unsigned int f_length = f_length1+f_length2*256;
//ファイルオープン 
	FILEIO* file = new FILEIO();
	bool result = file->Fopen( create_local_path(m_name), FILEIO_READ_WRITE_BINARY );
	if( true == result ){
//状態コード送信(OK)
		snd1byte(0x00);
//ヘッダを飛ばす
 		file->Fseek(128, FILEIO_SEEK_SET);
//実データ
		long lp1 = 0;
		while (lp1 <= (long)f_length-1){
			int i=0;
			while(i<=255 && lp1<=(long)f_length-1){
				s_data[i]=rcv1byte();
				i++;
				lp1++;
			}
			file->Fwrite(s_data,i,1);
		}
		file->Fclose();
	} else {
//状態コード送信(ERROR)
		snd1byte(0xF1);
	}
	delete file;
}

//04D8H MONITOR リード インフォメーション代替処理 
void MZ80K_SD::mon_lhead(void){
//リード データ POINTクリア 
	m_lop=128;
//ファイルネーム取得 
	for (unsigned int lp1 = 0;lp1 <= 32;lp1++){
		m_name[lp1] = rcv1byte();
	}
	addmzt(m_name);
//ファイルが存在しなければERROR
	if (FILEIO::IsFileExisting(create_local_path(m_name)) == true){
		snd1byte(0x00);
//ファイルオープン
		FILEIO* file = new FILEIO();
		bool result = file->Fopen( create_local_path(m_name), FILEIO_READ_BINARY );
		if( true == result ){
			snd1byte(0x00);
			for (unsigned int lp1 = 0;lp1 < 128;lp1++){
					byte i_data = file->Fgetc();
					snd1byte(i_data);
			}
			file->Fclose();
			snd1byte(0x00);
		} else {
//状態コード送信(ERROR)
			snd1byte(0xFF);
		}
		delete file;
	} else {
//状態コード送信(FILE NOT FIND ERROR)
		snd1byte(0xF1);
	}
}

//04F8H MONITOR リード データ代替処理 
void MZ80K_SD::mon_ldata(void){
	addmzt(m_name);
//ファイルが存在しなければERROR
	if (FILEIO::IsFileExisting(create_local_path(m_name)) == true){
		snd1byte(0x00);
//ファイルオープン 
		FILEIO* file = new FILEIO();
		bool result = file->Fopen( create_local_path(m_name), FILEIO_READ_BINARY );
		if( true == result ){
			snd1byte(0x00);
			file->Fseek(m_lop, FILEIO_SEEK_SET);
//読み出しサイズ取得 
			int f_length2 = rcv1byte();
			int f_length1 = rcv1byte();
			unsigned int f_length = f_length1*256+f_length2;
			for (unsigned int lp1 = 0;lp1 < f_length;lp1++){
				byte i_data = file->Fgetc();
				snd1byte(i_data);
			}
			file->Fclose();
			m_lop=m_lop+f_length;
			snd1byte(0x00);
		} else {
//状態コード送信(ERROR)
			snd1byte(0xFF);
		}
		delete file;
	} else {
//状態コード送信(FILE NOT FIND ERROR)
		snd1byte(0xF1);
	}
}

//BOOT処理(MZ-2000_SD専用)
void MZ80K_SD::boot(void){
//ファイルネーム取得
	for (unsigned int lp1 = 0;lp1 <= 32;lp1++){
		m_name[lp1] = rcv1byte();
	}
////	Serial.print("m_name:");
////	Serial.println(m_name);
//ファイルが存在しなければERROR
	if (FILEIO::IsFileExisting(create_local_path(m_name)) == true){
		snd1byte(0x00);
//ファイルオープン 
		FILEIO* file = new FILEIO();
		bool result = file->Fopen( create_local_path(m_name), FILEIO_READ_BINARY );
		if( true == result ){
		snd1byte(0x00);
//ファイルサイズ送信 
			unsigned long f_length = file->FileLength();
			unsigned int f_len1 = f_length / 256;
			unsigned int f_len2 = f_length % 256;
			snd1byte(f_len2);
			snd1byte(f_len1);
////	Serial.println(f_length,HEX);
////	Serial.println(f_len2,HEX);
////	Serial.println(f_len1,HEX);

//実データ送信 
			for (unsigned long lp1 = 1;lp1 <= f_length;lp1++){
				 byte i_data = file->Fgetc();
				 snd1byte(i_data);
			}

		} else {
//状態コード送信(ERROR)
			snd1byte(0xFF);
		}
		delete file;
	} else {
//状態コード送信(FILE NOT FIND ERROR)
		snd1byte(0xF1);
	}
}

// 連結ファイルオープン 
// 0xE0, ファイルネーム(33bytes)
// Result: 0x00:OK, 0xF1:FILE NOT FIND ERROR, 0xFF:ERROR
void MZ80K_SD::ConcatFileOpen()
{
	if(isConcatState == 1)
	{
		concatFile->Fclose();
	}
	// ファイルネーム取得 
	for (unsigned int lp1 = 0; lp1 <= 32; lp1 ++) {
		concatName[lp1] = rcv1byte();
	}
	addmzt(concatName);
	// ファイルが存在するか、しなければERROR
	if (FILEIO::IsFileExisting(create_local_path(concatName)) == true) {
		//ファイルオープン 
		bool result = concatFile->Fopen( create_local_path(concatName), FILEIO_READ_BINARY );
		if ( true == result ) {
			concatSize = concatFile->FileLength();
			//状態コード送信(OK)
			snd1byte(0x00);
			isConcatState = 1; // オープンしている 
			concatPos = 0;
		} else {
			//状態コード送信(ERROR)
			snd1byte(0xFF);
		}
	} else {
		// 状態コード送信(FILE NOT FIND ERROR)
		snd1byte(0xF1);
	}
}

// 連結ファイルを1ブロック読み込む 
// 0xE1
// Result:		17bytes: MZファイル名 
//						2bytes:	読み込みアドレス 
//						2bytes:	データサイズ 
//						2bytes:	実行アドレス 
// データサイズbytes:	データ 
//						1byte:	 ステータス: 0xFE:次のデータがある, 0x00次のデータが無い(終了)
void MZ80K_SD::ConcatFileRead()
{
	if(isConcatState == 0)
	{
		// オープンしていない 
		for (unsigned int i = 0; i < 17; i ++) {
			snd1byte(0);
		}
		snd1byte(0xB1);
		for (unsigned int i = 0; i < 6; i ++) {
			snd1byte(0);
		}
		return;
	}
	// モード読み捨て 
	int wk1 = concatFile->Fgetc();
	// ファイル名 
	for (unsigned int lp1 = 0;lp1 <= 16; lp1 ++) {
		wk1 = concatFile->Fgetc();
		snd1byte(wk1);
	}
	//データサイズ取得 
	int f_length2 = concatFile->Fgetc();
	int f_length1 = concatFile->Fgetc();
	unsigned int f_length = f_length1*256+f_length2;
	//スタートアドレス取得 
	int s_adrs2 = concatFile->Fgetc();
	int s_adrs1 = concatFile->Fgetc();
	unsigned int s_adrs = s_adrs1*256+s_adrs2;
	//実行アドレス取得 
	int g_adrs2 = concatFile->Fgetc();
	int g_adrs1 = concatFile->Fgetc();
	unsigned int g_adrs = g_adrs1*256+g_adrs2;
	snd1byte(s_adrs2);
	snd1byte(s_adrs1);
	snd1byte(f_length2);
	snd1byte(f_length1);
	snd1byte(g_adrs2);
	snd1byte(g_adrs1);
	concatPos += 128;
	concatFile->Fseek(concatPos, FILEIO_SEEK_SET);
	//データ送信 
	for (unsigned int lp1 = 0;lp1 < f_length; lp1 ++) {
		byte i_data = concatFile->Fgetc();
		snd1byte(i_data);
		++ concatPos;
	}
	if (concatPos < concatSize) {
		// 次のデータがある 
		snd1byte(0xFE);
	} else {
		// 次のデータが無い 
		snd1byte(0x00);
	}
}

// 連結ファイルを1ブロックスキップ 
// 0xE2
// Result: 0xFE:次のデータがある, 0x00:次のデータが無い(終了), 0xFF:オープンしていない 
void MZ80K_SD::ConcatFileSkip()
{
	if(isConcatState == 0)
	{
		// オープンしていない 
		snd1byte(0xFF);
		return;
	}
	// モード読み捨て 
	int wk1 = concatFile->Fgetc();
	// ファイル名 
	for(unsigned int lp1 = 0;lp1 <= 16; lp1 ++) {
		wk1 = concatFile->Fgetc();
	}
	//データサイズ取得 
	int f_length2 = concatFile->Fgetc();
	int f_length1 = concatFile->Fgetc();
	unsigned int f_length = f_length1*256+f_length2;
	concatPos += (128 + f_length);
	concatFile->Fseek(concatPos, FILEIO_SEEK_SET);
	if (concatPos < concatSize) {
		// 次のデータがある 
		snd1byte(0xFE);
	} else {
		// 次のデータが無い 
		snd1byte(0x00);
	}
}

// 連結ファイルからブロックを探す 
// 0xE3, MZファイルネーム(17bytes)
// Result: 0x00:OK, 0xF1:FILE NOT FIND, 0xFF:オープンしていない 
void MZ80K_SD::ConcatFileFind()
{
	if(isConcatState == 0) {
		snd1byte(0xFF);
		return;
	}
	unsigned long concatPosBackup = concatPos;
	// ファイル名 
	for (unsigned int lp1 = 0;lp1 < 17; lp1 ++) {
		f_name[lp1] = rcv1byte();
		if(f_name[lp1] == 0x0D) {
			f_name[lp1] = 0;
		}
	}
	f_name[17] = 0;
	snd1byte(0x00);
	// 検索する 
	while(1) {
		// モード読み捨て 
		int wk1 = concatFile->Fgetc();
		// ファイル名 
		for (unsigned int lp1 = 0;lp1 < 17; lp1 ++) {
			m_name[lp1] = concatFile->Fgetc();
			if(m_name[lp1] == 0x0D) {
				m_name[lp1] = 0;
			}
		}
		m_name[17] = 0;
		//データサイズ取得 
		int f_length2 = concatFile->Fgetc();
		int f_length1 = concatFile->Fgetc();
		unsigned int f_length = f_length1*256+f_length2;
		if(std::string(f_name).compare(m_name) == 0) {
			// 見つけた 
			concatFile->Fseek(concatPos, FILEIO_SEEK_SET);
			break;
		} else {
			// 次のファイル 
			concatPos += (128 + f_length);
			if (concatPos >= concatSize) {
				// 見つからなかった 
				concatFile->Fseek(concatPosBackup, FILEIO_SEEK_SET);
				snd1byte(0xF1);
				return;
			}
			concatFile->Fseek(concatPos, FILEIO_SEEK_SET);
		}
	}
	snd1byte(0x00);
}

// 連結ファイルのトップに戻る 
// 0xE4
// Result: 0x00:OK, 0xF1:FILE NOT FIND ERROR, 0xFF:オープンしていない 
void MZ80K_SD::ConcatFileTop()
{
	if(isConcatState == 0) {
		snd1byte(0xFF);
		return;
	}
	concatPos = 0;
	concatFile->Fseek(concatPos, FILEIO_SEEK_SET);
	snd1byte(0x00);
}

// 連結ファイルクローズ 
// 0xE5
// Result: 0x00:OK, 0xF1:FILE NOT FIND ERROR, 0xFF:オープンしていない 
void MZ80K_SD::ConcatFileClose()
{
	if(isConcatState == 0) {
		snd1byte(0xFF);
		return;
	} else if(isConcatState == 1) {
		concatFile->Fclose();
	}
	isConcatState = 0;
	snd1byte(0x00);
}

// 連結ファイルの次のデータがあるか 
// 0xE6
// Result: 0xFE:次のデータがある, 0x00:次のデータが無い(終了) 0xFF:オープンしていない 
void MZ80K_SD::ConcatFileState(void)
{
	if(isConcatState == 0) {
		// オープンしていない 
		snd1byte(0xFF);
	} else if (concatPos < concatSize) {
		// 次のデータがある 
		snd1byte(0xFE);
	} else {
		// 次のデータが無い 
		snd1byte(0x00);
	}
}

// MIDIコマンド送信 
// 0xE7, 送信バイト数, 送信データ 
// Result:	0x00:エラー無し, 0xFF:エラー 
void MZ80K_SD::SendMidi(void)
{
	byte length = rcv1byte();
	for(byte i = 0; i < length; i ++)
	{
		byte data = rcv1byte();
		// todo To MIDI
//		Serial.write(data);
	}
}

void MZ80K_SD::loop()
{
	try
	{
		digitalWrite(PB0PIN,LOW);
		digitalWrite(PB1PIN,LOW);
		digitalWrite(PB2PIN,LOW);
		digitalWrite(PB3PIN,LOW);
		digitalWrite(PB4PIN,LOW);
		digitalWrite(PB5PIN,LOW);
		digitalWrite(PB6PIN,LOW);
		digitalWrite(PB7PIN,LOW);
		digitalWrite(FLGPIN,LOW);
	//コマンド取得待ち 
	////	Serial.print("cmd:");
		byte cmd = rcv1byte();

	////	Serial.println(cmd,HEX);
		if((cmd < 0xE0) && (isConcatState == 1))
		{
			// 連結ファイルオープン中に通常ファイルコマンドが来たので連結ファイルはクローズする 
			concatFile->Fclose();
			isConcatState = 0;
		}
		if (eflg == false){
			switch(cmd) {
	//80hでSDカードにsave
				case 0x80:
	////	Serial.println("SAVE START");
	//状態コード送信(OK)
					snd1byte(0x00);
					f_save();
					break;
	//81hでSDカードからload
				case 0x81:
	////	Serial.println("LOAD START");
	//状態コード送信(OK)
					snd1byte(0x00);
					f_load();
					break;
	//82hで指定ファイルを0000.mztとしてリネームコピー 
				case 0x82:
	////	Serial.println("ASTART START");
	//状態コード送信(OK)
					snd1byte(0x00);
					astart();
					break;
	//83hでファイルリスト出力 
				case 0x83:
	////	Serial.println("FILE LIST START");
	//状態コード送信(OK)
					snd1byte(0x00);
					dirlist();
					break;
	//84hでファイルDelete
				case 0x84:
	////	Serial.println("FILE Delete START");
	//状態コード送信(OK)
					snd1byte(0x00);
					f_del();
					break;
	//85hでファイルリネーム 
				case 0x85:
	////	Serial.println("FILE Rename START");
	//状態コード送信(OK)
					snd1byte(0x00);
					f_ren();
					break;
				case 0x86:	
	//86hでファイルダンプ 
	////	Serial.println("FILE Dump START");
	//状態コード送信(OK)
					snd1byte(0x00);
					f_dump();
					break;
				case 0x87:	
	//87hでファイルコピー 
	////	Serial.println("FILE Copy START");
	//状態コード送信(OK)
					snd1byte(0x00);
					f_copy();
					break;
				case 0x91:
	//91hで0436H MONITOR ライト インフォメーション代替処理 
	////	Serial.println("0436H START");
	//状態コード送信(OK)
					snd1byte(0x00);
					mon_whead();
					break;
	//92hで0475H MONITOR ライト データ代替処理 
				case 0x92:
	////	Serial.println("0475H START");
	//状態コード送信(OK)
					snd1byte(0x00);
					mon_wdata();
					break;
	//93hで04D8H MONITOR リード インフォメーション代替処理 
				case 0x93:
	////	Serial.println("04D8H START");
	//状態コード送信(OK)
					snd1byte(0x00);
					mon_lhead();
					break;
	//94hで04F8H MONITOR リード データ代替処理 
				case 0x94:
	////	Serial.println("04F8H START");
	//状態コード送信(OK)
					snd1byte(0x00);
					mon_ldata();
					break;
	//95hでBOOT LOAD(MZ-2000_SD専用)
				case 0x95:
	////	Serial.println("BOOT LOAD START");
	//状態コード送信(OK)
					snd1byte(0x00);
					boot();
					break;

	//連結ファイルコマンド 
	//0E0hで連結ファイルオープン 
				case 0xE0:
	//状態コード送信(OK)
					snd1byte(0x00);
					ConcatFileOpen();
					break;
	//0E1hで連結ファイル1ブロック読み込み 
				case 0xE1:
	//状態コード送信(OK)
					snd1byte(0x00);
					ConcatFileRead();
					break;
	//0E2hで連結ファイルを1ブロックスキップ 
				case 0xE2:
	//状態コード送信(OK)
					snd1byte(0x00);
					ConcatFileSkip();
					break;
	//0E3hで連結ファイルからブロックを探す 
				case 0xE3:
	//状態コード送信(OK)
					snd1byte(0x00);
					ConcatFileFind();
					break;
	//0E4hで連結ファイルのトップに戻る 
				case 0xE4:
	//状態コード送信(OK)
					snd1byte(0x00);
					ConcatFileTop();
					break;
	//0E5hで連結ファイルクローズ 
				case 0xE5:
	//状態コード送信(OK)
					snd1byte(0x00);
					ConcatFileClose();
					break;
	//0E6hで連結ファイルの次のデータがあるか 
				case 0xE6:
	//状態コード送信(OK)
					snd1byte(0x00);
					ConcatFileState();
					break;

	//0E7hでMIDIコマンド送信 
				case 0xE7:
	//状態コード送信(OK)
					snd1byte(0x00);
					SendMidi();
					break;

				default:
	//状態コード送信(CMD ERROR)
					snd1byte(0xF4);
			}
		} else {
	//状態コード送信(ERROR)
			snd1byte(0xF0);
		}
	}
	catch(char* e)
	{
		(void)e;
	}
}

unsigned __stdcall MZ80K_SD::loop_thread(void* param)
{
	MZ80K_SD* mz80k_sd = (MZ80K_SD*)param;
	try
	{
		while(!mz80k_sd->terminate) {
			mz80k_sd->loop();
		}
	}
	catch(...)
	{
	}
	_endthreadex(0);
	return 0;
}
