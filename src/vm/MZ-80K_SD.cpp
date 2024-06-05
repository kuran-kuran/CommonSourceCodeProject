// 2022. 1.24 �t�@�C���l�[���̌���20h�l�߂�0dh�ɏC�����邽�߂̏�����Arduino������MZ-80K���ɏC��
//           ��r���Z�q�̋L�q��������
// 2022. 1.25 �e�R�}���h��M����delay()��p�~
// 2022. 1.26 FD�R�}���h�Ń��[�h�\�ȃt�@�C����ރR�[�h��0x01�݂̂Ƃ��Ă���������P�p
// 2022. 1.29 FDP�R�}���h��bug�C��
// 2022. 1.30 FDL�R�}���h�d�l�ύX FDL A�`Z�̏ꍇ�A�t�@�C�����擪�ꕶ�����r���Ĉ�v�������̂������o��
// 2022. 1.31 FDL�R�}���h�d�l�ύX FDL x�̏ꍇ�A�t�@�C�����擪�ꕶ�����r���Ĉ�v�������̂������o��
//            B�L�[�őO��20����\��
// 2022. 2. 2 DOS�t�@�C�������A���t�@�x�b�g�������ł�FDL x�Ō����ł���悤�ɏC��
// 2022. 2. 4 MZ-1200�΍�@����������delay(1000)��ǉ�
// 2022. 2. 8 FDL�R�}���h�d�l�ύX FDL x�̏ꍇ�A�t�@�C�����擪1�����`32�����܂łɊg��
// 2023. 6.19 MZ-2000_SD�̋N�������ǉ��ɂ��BOOT LOADER�ǂݍ��݂�ǉ��BMZ-80K_SD�ɂ͉e���Ȃ��B
//
// �ȉ�kuran_kuran���ǉ�
// 2023.12.12 �A���t�@�C���Ή�
// 2024.01.21 �A���t�@�C���I�[�v�����̒ʏ�t�@�C���A�N�Z�X�ɑΉ�
// 2024.05.08 MIDI�R�}���h���M�ɑΉ�
// 2024.06.05 �G�~�����[�^�p�ɏC������
//
//#include "SdFat.h"
//#include <SPI.h>

#include "vm.h"
#include "../emu.h"
#include "device.h"

typedef unsigned char byte;
#define LOW 0
#define HIGH 1

//SdFat SD;
unsigned long m_lop=128;
char m_name[40];
byte s_data[260];
char f_name[40];
char c_name[40];
char new_name[40];
// �A���t�@�C��
char concatName[40];
bool isConcatState = 0; // 0:���g�p, 1:�I�[�v�����Ă���
FILEIO* concatFile;
unsigned long concatPos = 0;
unsigned long concatSize = 0;

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
// �t�@�C�����́A�����O�t�@�C���l�[���`���Ή�
boolean eflg;

void digitalWrite(int pin, int data)
{
}

int digitalRead(int pin)
{
    return 0;
}

void setup(){
//  Serial.begin(31250); // MIDI

////  Serial.begin(9600);
// CS=pin10
// pin10 output

/*
  pinMode(CABLESELECTPIN,OUTPUT);
  pinMode( CHKPIN,INPUT);  //CHK
  pinMode( PB0PIN,OUTPUT); //���M�f�[�^
  pinMode( PB1PIN,OUTPUT); //���M�f�[�^
  pinMode( PB2PIN,OUTPUT); //���M�f�[�^
  pinMode( PB3PIN,OUTPUT); //���M�f�[�^
  pinMode( PB4PIN,OUTPUT); //���M�f�[�^
  pinMode( PB5PIN,OUTPUT); //���M�f�[�^
  pinMode( PB6PIN,OUTPUT); //���M�f�[�^
  pinMode( PB7PIN,OUTPUT); //���M�f�[�^
  pinMode( FLGPIN,OUTPUT); //FLG

  pinMode( PA0PIN,INPUT_PULLUP); //��M�f�[�^
  pinMode( PA1PIN,INPUT_PULLUP); //��M�f�[�^
  pinMode( PA2PIN,INPUT_PULLUP); //��M�f�[�^
  pinMode( PA3PIN,INPUT_PULLUP); //��M�f�[�^
*/

  digitalWrite(PB0PIN,LOW);
  digitalWrite(PB1PIN,LOW);
  digitalWrite(PB2PIN,LOW);
  digitalWrite(PB3PIN,LOW);
  digitalWrite(PB4PIN,LOW);
  digitalWrite(PB5PIN,LOW);
  digitalWrite(PB6PIN,LOW);
  digitalWrite(PB7PIN,LOW);
  digitalWrite(FLGPIN,LOW);

// 2022. 2. 4 MZ-1200�΍�
  Sleep(1500);

  // SD������
//  if( !SD.begin(CABLESELECTPIN,8) )
  {
////    Serial.println("Failed : SD.begin");
    eflg = true;
//  } else {
////    Serial.println("OK : SD.begin");
//    eflg = false;
  }
////  Serial.println("START");
}

//4BIT��M
byte rcv4bit(void){
//HIGH�ɂȂ�܂Ń��[�v
  while(digitalRead(CHKPIN) != HIGH){
  }
//��M
  byte j_data = digitalRead(PA0PIN)+digitalRead(PA1PIN)*2+digitalRead(PA2PIN)*4+digitalRead(PA3PIN)*8;
//FLG���Z�b�g
  digitalWrite(FLGPIN,HIGH);
//LOW�ɂȂ�܂Ń��[�v
  while(digitalRead(CHKPIN) == HIGH){
  }
//FLG�����Z�b�g
  digitalWrite(FLGPIN,LOW);
  return(j_data);
}

//1BYTE��M
byte rcv1byte(void){
  byte i_data = 0;
  i_data=rcv4bit()*16;
  i_data=i_data+rcv4bit();
  return(i_data);
}

//1BYTE���M
void snd1byte(byte i_data){
//���ʃr�b�g����8�r�b�g�����Z�b�g
  digitalWrite(PB0PIN,(i_data)&0x01);
  digitalWrite(PB1PIN,(i_data>>1)&0x01);
  digitalWrite(PB2PIN,(i_data>>2)&0x01);
  digitalWrite(PB3PIN,(i_data>>3)&0x01);
  digitalWrite(PB4PIN,(i_data>>4)&0x01);
  digitalWrite(PB5PIN,(i_data>>5)&0x01);
  digitalWrite(PB6PIN,(i_data>>6)&0x01);
  digitalWrite(PB7PIN,(i_data>>7)&0x01);
  digitalWrite(FLGPIN,HIGH);
//HIGH�ɂȂ�܂Ń��[�v
  while(digitalRead(CHKPIN) != HIGH){
  }
  digitalWrite(FLGPIN,LOW);
//LOW�ɂȂ�܂Ń��[�v
  while(digitalRead(CHKPIN) == HIGH){
  }
}

//������->�啶��
char upper(char c){
  if('a' <= c && c <= 'z'){
    c = c - ('a' - 'A');
  }
  return c;
}

//�t�@�C�����̍Ōオ�u.mzt�v�łȂ���Εt��
void addmzt(char *f_name){
  unsigned int lp1=0;
  while (f_name[lp1] != 0x0D){
    lp1++;
  }
  if (f_name[lp1-4]!='.' ||
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

//SD�J�[�h��SAVE
void f_save(void){
char p_name[20];

//�ۑ��t�@�C���l�[���擾
  for (unsigned int lp1 = 0;lp1 <= 32;lp1++){
    f_name[lp1] = rcv1byte();
  }
  addmzt(f_name);
//�v���O�����l�[���擾
  for (unsigned int lp1 = 0;lp1 <= 16;lp1++){
    p_name[lp1] = rcv1byte();
  }
  p_name[15] =0x0D;
  p_name[16] =0x00;
//�X�^�[�g�A�h���X�擾
  int s_adrs1 = rcv1byte();
  int s_adrs2 = rcv1byte();
//�X�^�[�g�A�h���X�Z�o
  unsigned int s_adrs = s_adrs1+s_adrs2*256;
//�G���h�A�h���X�擾
  int e_adrs1 = rcv1byte();
  int e_adrs2 = rcv1byte();
//�G���h�A�h���X�Z�o
  unsigned int e_adrs = e_adrs1+e_adrs2*256;
//���s�A�h���X�擾
  int g_adrs1 = rcv1byte();
  int g_adrs2 = rcv1byte();
//���s�A�h���X�Z�o
  unsigned int g_adrs = g_adrs1+g_adrs2*256;
//�t�@�C���T�C�Y�Z�o
  unsigned int f_length = e_adrs - s_adrs + 1;
  unsigned int f_length1 = f_length % 256;
  unsigned int f_length2 = f_length / 256;
//�t�@�C�������݂����delete
  if (FILEIO::IsFileExisting(f_name) == true){
      FILEIO::RemoveFile(f_name);
  }
//�t�@�C���I�[�v��
  FILEIO* file = new FILEIO();
  bool result = file->Fopen( f_name, FILEIO_WRITE_BINARY );
  if( true == result ){
//��ԃR�[�h���M(OK)
    snd1byte(0x00);
//�t�@�C�����[�h�ݒ�(01)
    file->Fputc(char(0x01));
//�v���O�����l�[��
    file->Fwrite(p_name, sizeof(p_name), 1);
    file->Fputc(char(0x00));
//�t�@�C���T�C�Y
    file->Fputc(f_length1);
    file->Fputc(f_length2);
//�X�^�[�g�A�h���X
    file->Fputc(s_adrs1);
    file->Fputc(s_adrs2);
//���s�A�h���X
    file->Fputc(g_adrs1);
    file->Fputc(g_adrs2);
//7F�܂�00����
    for (unsigned int lp1 = 0;lp1 <= 103;lp1++){
      file->Fputc(char(0x00));
    }
//���f�[�^
    long lp1 = 0;
    while (lp1 <= f_length-1){
      int i=0;
      while(i<=255 && lp1<=f_length-1){
        s_data[i]=rcv1byte();
        i++;
        lp1++;
      }
      file->Fwrite(s_data, i, 1);
    }
    file->Fclose();
   } else {
//��ԃR�[�h���M(ERROR)
    snd1byte(0xF1);
  }
}

//SD�J�[�h����Ǎ�
void f_load(void){
//�t�@�C���l�[���擾
  for (unsigned int lp1 = 0;lp1 <= 32;lp1++){
    f_name[lp1] = rcv1byte();
  }
  addmzt(f_name);
//�t�@�C�������݂��Ȃ����ERROR
  if (FILEIO::IsFileExisting(f_name) == true){
//�t�@�C���I�[�v��
    FILEIO* file = new FILEIO();
    bool result = file->Fopen( f_name, FILEIO_READ_BINARY );
    if( true == result ){
//�t�@�C����ރR�[�h�̔��ʂ�P�p
//      if( file.read() == 0x01){
//��ԃR�[�h���M(OK)
        snd1byte(0x00);
        int wk1 = 0;
        wk1 = file->Fgetc();
        for (unsigned int lp1 = 0;lp1 <= 16;lp1++){
          wk1 = file->Fgetc();
          snd1byte(wk1);
        }
//�t�@�C���T�C�Y�擾
        int f_length2 = file->Fgetc();
        int f_length1 = file->Fgetc();
        unsigned int f_length = f_length1*256+f_length2;
//�X�^�[�g�A�h���X�擾
        int s_adrs2 = file->Fgetc();
        int s_adrs1 = file->Fgetc();
        unsigned int s_adrs = s_adrs1*256+s_adrs2;
//���s�A�h���X�擾
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
//�f�[�^���M
        for (unsigned int lp1 = 0;lp1 < f_length;lp1++){
            byte i_data = file->Fgetc();
            snd1byte(i_data);
        }
        file->Fclose();
//       } else {
//��ԃR�[�h���M(ERROR)
//        snd1byte(0xF2);
//      }  
     } else {
//��ԃR�[�h���M(ERROR)
      snd1byte(0xFF);
     }
   } else {
//��ԃR�[�h���M(FILE NOT FIND ERROR)
    snd1byte(0xF1);
  }
}

//ASTART �w�肳�ꂽ�t�@�C�����t�@�C�����u0000.mzt�v�Ƃ��ăR�s�[
void astart(void){
char w_name[]="0000.mzt";

//�t�@�C���l�[���擾
  for (unsigned int lp1 = 0;lp1 <= 32;lp1++){
    f_name[lp1] = rcv1byte();
  }
  addmzt(f_name);
//�t�@�C�������݂��Ȃ����ERROR
  if (FILEIO::IsFileExisting(f_name) == true){
//0000.mzt�����݂����delete
    if (FILEIO::IsFileExisting(w_name) == true){
        FILEIO::RemoveFile(w_name);
    }
//�t�@�C���I�[�v��
    FILEIO* file_r = new FILEIO();
    FILEIO* file_w = new FILEIO();
    bool result_r = file_r->Fopen( f_name, FILEIO_READ_BINARY );
    file_w->Fopen( w_name, FILEIO_WRITE_BINARY );
      if( true == result_r ){
//���f�[�^
        unsigned int f_length = file_r->FileLength();
        long lp1 = 0;
        while (lp1 <= f_length-1){
          int i=0;
          while(i<=255 && lp1<=f_length-1){
            s_data[i]=file_r->Fgetc();
            i++;
            lp1++;
          }
          file_w->Fwrite(s_data,i,1);
        }
        file_w->Fclose();
        file_r->Fclose();
//��ԃR�[�h���M(OK)
        snd1byte(0x00);
      } else {
//��ԃR�[�h���M(ERROR)
      snd1byte(0xF1);
    }
  } else {
//��ԃR�[�h���M(ERROR)
    snd1byte(0xF1);
  }  
}

// SD-CARD��FILELIST
void dirlist(void){
//��r������擾 32+1�����܂�
  for (unsigned int lp1 = 0;lp1 <= 32;lp1++){
    c_name[lp1] = rcv1byte();
//  Serial.print(c_name[lp1],HEX);
//  Serial.println("");
  }
//
  File file = SD.open( "/" );
  File entry =  file.openNextFile();
  int cntl2 = 0;
  unsigned int br_chk =0;
  int page = 1;
//�S���o�͂̏ꍇ�ɂ�20���o�͂����Ƃ���ňꎞ��~�A�L�[���͂ɂ��p���A�ł��؂��I��
  while (br_chk == 0) {
    if(entry){
      entry.getName(f_name,36);
      unsigned int lp1=0;
//�ꌏ���M
//��r������Ńt�@�C���l�[����擪10�����܂Ŕ�r���Ĉ�v������̂������o��
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
//�p���E�ł��؂�I���w���v��
      snd1byte(0xfe);

//�I���w����M(0:�p�� B:�O�y�[�W �ȊO:�ł��؂�)
      br_chk = rcv1byte();
//�O�y�[�W����
      if (br_chk==0x42){
//�擪�t�@�C����
        file.rewindDirectory();
//entry�l�X�V
        entry =  file.openNextFile();
//������x�擪�t�@�C����
        file.rewindDirectory();
        if(page <= 2){
//���݃y�[�W��1�y�[�W����2�y�[�W�Ȃ�1�y�[�W�ڂɖ߂鏈��
          page = 0;
        } else {
//���݃y�[�W��3�y�[�W�ȍ~�Ȃ�O�X�y�[�W�܂ł̃t�@�C����ǂݔ�΂�
          page = page -2;
          cntl2=0;
          while(cntl2 < page*20){
            entry =  file.openNextFile();
//          if (upper(f_name[0]) == pg0 || pg0 == 0x20){
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
//�t�@�C�����܂�����Ȃ玟�ǂݍ��݁A�Ȃ���Αł��؂�w��
    if (entry){
      entry =  file.openNextFile();
    }else{
      br_chk=1;
    }
//FDL�̌��ʂ�20�������Ȃ�p���w���v�������ɂ��̂܂܏I��
    if (!entry && cntl2 < 20 && page ==1){
      break;
    }
  }
//�����I���w��
  snd1byte(0xFF);
  snd1byte(0x00);
}

//f_name��c_name��c_name��0x00���o��܂Ŕ�r
//FILENAME COMPARE
boolean f_match(char *f_name,char *c_name){
  boolean flg1 = true;
  unsigned int lp1 = 0;
//  Serial.print(f_name);
//  Serial.print(" ");
//  Serial.print(c_name);
//  Serial.print(" ");
  while (lp1 <=32 && c_name[0] != 0x00 && flg1 == true){
//  Serial.print(f_name[lp1],HEX);
//  Serial.print("-");
//  Serial.print(c_name[lp1+1],HEX);
//  Serial.print(" ");
    if (upper(f_name[lp1]) != c_name[lp1+1]){
      flg1 = false;
    }
    lp1++;
    if (c_name[lp1+1]==0x00){
      break;
    }
  }
//  if (flg1){
//    Serial.println("true");
//  }else{
//    Serial.println("false");
//  }
  return flg1;
}

//FILE DELETE
void f_del(void){

//�t�@�C���l�[���擾
  for (unsigned int lp1 = 0;lp1 <= 32;lp1++){
    f_name[lp1] = rcv1byte();
  }
  addmzt(f_name);

//�t�@�C�������݂��Ȃ����ERROR
  if (FILEIO::IsFileExisting(f_name) == true){
//��ԃR�[�h���M(OK)
    snd1byte(0x00);

//�����I������M(0:�p������DELETE 0�ȊO:CANSEL)
    if (rcv1byte() == 0x00){
      if (FILEIO::RemoveFile(f_name) == true){
//��ԃR�[�h���M(OK)
        snd1byte(0x00);
      }else{
//��ԃR�[�h���M(Error)
        snd1byte(0xf1);
      }
    } else{
//��ԃR�[�h���M(Cansel)
      snd1byte(0x01);
    }
  }else{
//��ԃR�[�h���M(Error)
        snd1byte(0xf1);
  }
}

//FILE RENAME
void f_ren(void){

//���t�@�C���l�[���擾
  for (unsigned int lp1 = 0;lp1 <= 32;lp1++){
    f_name[lp1] = rcv1byte();
  }
  addmzt(f_name);

//�t�@�C�������݂��Ȃ����ERROR
  if (FILEIO::IsFileExisting(f_name) == true){
//��ԃR�[�h���M(OK)
    snd1byte(0x00);

//�V�t�@�C���l�[���擾
    for (unsigned int lp1 = 0;lp1 <= 32;lp1++){
      new_name[lp1] = rcv1byte();
    }
    addmzt(new_name);
//��ԃR�[�h���M(OK)
    snd1byte(0x00);

    File file = SD.open( f_name, FILE_WRITE );
    if( true == file ){
      if (file.rename(new_name)){
 //��ԃR�[�h���M(OK)
         snd1byte(0x00);
        } else {
 //��ԃR�[�h���M(OK)
          snd1byte(0xff);
        }
      file.close();
    }else{
//��ԃR�[�h���M(Error)
      snd1byte(0xf1);
    }
  }else{
//��ԃR�[�h���M(Error)
      snd1byte(0xf1);
  }
}

//FILE DUMP
void f_dump(void){
unsigned int br_chk =0;

//�t�@�C���l�[���擾
  for (unsigned int lp1 = 0;lp1 <= 32;lp1++){
    f_name[lp1] = rcv1byte();
  }
  addmzt(f_name);

//�t�@�C�������݂��Ȃ����ERROR
  if (FILEIO::IsFileExisting(f_name) == true){
//��ԃR�[�h���M(OK)
    snd1byte(0x00);

//�t�@�C���I�[�v��
    File file = SD.open( f_name, FILE_READ );
      if( true == file ){
//���f�[�^���M(1���:128Byte)
        unsigned int f_length = file.size();
        long lp1 = 0;
        while (lp1 <= f_length-1){
//��ʐ擪ADRS�𑗐M
          snd1byte(lp1 % 256);
          snd1byte(lp1 / 256);
          int i=0;
//���f�[�^�𑗐M
          while(i<128 && lp1<=f_length-1){
            snd1byte(file.read());
            i++;
            lp1++;
          }
//FILE END��128Byte�ɖ����Ȃ�������c��Byte��0x00�𑗐M
          while(i<128){
            snd1byte(0x00);
            i++;
          }
//�w���҂�
          br_chk=rcv1byte();
//BREAK�Ȃ�|�C���^��FILE END�Ƃ���
          if (br_chk==0xff){
            lp1 = f_length; 
          }
//B:BACK����M������|�C���^��256Byte�߂��B�擪��ʂȂ�0�ɖ߂��Ă�����x�擪��ʕ\��
          if (br_chk==0x42){
            if(lp1>255){
              if (lp1 % 128 == 0){
                lp1 = lp1 - 256;
              } else {
                lp1 = lp1 - 128 - (lp1 % 128);
              }
              file.seek(lp1);
            } else{
              lp1 = 0;
              file.seek(0);
            }
          }
        }
//FILE END��������BREAK�Ȃ�ADRS�ɏI���R�[�h0FFFFH�𑗐M
        if (lp1 > f_length-1){
          snd1byte(0xff);
          snd1byte(0xff);
        };
        file.close();
//��ԃR�[�h���M(OK)
        snd1byte(0x00);
      } else {
//��ԃR�[�h���M(ERROR)
      snd1byte(0xF1);
    }
  }else{
//��ԃR�[�h���M(Error)
        snd1byte(0xf1);
  }
}

//FILE COPY
void f_copy(void){

//���t�@�C���l�[���擾
  for (unsigned int lp1 = 0;lp1 <= 32;lp1++){
    f_name[lp1] = rcv1byte();
  }
  addmzt(f_name);
//�t�@�C�������݂��Ȃ����ERROR
  if (FILEIO::IsFileExisting(f_name) == true){
//��ԃR�[�h���M(OK)
    snd1byte(0x00);

//�V�t�@�C���l�[���擾
    for (unsigned int lp1 = 0;lp1 <= 32;lp1++){
      new_name[lp1] = rcv1byte();
    }
    addmzt(new_name);
//�V�t�@�C���l�[���Ɠ����t�@�C���l�[�������݂����ERROR
    if (FILEIO::IsFileExisting(new_name) == false){
//��ԃR�[�h���M(OK)
        snd1byte(0x00);
//�t�@�C���I�[�v��
    File file_r = SD.open( f_name, FILE_READ );
    File file_w = SD.open( new_name, FILE_WRITE );
      if( true == file_r ){
//���f�[�^�R�s�[
        unsigned int f_length = file_r.size();
        long lp1 = 0;
        while (lp1 <= f_length-1){
          int i=0;
          while(i<=255 && lp1<=f_length-1){
            s_data[i]=file_r.read();
            i++;
            lp1++;
          }
          file_w.write(s_data,i);
        }
        file_w.close();
        file_r.close();
//��ԃR�[�h���M(OK)
        snd1byte(0x00);
      }else{
//��ԃR�[�h���M(Error)
      snd1byte(0xf1);
    }
      }else{
//��ԃR�[�h���M(Error)
        snd1byte(0xf3);
    }
  }else{
//��ԃR�[�h���M(Error)
      snd1byte(0xf1);
  }
}

//91h��0436H MONITOR ���C�g �C���t�H���[�V������֏���
void mon_whead(void){
char m_info[130];
//�C���t�H���[�V�����u���b�N��M
  for (unsigned int lp1 = 0;lp1 < 128;lp1++){
    m_info[lp1] = rcv1byte();
  }
//S-OS SWORD����͍Ōオ20h�̃t�@�C���l�[���������ė��邽��0dh��t��
//8080�p�e�L�X�g�E�G�f�B�^���A�Z���u������t�@�C���l�[���̌���20h�������ė��邽��0dh�ɏC��
//MZ-80K���őΏ�
//  int lp2 = 17;
//  while (lp2>0 && (m_info[lp2] ==0x20 || m_info[lp2] ==0x0d)){
//    m_info[lp2]=0x0d;
//    lp2--;
//  }
//�t�@�C���l�[�����o��
  for (unsigned int lp1 = 0;lp1 < 17;lp1++){
    m_name[lp1] = m_info[lp1+1];
  }
//DOS�t�@�C���l�[���p��.MZT��t��
  addmzt(m_name);
  m_info[16] = 0x0d;
//�t�@�C�������݂����delete
  if (FILEIO::IsFileExisting(m_name) == true){
      FILEIO::RemoveFile(m_name);
  }
//�t�@�C���I�[�v��
  File file = SD.open( m_name, FILE_WRITE );
  if( true == file ){
//��ԃR�[�h���M(OK)
    snd1byte(0x00);
//�C���t�H���[�V�����u���b�Nwrite
    for (unsigned int lp1 = 0;lp1 < 128;lp1++){
      file.write(m_info[lp1]);
    }
    file.close();
  } else {
//��ԃR�[�h���M(ERROR)
    snd1byte(0xF1);
  }
}

//92h��0475H MONITOR ���C�g �f�[�^��֏���
void mon_wdata(void){
//�t�@�C���T�C�Y�擾
  int f_length1 = rcv1byte();
  int f_length2 = rcv1byte();
//�t�@�C���T�C�Y�Z�o
  unsigned int f_length = f_length1+f_length2*256;
//�t�@�C���I�[�v��
  File file = SD.open( m_name, FILE_WRITE );
  if( true == file ){
//��ԃR�[�h���M(OK)
    snd1byte(0x00);
//���f�[�^
    long lp1 = 0;
    while (lp1 <= f_length-1){
      int i=0;
      while(i<=255 && lp1<=f_length-1){
        s_data[i]=rcv1byte();
        i++;
        lp1++;
      }
      file.write(s_data,i);
    }
    file.close();
  } else {
//��ԃR�[�h���M(ERROR)
    snd1byte(0xF1);
  }
}

//04D8H MONITOR ���[�h �C���t�H���[�V������֏���
void mon_lhead(void){
//���[�h �f�[�^ POINT�N���A
  m_lop=128;
//�t�@�C���l�[���擾
  for (unsigned int lp1 = 0;lp1 <= 32;lp1++){
    m_name[lp1] = rcv1byte();
  }
  addmzt(m_name);
//�t�@�C�������݂��Ȃ����ERROR
  if (FILEIO::IsFileExisting(m_name) == true){
    snd1byte(0x00);
//�t�@�C���I�[�v��
    File file = SD.open( m_name, FILE_READ );
    if( true == file ){
      snd1byte(0x00);
      for (unsigned int lp1 = 0;lp1 < 128;lp1++){
          byte i_data = file.read();
          snd1byte(i_data);
      }
      file.close();
      snd1byte(0x00);
    } else {
//��ԃR�[�h���M(ERROR)
      snd1byte(0xFF);
    }  
  } else {
//��ԃR�[�h���M(FILE NOT FIND ERROR)
    snd1byte(0xF1);
  }
}

//04F8H MONITOR ���[�h �f�[�^��֏���
void mon_ldata(void){
  addmzt(m_name);
//�t�@�C�������݂��Ȃ����ERROR
  if (FILEIO::IsFileExisting(m_name) == true){
    snd1byte(0x00);
//�t�@�C���I�[�v��
    File file = SD.open( m_name, FILE_READ );
    if( true == file ){
      snd1byte(0x00);
      file.seek(m_lop);
//�ǂݏo���T�C�Y�擾
      int f_length2 = rcv1byte();
      int f_length1 = rcv1byte();
      unsigned int f_length = f_length1*256+f_length2;
      for (unsigned int lp1 = 0;lp1 < f_length;lp1++){
        byte i_data = file.read();
        snd1byte(i_data);
      }
      file.close();
      m_lop=m_lop+f_length;
      snd1byte(0x00);
    } else {
//��ԃR�[�h���M(ERROR)
      snd1byte(0xFF);
    }  
  } else {
//��ԃR�[�h���M(FILE NOT FIND ERROR)
    snd1byte(0xF1);
  }
}

//BOOT����(MZ-2000_SD��p)
void boot(void){
//�t�@�C���l�[���擾
  for (unsigned int lp1 = 0;lp1 <= 32;lp1++){
    m_name[lp1] = rcv1byte();
  }
////  Serial.print("m_name:");
////  Serial.println(m_name);
//�t�@�C�������݂��Ȃ����ERROR
  if (FILEIO::IsFileExisting(m_name) == true){
    snd1byte(0x00);
//�t�@�C���I�[�v��
    File file = SD.open( m_name, FILE_READ );
    if( true == file ){
    snd1byte(0x00);
//�t�@�C���T�C�Y���M
      unsigned long f_length = file.size();
      unsigned int f_len1 = f_length / 256;
      unsigned int f_len2 = f_length % 256;
      snd1byte(f_len2);
      snd1byte(f_len1);
////  Serial.println(f_length,HEX);
////  Serial.println(f_len2,HEX);
////  Serial.println(f_len1,HEX);

//���f�[�^���M
      for (unsigned long lp1 = 1;lp1 <= f_length;lp1++){
         byte i_data = file.read();
         snd1byte(i_data);
      }

    } else {
//��ԃR�[�h���M(ERROR)
      snd1byte(0xFF);
    }  
  } else {
//��ԃR�[�h���M(FILE NOT FIND ERROR)
    snd1byte(0xF1);
  }
}

// �A���t�@�C���I�[�v��
// 0xE0, �t�@�C���l�[��(33bytes)
// Result: 0x00:OK, 0xF1:FILE NOT FIND ERROR, 0xFF:ERROR
void ConcatFileOpen()
{
  if(isConcatState == 1)
  {
    concatFile.close();
  }
  // �t�@�C���l�[���擾
  for (unsigned int lp1 = 0; lp1 <= 32; lp1 ++) {
    concatName[lp1] = rcv1byte();
  }
  addmzt(concatName);
  // �t�@�C�������݂��邩�A���Ȃ����ERROR
  if (FILEIO::IsFileExisting(concatName) == true) {
    //�t�@�C���I�[�v��
    concatFile = SD.open(concatName, FILE_READ);
    if ( true == concatFile ) {
      concatSize = concatFile.size();
      //��ԃR�[�h���M(OK)
      snd1byte(0x00);
      isConcatState = 1; // �I�[�v�����Ă���
      concatPos = 0;
    } else {
      //��ԃR�[�h���M(ERROR)
      snd1byte(0xFF);
    }
  } else {
    // ��ԃR�[�h���M(FILE NOT FIND ERROR)
    snd1byte(0xF1);
  }
}

// �A���t�@�C����1�u���b�N�ǂݍ���
// 0xE1
// Result:    17bytes: MZ�t�@�C����
//            2bytes:  �ǂݍ��݃A�h���X
//            2bytes:  �f�[�^�T�C�Y
//            2bytes:  ���s�A�h���X
// �f�[�^�T�C�Ybytes:  �f�[�^
//            1byte:   �X�e�[�^�X: 0xFE:���̃f�[�^������, 0x00���̃f�[�^������(�I��)
void ConcatFileRead()
{
  if(isConcatState == 0)
  {
    // �I�[�v�����Ă��Ȃ�
    for (unsigned int i = 0; i < 17; i ++) {
      snd1byte(0);
    }
    snd1byte(0xB1);
    for (unsigned int i = 0; i < 6; i ++) {
      snd1byte(0);
    }
    return;
  }
  // ���[�h�ǂݎ̂�
  int wk1 = concatFile.read();
  // �t�@�C����
  for (unsigned int lp1 = 0;lp1 <= 16; lp1 ++) {
    wk1 = concatFile.read();
    snd1byte(wk1);
  }
  //�f�[�^�T�C�Y�擾
  int f_length2 = concatFile.read();
  int f_length1 = concatFile.read();
  unsigned int f_length = f_length1*256+f_length2;
  //�X�^�[�g�A�h���X�擾
  int s_adrs2 = concatFile.read();
  int s_adrs1 = concatFile.read();
  unsigned int s_adrs = s_adrs1*256+s_adrs2;
  //���s�A�h���X�擾
  int g_adrs2 = concatFile.read();
  int g_adrs1 = concatFile.read();
  unsigned int g_adrs = g_adrs1*256+g_adrs2;
  snd1byte(s_adrs2);
  snd1byte(s_adrs1);
  snd1byte(f_length2);
  snd1byte(f_length1);
  snd1byte(g_adrs2);
  snd1byte(g_adrs1);
  concatPos += 128;
  concatFile.seek(concatPos);
  //�f�[�^���M
  for (unsigned int lp1 = 0;lp1 < f_length; lp1 ++) {
    byte i_data = concatFile.read();
    snd1byte(i_data);
    ++ concatPos;
  }
  if (concatPos < concatSize) {
    // ���̃f�[�^������
    snd1byte(0xFE);
  } else {
    // ���̃f�[�^������
    snd1byte(0x00);
  }
}

// �A���t�@�C����1�u���b�N�X�L�b�v
// 0xE2
// Result: 0xFE:���̃f�[�^������, 0x00:���̃f�[�^������(�I��), 0xFF:�I�[�v�����Ă��Ȃ�
void ConcatFileSkip()
{
  if(isConcatState == 0)
  {
    // �I�[�v�����Ă��Ȃ�
    snd1byte(0xFF);
    return;
  }
  // ���[�h�ǂݎ̂�
  int wk1 = concatFile.read();
  // �t�@�C����
  for(unsigned int lp1 = 0;lp1 <= 16; lp1 ++) {
    wk1 = concatFile.read();
  }
  //�f�[�^�T�C�Y�擾
  int f_length2 = concatFile.read();
  int f_length1 = concatFile.read();
  unsigned int f_length = f_length1*256+f_length2;
  concatPos += (128 + f_length);
  concatFile.seek(concatPos);
  if (concatPos < concatSize) {
    // ���̃f�[�^������
    snd1byte(0xFE);
  } else {
    // ���̃f�[�^������
    snd1byte(0x00);
  }
}

// �A���t�@�C������u���b�N��T��
// 0xE3, MZ�t�@�C���l�[��(17bytes)
// Result: 0x00:OK, 0xF1:FILE NOT FIND, 0xFF:�I�[�v�����Ă��Ȃ�
void ConcatFileFind()
{
  if(isConcatState == 0) {
    snd1byte(0xFF);
    return;
  }
  unsigned long concatPosBackup = concatPos;
  // �t�@�C����
  for (unsigned int lp1 = 0;lp1 < 17; lp1 ++) {
    f_name[lp1] = rcv1byte();
    if(f_name[lp1] == 0x0D) {
      f_name[lp1] = 0;
    }
  }
  f_name[17] = 0;
  snd1byte(0x00);
  // ��������
  while(1) {
    // ���[�h�ǂݎ̂�
    int wk1 = concatFile.read();
    // �t�@�C����
    for (unsigned int lp1 = 0;lp1 < 17; lp1 ++) {
      m_name[lp1] = concatFile.read();
      if(m_name[lp1] == 0x0D) {
        m_name[lp1] = 0;
      }
    }
    m_name[17] = 0;
    //�f�[�^�T�C�Y�擾
    int f_length2 = concatFile.read();
    int f_length1 = concatFile.read();
    unsigned int f_length = f_length1*256+f_length2;
    if(String(f_name).equals(m_name) == true) {
      // ������
      concatFile.seek(concatPos);
      break;
    } else {
      // ���̃t�@�C��
      concatPos += (128 + f_length);
      if (concatPos >= concatSize) {
        // ������Ȃ�����
        concatFile.seek(concatPosBackup);
        snd1byte(0xF1);
        return;
      }
      concatFile.seek(concatPos);
    }
  }
  snd1byte(0x00);
}

// �A���t�@�C���̃g�b�v�ɖ߂�
// 0xE4
// Result: 0x00:OK, 0xF1:FILE NOT FIND ERROR, 0xFF:�I�[�v�����Ă��Ȃ�
void ConcatFileTop()
{
  if(isConcatState == 0) {
    snd1byte(0xFF);
    return;
  }
  concatPos = 0;
  concatFile.seek(concatPos);
  snd1byte(0x00);
}

// �A���t�@�C���N���[�Y
// 0xE5
// Result: 0x00:OK, 0xF1:FILE NOT FIND ERROR, 0xFF:�I�[�v�����Ă��Ȃ�
void ConcatFileClose()
{
  if(isConcatState == 0) {
    snd1byte(0xFF);
    return;
  } else if(isConcatState == 1) {
    concatFile.close();
  }
  isConcatState = 0;
  snd1byte(0x00);
}

// �A���t�@�C���̎��̃f�[�^�����邩
// 0xE6
// Result: 0xFE:���̃f�[�^������, 0x00:���̃f�[�^������(�I��) 0xFF:�I�[�v�����Ă��Ȃ�
void ConcatFileState(void)
{
  if(isConcatState == 0) {
    // �I�[�v�����Ă��Ȃ�
    snd1byte(0xFF);
  } else if (concatPos < concatSize) {
    // ���̃f�[�^������
    snd1byte(0xFE);
  } else {
    // ���̃f�[�^������
    snd1byte(0x00);
  }
}

// MIDI�R�}���h���M
// 0xE7, ���M�o�C�g��, ���M�f�[�^
// Result:  0x00:�G���[����, 0xFF:�G���[
void SendMidi(void)
{
  byte length = rcv1byte();
  for(byte i = 0; i < length; i ++)
  {
    byte data = rcv1byte();
    Serial.write(data);
  }
}

void loop()
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
//�R�}���h�擾�҂�
////  Serial.print("cmd:");
  byte cmd = rcv1byte();
////  Serial.println(cmd,HEX);
  if((cmd < 0xE0) && (isConcatState == 1))
  {
    // �A���t�@�C���I�[�v�����ɒʏ�t�@�C���R�}���h�������̂ŘA���t�@�C���̓N���[�Y����
    concatFile.close();
    isConcatState = 0;
  }
  if (eflg == false){
    switch(cmd) {
//80h��SD�J�[�h��save
      case 0x80:
////  Serial.println("SAVE START");
//��ԃR�[�h���M(OK)
        snd1byte(0x00);
        f_save();
        break;
//81h��SD�J�[�h����load
      case 0x81:
////  Serial.println("LOAD START");
//��ԃR�[�h���M(OK)
        snd1byte(0x00);
        f_load();
        break;
//82h�Ŏw��t�@�C����0000.mzt�Ƃ��ă��l�[���R�s�[
      case 0x82:
////  Serial.println("ASTART START");
//��ԃR�[�h���M(OK)
        snd1byte(0x00);
        astart();
        break;
//83h�Ńt�@�C�����X�g�o��
      case 0x83:
////  Serial.println("FILE LIST START");
//��ԃR�[�h���M(OK)
        snd1byte(0x00);
        dirlist();
        break;
//84h�Ńt�@�C��Delete
      case 0x84:
////  Serial.println("FILE Delete START");
//��ԃR�[�h���M(OK)
        snd1byte(0x00);
        f_del();
        break;
//85h�Ńt�@�C�����l�[��
      case 0x85:
////  Serial.println("FILE Rename START");
//��ԃR�[�h���M(OK)
        snd1byte(0x00);
        f_ren();
        break;
      case 0x86:  
//86h�Ńt�@�C���_���v
////  Serial.println("FILE Dump START");
//��ԃR�[�h���M(OK)
        snd1byte(0x00);
        f_dump();
        break;
      case 0x87:  
//87h�Ńt�@�C���R�s�[
////  Serial.println("FILE Copy START");
//��ԃR�[�h���M(OK)
        snd1byte(0x00);
        f_copy();
        break;
      case 0x91:
//91h��0436H MONITOR ���C�g �C���t�H���[�V������֏���
////  Serial.println("0436H START");
//��ԃR�[�h���M(OK)
        snd1byte(0x00);
        mon_whead();
        break;
//92h��0475H MONITOR ���C�g �f�[�^��֏���
      case 0x92:
////  Serial.println("0475H START");
//��ԃR�[�h���M(OK)
        snd1byte(0x00);
        mon_wdata();
        break;
//93h��04D8H MONITOR ���[�h �C���t�H���[�V������֏���
      case 0x93:
////  Serial.println("04D8H START");
//��ԃR�[�h���M(OK)
        snd1byte(0x00);
        mon_lhead();
        break;
//94h��04F8H MONITOR ���[�h �f�[�^��֏���
      case 0x94:
////  Serial.println("04F8H START");
//��ԃR�[�h���M(OK)
        snd1byte(0x00);
        mon_ldata();
        break;
//95h��BOOT LOAD(MZ-2000_SD��p)
      case 0x95:
////  Serial.println("BOOT LOAD START");
//��ԃR�[�h���M(OK)
        snd1byte(0x00);
        boot();
        break;

//�A���t�@�C���R�}���h
//0E0h�ŘA���t�@�C���I�[�v��
      case 0xE0:
//��ԃR�[�h���M(OK)
        snd1byte(0x00);
        ConcatFileOpen();
        break;
//0E1h�ŘA���t�@�C��1�u���b�N�ǂݍ���
      case 0xE1:
//��ԃR�[�h���M(OK)
        snd1byte(0x00);
        ConcatFileRead();
        break;
//0E2h�ŘA���t�@�C����1�u���b�N�X�L�b�v
      case 0xE2:
//��ԃR�[�h���M(OK)
        snd1byte(0x00);
        ConcatFileSkip();
        break;
//0E3h�ŘA���t�@�C������u���b�N��T��
      case 0xE3:
//��ԃR�[�h���M(OK)
        snd1byte(0x00);
        ConcatFileFind();
        break;
//0E4h�ŘA���t�@�C���̃g�b�v�ɖ߂�
      case 0xE4:
//��ԃR�[�h���M(OK)
        snd1byte(0x00);
        ConcatFileTop();
        break;
//0E5h�ŘA���t�@�C���N���[�Y
      case 0xE5:
//��ԃR�[�h���M(OK)
        snd1byte(0x00);
        ConcatFileClose();
        break;
//0E6h�ŘA���t�@�C���̎��̃f�[�^�����邩
      case 0xE6:
//��ԃR�[�h���M(OK)
        snd1byte(0x00);
        ConcatFileState();
        break;

//0E7h��MIDI�R�}���h���M
      case 0xE7:
//��ԃR�[�h���M(OK)
        snd1byte(0x00);
        SendMidi();
        break;

      default:
//��ԃR�[�h���M(CMD ERROR)
        snd1byte(0xF4);
    }
  } else {
//��ԃR�[�h���M(ERROR)
    snd1byte(0xF0);
  }
}
