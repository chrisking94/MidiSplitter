#pragma once
#include "Common.h"

#define EVENT_TRACKEND 2
#define EVENT_UNDEFINED 3
class CMidiEvent
{
public:
	typedef DWORD DynamicBytes;
	typedef enum
	{
		_undefined = 0,
		_releaseNote = 0x8f,//低4位表示通道
		_pressNote = 0x9f,
		_noteAferTouch = 0xaf,
		_controller = 0xbf,
		_changeInstrument = 0xcf,//0xc0 ~ 0xcf
		_channelAferTouch = 0xdf,
		_slideNote = 0xef,
		_system = 0xf0,
		_metaEvent = 0xff
	}EventType;
public:
	CMidiEvent(const CMidiEvent &src);
	CMidiEvent(EventType type = _undefined);
	~CMidiEvent();
	CMidiEvent &operator=(const CMidiEvent &src);
	CMidiEvent(DynamicBytes delay,BYTE eve,BYTE *param,int paramLen);
	DWORD SizeOf() const;//包括事件号的长度

	BOOL Eat(FILE *fp ,BYTE bLastEvent);
	BOOL Puke(FILE *fp ,BYTE bLastEvent);
	BYTE GetChannel() const{return bEvent&0xf;};
	void SetChannel(BYTE bCh);
	void SetParam( BYTE param);
	EventType TypeOf() const;
	DWORD PramSizeOf() const;
public:
	DynamicBytes dwDelay;
	BYTE bEvent;//事件号
	BYTE *pParam;//Parameters
	CMidiEvent *next;
};

/*
char* CMidiFile::NoteToString(BYTE bNote)
{
static char szBuf[5];//顶多5字符
switch(bNote % 12)
{
case 0:
sprintf(szBuf,"C%d",(int)(bNote / 12)-2);
break;
case 1:
sprintf(szBuf,"C%d#",(int)(bNote / 12)-2);
break;
case 2:
sprintf(szBuf,"D%d",(int)(bNote / 12)-2);
break;
case 3:
sprintf(szBuf,"D%d#",(int)(bNote / 12)-2);
break;
case 4:
sprintf(szBuf,"E%d",(int)(bNote / 12)-2);
break;
case 5:
sprintf(szBuf,"F%d",(int)(bNote / 12)-2);
break;
case 6:
sprintf(szBuf,"F%d#",(int)(bNote / 12)-2);
break;
case 7:
sprintf(szBuf,"G%d",(int)(bNote / 12)-2);
break;
case 8:
sprintf(szBuf,"G%d#",(int)(bNote / 12)-2);
break;
case 9:
sprintf(szBuf,"A%d",(int)(bNote / 12)-2);
break;
case 10:
sprintf(szBuf,"A%d#",(int)(bNote / 12)-2);
break;
case 11:
sprintf(szBuf,"B%d",(int)(bNote / 12)-2);
break;
}
return szBuf;
}
*/