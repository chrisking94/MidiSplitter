#include "StdAfx.h"
#include "MidiEvent.h"


CMidiEvent::CMidiEvent(const CMidiEvent &src):next(NULL)
{
	*this = src;
}

CMidiEvent::CMidiEvent(DynamicBytes delay,BYTE eve,BYTE *param,int paramLen):next(NULL)
{
	dwDelay = delay;
	bEvent = eve;
	pParam = _malloc(paramLen);
	memcpy(pParam,param,paramLen);
}

CMidiEvent::CMidiEvent(EventType type):pParam(NULL),next(NULL)
{
	BYTE k;
	if(type == _changeInstrument)
	{
		bEvent = 0xc0;
		k = PramSizeOf();
		pParam = _malloc(k);
	}
}

DWORD CMidiEvent::SizeOf() const
{
	DWORD n=0,dw=dwDelay;
	//计算动态字节长度
	do
	{
		n++;
		dw>>=7;
	}while(dw);
	n+=_sizeof(pParam)+sizeof(bEvent);
	return n;
}

BOOL CMidiEvent::Eat(FILE *fp , BYTE bLastEvent)
{
	//每个事件的构成都是：
	//延时，事件号，参数
	//其中的延时是动态字节，参数大小随事件号而定
	DWORD i=0;
	BYTE buffer[256];

	//读取延时
	ReadStruct("yc",this,fp);

ProcEvent:	
	if(bEvent<=0x7F)
	{
		fseek(fp,-1,SEEK_CUR);//回退一个字节
		bEvent=bLastEvent;
		//printf("0x%02x ",bLastEvent);
		goto ProcEvent;
	}
	else if(bEvent<=0xBF)//2 bytes param
	{
		pParam = _malloc(2);
		ReadStruct("\\2",pParam,fp);//read 2 byte,stored in the pointer
	}
	else if(bEvent<=0xDF)//1 byte param
	{
		pParam = _malloc(1);
		ReadStruct("\\1",pParam,fp);//read 1 byte,stored in the pointer
	}
	else if(bEvent<=0xEF)//2 byte param
	{
		pParam = _malloc(2);
		ReadStruct("\\2",pParam,fp);//read WORD,stored in the pointer
	}
	else if(bEvent<=0xF0)//1 byte param
	{
		i=0;
		do 
		{
			ReadStruct("c",&buffer[i],fp);
		} while (buffer[i++]!=0xF7);
		pParam = _malloc(1);
		memcpy(pParam,buffer,i);
	}
	else if(bEvent == 0xFF)
	{
		//元数据
		ReadStruct("\\1",buffer,fp);//byte1:元数据类型
		if(buffer[0]==0x2f)
		{
			pParam=_malloc(1);
			*pParam=buffer[0];
			return EVENT_TRACKEND;//next track
		}
		ReadStruct("\\?",buffer+1,fp);//byte2:元数据字节数,?由首字节提供
		i = buffer[1]+2;
		pParam=_malloc(i);
		memcpy(pParam,buffer,i);
	}
	else//其它事件，未知事件
	{
		fprintf(stdout,"未知事件:0x%08X. 停止当前音轨的分析。\n",bEvent);
		return EVENT_UNDEFINED;
	}
	next = NULL;

	return TRUE;
}

BOOL CMidiEvent::Puke(FILE *fp ,BYTE bLastEvent)
{
	//fputc(0xaa,fp);
	if(bEvent==bLastEvent && (pParam[0] <= 0x7F) && bEvent<=0xCF) // 从左到右 ||优先级高于&?， != 优先级高于 &
	{
		WriteStruct("y4",this,fp);//写出延时，本事件跟上个事件一样
	}
	else//同时间，同通道
	{
		WriteStruct("y4c",this,fp);//写出延时，事件号
	}
	//写出参数
	WriteStruct("\\!",pParam,fp);

	return TRUE;
}

void CMidiEvent::SetChannel(BYTE bCh)
{
	if(bCh>0x0f)
	{
		printf("SetCh Error %02x",bCh);
		return;
	}
	bEvent&=0xf0;
	bEvent|=bCh;
}

void CMidiEvent::SetParam( BYTE param)
{
	pParam[0] = param;
}

CMidiEvent::EventType CMidiEvent::TypeOf() const
{
	if(bEvent <= 0x8F)
	{//0x80到0x8F:松开音符
		return _releaseNote;
	}
	else if(bEvent <= 0x9F)
	{//0x90到0x9F:按下音符
		return _pressNote;
	}
	else if(bEvent <= 0xAF)
	{//0xA0到0xAF:触后音符
		return _noteAferTouch;
	}
	else if(bEvent <= 0xBF)
	{//0xB0到0xBF:控制器
		return _controller;
	}
	else if(bEvent <= 0xCF)
	{//0xC0到0xCF:改变乐器
		return _changeInstrument;
	}
	else if(bEvent <= 0xDF)
	{//0xD0到0xDF:触后通道
		return _channelAferTouch;
	}
	else if(bEvent <= 0xEF)
	{//0xE0到0xEF:滑音
		return _slideNote;
	}
	else if(bEvent == 0xF0)
	{//0xF0:系统码
		return _system;
	}
	else if(bEvent == 0xFF)
	{//元事件
		BYTE bType,bBytes;
		size_t CurrentPos;

		bType=((BYTE*)pParam)[0];
		bBytes=((BYTE*)pParam)[1];

		fputs("元数据 - ",stdout);

		switch(bType)
		{
		case 0x00://设置轨道音序
			{
				fprintf(stdout,"设置轨道音序:0x%04X\n",((WORD*)pParam+2)[0]);
			}
			break;
		case 0x01://歌曲备注
			break;
		case 0x02://版权
			break;
		case 0x03://歌曲标题
			break;
		case 0x04://乐器名称
			break;
		case 0x05://歌词
			break;
		case 0x06://标记
			break;
		case 0x07://开始点
			break;
		case 0x21://音轨开始标识
			break;
		case 0x2F://音轨结束标识
		case 0x51://速度
			break;
		case 0x58://节拍
			break;
		case 0x59://调号
			break;
		case 0x7F://音序特定信息
			break;
		default:
			fprintf(stdout,"未知元数据类型(0x%02X)\n",bType);
			break;
		}
	}
}

DWORD CMidiEvent::PramSizeOf() const
{
	if(TypeOf() == _changeInstrument) return 1;
}

CMidiEvent::~CMidiEvent(void)
{
	_free(pParam);
}

CMidiEvent & CMidiEvent::operator=(const CMidiEvent &src)
{
	dwDelay = src.dwDelay;
	bEvent = src.bEvent;
	_memcpy(pParam,src.pParam);
	next = NULL;

	return *this;
}