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

	//创建结尾事件
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
	//清除Events
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
	//每个音轨的开头都是一个dwTrackFlag和一个dwTrackLen
	//dwTrackFlag的值必须是MIDI_MTrk
	//dwTrackLen标记了下一个音轨的位置
	size_t TrackStartPos;
	BYTE bLastEvent = 0;
	BYTE bRet=0;

	//fputs("读取音轨\n",stdout);
	ReadStruct("\\4i",this,fp);

	if(dwFlag!=MIDI_MTrk) return FALSE;//检查文件格式
	TrackStartPos=ftell(fp);//记录当前位置

	CMidiEvent *p = new CMidiEvent;
	while (bRet = p->Eat(fp,bLastEvent))
	{
		if(p->bEvent==0xFF)
		{
			if(p->pParam[0]==0x2f) break;//音轨结束
			else if(0x51==p->pParam[0])//速度
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
	if(bRet != EVENT_TRACKEND) return FALSE;//出错

	fseek(fp,TrackStartPos+dwLen,SEEK_SET);//转到下一个音轨

	return TRUE;
}

BOOL CMidiTrack::Puke(FILE *fp)
{
	size_t TrackStartPos;
	WriteStruct("\\4i",this,fp);
	TrackStartPos=ftell(fp);//记录当前位置

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
	while(ftell(fp)<TrackStartPos+dwLen)//填充长度
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
			if(p->dwDelay>0)//输出时间序列
			{
				dwDelay = p->dwDelay;
				bType = p->TypeOf();
				i = 1;
				p= p ->next;
				while(p&&p->dwDelay==0)//统计这拍按下音符数
				{
					if(p->TypeOf() == CMidiEvent::_pressNote)
					{
						i++;
						bType = CMidiEvent::_pressNote;//只要有一个按下事件，这拍就算按下
					}
					p = p->next;
				}

				n+=dwDelay;
				if(i>=1&&n>=60&&bType==CMidiEvent::_pressNote)//最小tick值120，且必须是按下音符事件
				{
					m++;
					_itoa(n/10,buffer,10);//除以10以便存储
					k = strlen(buffer);
					buffer[k++]=',';
					if(l==10)
					{
						l=0;
						buffer[k++]='\r';//换行
						buffer[k++]='\n';//换行
					}else l++;
					buffer[k++]=0;
					fwrite(buffer,1,k,fp);

					n=0;//清 0
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
	while(pInstruments[n]<0x80) n++;//计算列表长
	if(n == 0) return;
	n--;
	//处理track
	CMidiEvent *p = pFirstEvent ,*q,*r;
	DWORD i=0,j=0,k=0;
	BYTE bCh;
	
	while(p)
	{
		if(p->TypeOf() == CMidiEvent::_changeInstrument)
		{
			//从乐器列表随机选择一个乐器
			p->SetParam(pInstruments[getRandNum(0,n)]);
			bCh = p->GetChannel();
		}
		//改变乐器的规则，出现3个音符在同一拍
		BYTE t=p->TypeOf();
		if(t == CMidiEvent::_pressNote)
		{
			i=1;
			while(p->next->dwDelay == 0)
			{
				if(p->TypeOf() == CMidiEvent::_pressNote) i++;
				p = p->next;
			}
			if(i>=1)//同时按3个以上音符一拍
			{
				//保存tick序列
				//改变乐器
				//r = new CMidiEvent(CMidiEvent::_changeInstrument);
				//r->SetChannel( bCh);
				//int k = getRandNum(0,n);
				//r->SetParam((BYTE)pInstruments[k]);
				//int k = getRandNum(0,90);//随机乐器
				//r->SetParam((BYTE)k);
				//r->next = q->next;//插入事件
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
		if(p->bEvent==lastEvent && (p->pParam[0] <= 0x7F) && p->bEvent<=0xCF) // 从左到右 ||优先级高于&?， != 优先级高于 &
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