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
	//���㶯̬�ֽڳ���
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
	//ÿ���¼��Ĺ��ɶ��ǣ�
	//��ʱ���¼��ţ�����
	//���е���ʱ�Ƕ�̬�ֽڣ�������С���¼��Ŷ���
	DWORD i=0;
	BYTE buffer[256];

	//��ȡ��ʱ
	ReadStruct("yc",this,fp);

ProcEvent:	
	if(bEvent<=0x7F)
	{
		fseek(fp,-1,SEEK_CUR);//����һ���ֽ�
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
		//Ԫ����
		ReadStruct("\\1",buffer,fp);//byte1:Ԫ��������
		if(buffer[0]==0x2f)
		{
			pParam=_malloc(1);
			*pParam=buffer[0];
			return EVENT_TRACKEND;//next track
		}
		ReadStruct("\\?",buffer+1,fp);//byte2:Ԫ�����ֽ���,?�����ֽ��ṩ
		i = buffer[1]+2;
		pParam=_malloc(i);
		memcpy(pParam,buffer,i);
	}
	else//�����¼���δ֪�¼�
	{
		fprintf(stdout,"δ֪�¼�:0x%08X. ֹͣ��ǰ����ķ�����\n",bEvent);
		return EVENT_UNDEFINED;
	}
	next = NULL;

	return TRUE;
}

BOOL CMidiEvent::Puke(FILE *fp ,BYTE bLastEvent)
{
	//fputc(0xaa,fp);
	if(bEvent==bLastEvent && (pParam[0] <= 0x7F) && bEvent<=0xCF) // ������ ||���ȼ�����&?�� != ���ȼ����� &
	{
		WriteStruct("y4",this,fp);//д����ʱ�����¼����ϸ��¼�һ��
	}
	else//ͬʱ�䣬ͬͨ��
	{
		WriteStruct("y4c",this,fp);//д����ʱ���¼���
	}
	//д������
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
	{//0x80��0x8F:�ɿ�����
		return _releaseNote;
	}
	else if(bEvent <= 0x9F)
	{//0x90��0x9F:��������
		return _pressNote;
	}
	else if(bEvent <= 0xAF)
	{//0xA0��0xAF:��������
		return _noteAferTouch;
	}
	else if(bEvent <= 0xBF)
	{//0xB0��0xBF:������
		return _controller;
	}
	else if(bEvent <= 0xCF)
	{//0xC0��0xCF:�ı�����
		return _changeInstrument;
	}
	else if(bEvent <= 0xDF)
	{//0xD0��0xDF:����ͨ��
		return _channelAferTouch;
	}
	else if(bEvent <= 0xEF)
	{//0xE0��0xEF:����
		return _slideNote;
	}
	else if(bEvent == 0xF0)
	{//0xF0:ϵͳ��
		return _system;
	}
	else if(bEvent == 0xFF)
	{//Ԫ�¼�
		BYTE bType,bBytes;
		size_t CurrentPos;

		bType=((BYTE*)pParam)[0];
		bBytes=((BYTE*)pParam)[1];

		fputs("Ԫ���� - ",stdout);

		switch(bType)
		{
		case 0x00://���ù������
			{
				fprintf(stdout,"���ù������:0x%04X\n",((WORD*)pParam+2)[0]);
			}
			break;
		case 0x01://������ע
			break;
		case 0x02://��Ȩ
			break;
		case 0x03://��������
			break;
		case 0x04://��������
			break;
		case 0x05://���
			break;
		case 0x06://���
			break;
		case 0x07://��ʼ��
			break;
		case 0x21://���쿪ʼ��ʶ
			break;
		case 0x2F://���������ʶ
		case 0x51://�ٶ�
			break;
		case 0x58://����
			break;
		case 0x59://����
			break;
		case 0x7F://�����ض���Ϣ
			break;
		default:
			fprintf(stdout,"δ֪Ԫ��������(0x%02X)\n",bType);
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