#pragma once
#include "Common.h"
#include "MidiEvent.h"

#define MIDI_MTrk   0x6B72544D

class CMidiTrack
{
public:
	CMidiTrack(void);
	CMidiTrack(const CMidiTrack &src);//���������¼�
	~CMidiTrack(void);
	
	CMidiTrack &operator=(const CMidiTrack &src);
	void AppendEvent(CMidiEvent *pEvent);
	void CopyAppendEvent(const CMidiEvent *src);
	void CalcLen();
	void SetEndingOffset(UInt64 tick);//���ý�βӦ��ƫ�Ƶ�tick��
	DWORD SizeOf() const {return dwLen+sizeof(dwFlag)+sizeof(dwLen);}

	BOOL Eat(FILE *fp);
	BOOL Puke(FILE *fp);
	BOOL OutputTickSequence(FILE *fp);//����������µ�ʱ������
public:
	void AmazingInstruments(BYTE *pInstruments);//�ٱ�������0x80Ϊ�б������
public:
	DWORD   dwFlag;     //Ϊ0x6B72544D����"MTrk"
	DWORD   dwLen; //���쳤�ȣ���ȥ����ͷ��������ֽ�����(Big-Endian)
	CMidiEvent	*pFirstEvent;
	CMidiTrack *next;//track list

	CMidiEvent *pLast2Event;
	DWORD dwQuarterTus;// ?us per tick
};

