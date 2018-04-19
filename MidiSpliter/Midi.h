#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <io.h>
#include <string.h>
#define C1 60 //C��1�ļ���ֵ
#define FOURPAINUM 64 //1/4��������
#define MIDICLOCK 24 //ÿ1/64������MIDICLOCK��
#define JumpNullChar(x)  {while(*x==' '||*x=='\t'||*x=='\n' ||*x=='|') x++;}

 enum ERRORCODE{ //���������Ϣ
	 ChangeOK, //ת���ɹ�
	 TextFileNotOpen, //�ı��ļ����ܴ�
	 MidiFileCanNotCreate, //ָ����MIDI�ļ����ܽ���
	 TextFileToBig, //�ı��ļ�̫��
	 MallocError, //�ڴ�������
	 InvalideChar, //���ı��ļ��г����˷Ƿ��ַ�
	 NotFoundTrack, //û���ҵ�ָ���Ĵŵ���Ϣ
	 NotMIDITextFile, //�ı��ļ�����MIDI�ı��ļ�
 };
 void SWAP(char *x,char *y) //�����ݽ���
 { char i;
 i=*x;
 *x=*y;
 *y=i;
 }
 union LENGHT
 { long length;
 char b[4];
 } ;
 struct MH { //MIDI�ļ�ͷ
	 char MidiId[4]; //MIDI�ļ���־MThd
	 long length; //ͷ����Ϣ����
	 int format; //��ŵĸ�ʽ
	 int ntracks; //�ŵ���Ŀ
	 int PerPaiNum; //ÿ�ڼ�����ֵ
 };
 struct TH //����ͷ
 { char TrackId[4]; //�ŵ���־MTrk
 long length; //��Ϣ����
 } ;

 class MIDI
 {
 public:
	 char ErrorMsg[100]; //������Ϣ
 private:
	 char *TextFileBuf,
		 *TextFileOldBuf;
	 char *MidiFileBuf,
		 *MidiFileOldBuf;
	 char OneVal; //ĳ��ʱ,1�Ľ�ֵ
	 char PaiNum; //��һС�ڽ�������
	 char OnePaiToneNum; //�ü���������Ϊһ������
 public:
	 //����ȫMIDI�鶨��ʽ���ı��ļ�����MIDI�ļ�
	 int ChangeTextToMidi(char *TextFileName,
		 char *MidiFileName);
	 char *GetErrorMsg() //��ȡ������Ϣ
	 { return(ErrorMsg);}
 private:
	 char GetCurPaiSpeed(int n); //ȡ��ǰ�ĵİ���ǿ��
	 void WriteSoundSize(char ntrack,unsigned int );
	 void SetOnePaiToneNum(int n)
	 { OnePaiToneNum=n; };
	 void SetOneVal(char *m) ; //ȡm�����С��ʱ,1��ʵ�ʼ�ֵ
	 char GetToneNum(char c, //ȡ������Ӧ�ļ�ֵ
		 char flag) ;
	 void WriteMHToFile(long length, //����MIDI�ļ�ͷ
		 int format,
		 int ntracks,
		 int PerPaiNum,
		 FILE *fp);
	 void WriteTHToFile(long lenght,
		 FILE *fp); //����MIDI�ŵ�ͷ
	 void WriteTrackMsgToFile(FILE *fp);
	 //���ŵ�������Ϣ�����ļ���
	 void WriteSpeed(int speed);
	 void SetPaiNum(int n)
	 { PaiNum=n;}
	 long NewLong(long n); //�µ�longֵ
	 int NewInt(int n) //�µ�intֵ
	 { return(n<<8|n>>8);}
	 //��n��Ϊ�ɱ䳤��,����buf��
	 void WriteLenghtToBuf(unsigned long n,
		 char *buf);
	 void ChangePrommgram(char channel, //������ɫ
		 char promgram);
	 void NoteOn (char n, //��������
		 char speed,
		 unsigned long delaytime);
	 void WriteNoteOn(char,char,char ,unsigned long);
	 void WriteTextMsg(char *msg); //��һ���ı���Ϣ
	 void WriteTimeSignature(char n, //����ʱ����Ϣ
		 char d);
	 void WriteTrackEndMsg(); //���ôŵ�������Ϣ
 };

