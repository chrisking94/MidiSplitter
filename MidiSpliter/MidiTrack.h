#pragma once
#include "Common.h"
#include "MidiEvent.h"

#define MIDI_MTrk   0x6B72544D

class CMidiTrack
{
public:
	CMidiTrack(void);
	CMidiTrack(const CMidiTrack &src);//复制所有事件
	~CMidiTrack(void);
	
	CMidiTrack &operator=(const CMidiTrack &src);
	void AppendEvent(CMidiEvent *pEvent);
	void CopyAppendEvent(const CMidiEvent *src);
	void CalcLen();
	void SetEndingOffset(UInt64 tick);//设置结尾应该偏移的tick数
	DWORD SizeOf() const {return dwLen+sizeof(dwFlag)+sizeof(dwLen);}

	BOOL Eat(FILE *fp);
	BOOL Puke(FILE *fp);
	BOOL OutputTickSequence(FILE *fp);//输出音符按下的时间序列
public:
	void AmazingInstruments(BYTE *pInstruments);//百变乐器，0x80为列表结束码
public:
	DWORD   dwFlag;     //为0x6B72544D，即"MTrk"
	DWORD   dwLen; //音轨长度（除去音轨头部以外的字节数）(Big-Endian)
	CMidiEvent	*pFirstEvent;
	CMidiTrack *next;//track list

	CMidiEvent *pLast2Event;
	DWORD dwQuarterTus;// ?us per tick
};

