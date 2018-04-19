#pragma once
#include "stdafx.h"
#include "Common.h"
#include "MidiEvent.h"
#include "MidiTrack.h"
//统一类型长度
typedef signed int      MIDIInt,*MIDIIntP;
typedef signed char     MIDIInt8,*MIDIInt8P;
typedef signed short    MIDIInt16,*MIDIInt16P;
typedef signed int     MIDIInt32,*MIDIInt32P;
typedef unsigned int    MIDIUInt,*MIDIUIntP;
typedef unsigned char   MIDIUInt8,*MIDIUInt8P;
typedef unsigned short  MIDIUInt16,*MIDIUInt16P;
typedef unsigned int   MIDIUInt32,*MIDIUInt32P;

//MIDI文件中出现过的标识
#define MIDI_MThd   0x6468544D

//MIDI文件头的“类型”
#define MIDIT_SingleTrack   0   /*单音轨*/
#define MIDIT_MultiSync     1   /*同步多音轨*/
#define MIDIT_MultiAsync    2   /*异步多音轨*/

//各种长度的Big-Endian到Little-Endian的转换
#define BSwapW(x)   ((((x) & 0xFF)<<8)|(((x) & 0xFF00)>>8))
#define BSwap24(x)  ((((x) & 0xFF)<<16)|((x) & 0xFF00)|(((x) & 0xFF0000)>>16))
#define BSwapD(x)   ((((x) & 0xFF)<<24)|(((x) & 0xFF00)<<8)|(((x) & 0xFF0000)>>8)|(((x) & 0xFF000000)>>24))

class CMidiFile
{
public:
	typedef DWORD DynamicBytes;
	//MIDI文件头的结构体
	typedef struct MidiHeader
	{
		DWORD   dwFlag;             //MThd标识
		DWORD   dwRestLen;          //剩余部分长度
		WORD    wType;              //类型
		WORD    wNbTracks;          //音轨数
		WORD    wTicksPerCrotchet;  //每四分音符的Tick数

		MidiHeader()
		{
			dwFlag=0x6468544D;
			dwRestLen=sizeof(wType)+sizeof(wNbTracks)+sizeof(wTicksPerCrotchet);
			wType=MIDIT_MultiSync;
			wNbTracks=1;//全局音轨
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
	//分析MIDI文件。失败返回零，成功返回非零
	//-----------------------------------------------------------------------------
	int ParseMIDI();
	void Save();//save midi to file

	//跳过timeSeqOffset个时间序列，例：1表示跳过一段(00:00.00-timeSequence[0]，且从这里起，最多只保存splitCount个段
	//文件命名从outFileNameStart开始
	void Split(char **timeSequence,char outFileNameStart,char timeSeqOffset,char splitCount);
	void SplitByTick(UInt64 tickSequence[],char outFileNameStart,char tickSeqOffset,char splitCount);
	void Clone(const CMidiFile *src);//just clone header and first track
	void CloneTrack(const CMidiTrack *src ,BYTE *tpList=NULL);//复制事件块，只复制TpList里面的类型，可为空（复制所有的事件），列表以0结束
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

