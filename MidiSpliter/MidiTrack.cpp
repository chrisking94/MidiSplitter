#include "StdAfx.h"
#include "MidiTrack.h"
#include "Common.h"
#include <string.h>

CMidiTrack::CMidiTrack(void)
{
	dwQuarterTus = 0;
	dwFlag = 0x6B72544D;
	BYTE p[]={0x2f};
	pFirstEvent = new CMidiEvent(0,0xFF,p,1);//track end
	pLast2Event = NULL;
	CalcLen();
	next = NULL;
}

CMidiTrack::CMidiTrack(const CMidiTrack &src):pLast2Event(NULL),next(NULL)
{
	CMidiEvent *p ;
	dwFlag = src.dwFlag;
	dwLen = src.dwLen;
	dwQuarterTus = src.dwQuarterTus;

	//������β�¼�
	BYTE bt[]={0x2f};
	pFirstEvent = new CMidiEvent(0,0xFF,bt,1);//track end
	pLast2Event = NULL;
	
	p = src.pFirstEvent;
	while(p->next)
	{
		CopyAppendEvent(p);
		p = p->next;
	}
}

CMidiTrack::~CMidiTrack(void)
{
	//���Events
	CMidiEvent *e=pFirstEvent,*q;
	while(e)
	{
		q=e->next;
		delete e;
		e=q;
	}
}

void CMidiTrack::SetEndingOffset(UInt64 tick)
{
	if(pLast2Event)
	{
		pLast2Event->next->dwDelay=tick;
	}
	else
	{
		pFirstEvent->dwDelay=tick;
	}
}

BOOL CMidiTrack::Eat(FILE *fp)
{
	//ÿ������Ŀ�ͷ����һ��dwTrackFlag��һ��dwTrackLen
	//dwTrackFlag��ֵ������MIDI_MTrk
	//dwTrackLen�������һ�������λ��
	size_t TrackStartPos;
	BYTE bLastEvent = 0;
	BYTE bRet=0;

	//fputs("��ȡ����\n",stdout);
	ReadStruct("\\4i",this,fp);

	if(dwFlag!=MIDI_MTrk) return FALSE;//����ļ���ʽ
	TrackStartPos=ftell(fp);//��¼��ǰλ��

	CMidiEvent *p = new CMidiEvent;
	while (bRet = p->Eat(fp,bLastEvent))
	{
		if(p->bEvent==0xFF)
		{
			if(p->pParam[0]==0x2f) break;//�������
			else if(0x51==p->pParam[0])//�ٶ�
			{
				dwQuarterTus = 0;
				memcpy(&dwQuarterTus,p->pParam+2,3);
				BSConvert((BYTE*)&dwQuarterTus,3);
			}
		}
		bLastEvent = p->bEvent;
		AppendEvent(p);

		p=new CMidiEvent;
	}
	if(bRet != EVENT_TRACKEND) return FALSE;//����

	fseek(fp,TrackStartPos+dwLen,SEEK_SET);//ת����һ������

	return TRUE;
}

BOOL CMidiTrack::Puke(FILE *fp)
{
	size_t TrackStartPos;
	WriteStruct("\\4i",this,fp);
	TrackStartPos=ftell(fp);//��¼��ǰλ��

	CMidiEvent *pEvent=pFirstEvent;

	BYTE bLastEvent = 0;
	while(pEvent)
	{
		if(!pEvent->Puke(fp,bLastEvent))
		{
			return FALSE;
		}
		bLastEvent = pEvent->bEvent;
		pEvent = pEvent->next;
	}
	while(ftell(fp)<TrackStartPos+dwLen)//��䳤��
	{
		fputc(0,fp);
	}
	return TRUE;
}

BOOL CMidiTrack::OutputTickSequence(FILE *fp)
{
	CMidiEvent *p = pFirstEvent;
	int i,k,l=0,m=0,n=0;
	char buffer[64];
	DWORD dwDelay;
	CMidiEvent::EventType bType;

	while(p)
	{
		if(p->TypeOf() == CMidiEvent::_pressNote || p->TypeOf() == CMidiEvent::_releaseNote)
		{
			if(p->dwDelay>0)//���ʱ������
			{
				dwDelay = p->dwDelay;
				bType = p->TypeOf();
				i = 1;
				p= p ->next;
				while(p&&p->dwDelay==0)//ͳ�����İ���������
				{
					if(p->TypeOf() == CMidiEvent::_pressNote)
					{
						i++;
						bType = CMidiEvent::_pressNote;//ֻҪ��һ�������¼������ľ��㰴��
					}
					p = p->next;
				}

				n+=dwDelay;
				if(i>=1&&n>=60&&bType==CMidiEvent::_pressNote)//��Сtickֵ120���ұ����ǰ��������¼�
				{
					m++;
					_itoa(n/10,buffer,10);//����10�Ա�洢
					k = strlen(buffer);
					buffer[k++]=',';
					if(l==10)
					{
						l=0;
						buffer[k++]='\r';//����
						buffer[k++]='\n';//����
					}else l++;
					buffer[k++]=0;
					fwrite(buffer,1,k,fp);

					n=0;//�� 0
				}
				continue;
			}
		}
		p = p->next;
	}

	return m;
}

void CMidiTrack::AmazingInstruments(BYTE *pInstruments)
{
	int n = 0;
	while(pInstruments[n]<0x80) n++;//�����б�
	if(n == 0) return;
	n--;
	//����track
	CMidiEvent *p = pFirstEvent ,*q,*r;
	DWORD i=0,j=0,k=0;
	BYTE bCh;
	
	while(p)
	{
		if(p->TypeOf() == CMidiEvent::_changeInstrument)
		{
			//�������б����ѡ��һ������
			p->SetParam(pInstruments[getRandNum(0,n)]);
			bCh = p->GetChannel();
		}
		//�ı������Ĺ��򣬳���3��������ͬһ��
		BYTE t=p->TypeOf();
		if(t == CMidiEvent::_pressNote)
		{
			i=1;
			while(p->next->dwDelay == 0)
			{
				if(p->TypeOf() == CMidiEvent::_pressNote) i++;
				p = p->next;
			}
			if(i>=1)//ͬʱ��3����������һ��
			{
				//����tick����
				//�ı�����
				//r = new CMidiEvent(CMidiEvent::_changeInstrument);
				//r->SetChannel( bCh);
				//int k = getRandNum(0,n);
				//r->SetParam((BYTE)pInstruments[k]);
				//int k = getRandNum(0,90);//�������
				//r->SetParam((BYTE)k);
				//r->next = q->next;//�����¼�
				//r->dwDelay = 0;
				//q->next->dwDelay = 0;
				//q->next = r;
			}
		};

		q = p;
		p = p->next;
	}
}

void CMidiTrack::AppendEvent(CMidiEvent *pEvent)
{
	if(pLast2Event==NULL)
	{
		pEvent->next=pFirstEvent;
		pLast2Event = pEvent;
		pFirstEvent= pEvent;
	}
	else //insert
	{
		pEvent->next = pLast2Event->next ;
		pLast2Event->next = pEvent;
		pLast2Event = pEvent;
	}
}

void CMidiTrack::CopyAppendEvent(const CMidiEvent *src)
{
	CMidiEvent *p = new CMidiEvent(*src);
	AppendEvent(p);
}

void CMidiTrack::CalcLen()
{
	dwLen=1;//a byte for 0x00
	CMidiEvent *p=pFirstEvent;
	BYTE lastEvent=0;

	while(p)
	{
		dwLen+=p->SizeOf();
		if(p->bEvent==lastEvent && (p->pParam[0] <= 0x7F) && p->bEvent<=0xCF) // ������ ||���ȼ�����&?�� != ���ȼ����� &
		{
			dwLen-=1;
		}
		else lastEvent = p->bEvent;
		p=p->next;
	}
	printf("%d",dwLen);
}

CMidiTrack & CMidiTrack::operator=(const CMidiTrack &src)
{
	next=NULL;
	return *this;
}