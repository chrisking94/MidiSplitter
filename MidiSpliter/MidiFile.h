#pragma once
#include "stdafx.h"
#include "Common.h"
#include "MidiEvent.h"
#include "MidiTrack.h"
//ͳһ���ͳ���
typedef signed int      MIDIInt,*MIDIIntP;
typedef signed char     MIDIInt8,*MIDIInt8P;
typedef signed short    MIDIInt16,*MIDIInt16P;
typedef signed int     MIDIInt32,*MIDIInt32P;
typedef unsigned int    MIDIUInt,*MIDIUIntP;
typedef unsigned char   MIDIUInt8,*MIDIUInt8P;
typedef unsigned short  MIDIUInt16,*MIDIUInt16P;
typedef unsigned int   MIDIUInt32,*MIDIUInt32P;

//MIDI�ļ��г��ֹ��ı�ʶ
#define MIDI_MThd   0x6468544D

//MIDI�ļ�ͷ�ġ����͡�
#define MIDIT_SingleTrack   0   /*������*/
#define MIDIT_MultiSync     1   /*ͬ��������*/
#define MIDIT_MultiAsync    2   /*�첽������*/

//���ֳ��ȵ�Big-Endian��Little-Endian��ת��
#define BSwapW(x)   ((((x) & 0xFF)<<8)|(((x) & 0xFF00)>>8))
#define BSwap24(x)  ((((x) & 0xFF)<<16)|((x) & 0xFF00)|(((x) & 0xFF0000)>>16))
#define BSwapD(x)   ((((x) & 0xFF)<<24)|(((x) & 0xFF00)<<8)|(((x) & 0xFF0000)>>8)|(((x) & 0xFF000000)>>24))

class CMidiFile
{
public:
	typedef DWORD DynamicBytes;
	//MIDI�ļ�ͷ�Ľṹ��
	typedef struct MidiHeader
	{
		DWORD   dwFlag;             //MThd��ʶ
		DWORD   dwRestLen;          //ʣ�ಿ�ֳ���
		WORD    wType;              //����
		WORD    wNbTracks;          //������
		WORD    wTicksPerCrotchet;  //ÿ�ķ�������Tick��

		MidiHeader()
		{
			dwFlag=0x6468544D;
			dwRestLen=sizeof(wType)+sizeof(wNbTracks)+sizeof(wTicksPerCrotchet);
			wType=MIDIT_MultiSync;
			wNbTracks=1;//ȫ������
			wTicksPerCrotchet=0;
		}
	}MIDIHeader,*MIDIHeaderP;

	void PrintString(BYTE *p);
	DWORD TimeToTick(char *szTime , CMidiTrack *ptrack);
public:
	CMidiFile(char *szFileName);
	CMidiFile(char *szFileName,const CMidiFile &src);
	~CMidiFile(void);

	//=============================================================================
	//ParseMIDI:
	//����MIDI�ļ���ʧ�ܷ����㣬�ɹ����ط���
	//-----------------------------------------------------------------------------
	int ParseMIDI();
	void Save();//save midi to file

	//����timeSeqOffset��ʱ�����У�����1��ʾ����һ��(00:00.00-timeSequence[0]���Ҵ����������ֻ����splitCount����
	//�ļ�������outFileNameStart��ʼ
	void Split(char **timeSequence,char outFileNameStart,char timeSeqOffset,char splitCount);
	void SplitByTick(UInt64 tickSequence[],char outFileNameStart,char tickSeqOffset,char splitCount);
	void Clone(const CMidiFile *src);//just clone header and first track
	void CloneTrack(const CMidiTrack *src ,BYTE *tpList=NULL);//�����¼��飬ֻ����TpList��������ͣ���Ϊ�գ��������е��¼������б���0����
	void AppenTrack(CMidiTrack *p);
	void Format();
	void AmazingInstruments(BYTE *pInstruments);
	void Copy(const CMidiFile &src);
	BOOL OutputTickSequence();
public:
	MIDIHeader Header;
	CMidiTrack *pFirstTrack;
	CMidiTrack *pLastTrack;
private:
	FILE *fp;
	char *pszFileName;
};

