#include "stdafx.h"
#include "MidiFile.h"
#include <string.h>
#include "stdlib.h"
#include <direct.h>
#include <stdio.h>
#include <io.h>


void CMidiFile::PrintString(BYTE *p)
{
	int len=p[0];
	while(len--) printf("%c",*++p);
	printf("\n");
}

DWORD CMidiFile::TimeToTick(char *szTime , CMidiTrack *ptrack)
{
	unsigned long ms,tick;
	int m,s,mm;
	m=(szTime[0]-0x30)*10+szTime[1]-0x30;
	s=(szTime[3]-0x30)*10+szTime[4]-0x30;
	mm=(szTime[6]-0x30)*10+szTime[7]-0x30;
	ms=(m*60+s)*1000+mm;
	tick=ms*Header.wTicksPerCrotchet/ptrack->dwQuarterTus;

	return tick;
}

CMidiFile::CMidiFile(char *szFileName):fp(NULL),pFirstTrack(NULL),pLastTrack(NULL)
{
	pszFileName=new char[strlen(szFileName)+1];
	strcpy(pszFileName,szFileName);

	if(ParseMIDI()==0)//(access(pszFileName,0))//file not exists
	{
		//creat file
		fp=fopen(pszFileName,"w");
		fclose(fp);
		Header.wNbTracks = 0;
	}
}


CMidiFile::CMidiFile(char *szFileName,const CMidiFile &src):fp(NULL),pFirstTrack(NULL),pLastTrack(NULL)
{
	pszFileName=new char[strlen(szFileName)+1];
	strcpy(pszFileName,szFileName);
	Copy(src);
}

CMidiFile::~CMidiFile(void)
{
	delete []pszFileName;
	//清除Tracks
	CMidiTrack *t = pFirstTrack,*p;
	while(t)
	{
		p=t->next;
		delete t;
		t=p;
	}
}

int CMidiFile::ParseMIDI()
{
	//打开文件
	fp=fopen(pszFileName,"rb");
	if(!fp) return 0;

	//MIDI文件头就是一个MIDIHeader结构体。
	//但是要注意其中的数值都是Big-Endian存储的
	//需要进行转换

	//读取MIDI文件头
	ReadStruct("\\4ittt",&Header,fp);

	//检查文件格式
	if(Header.dwFlag!=MIDI_MThd)//标识必须是"MThd"
	{
		fclose(fp);
		return 0;
	}
	//正确跳转到MIDI音轨的位置，体现Header.dwRestLen的作用……
	fseek(fp,8+Header.dwRestLen,SEEK_SET);

	CMidiTrack *p;
	int i=Header.wNbTracks;

	Header.wNbTracks = 0;//append操作会将其增加
	while(i--)
	{
		p = new CMidiTrack;
		if(!p->Eat(fp))
		{
			return 0;//读取遇到错误
		}
		AppenTrack(p);
	}
	return 1;
}



void CMidiFile::Save()
{
	//write head
	int i;
	BYTE buffer[80];
	CMidiTrack *pTrack;
	CMidiEvent *pEvent;
	BYTE lastEvent=0,lastChannel=0;

	//i=strlen(pszFileName)-4;
	strcpy((char*)buffer,pszFileName);
	//strcpy((char*)(buffer+i),"_Output.mid");
	
	//printf("%s",buffer);
	//打开文件
	fp=fopen((const char*)buffer,"wb");
	if(!fp)
	{
		printf("Can't create file %s.\n",pszFileName);
		return;
		}

	//写出MIDI文件头
	WriteStruct("\\4ittt",&Header,fp);

	//正确跳转到MIDI音轨的位置，体现Header.dwRestLen的作用……
	fseek(fp,8+Header.dwRestLen,SEEK_SET);

	pTrack=pFirstTrack;

	//准备写出各个音轨
	for(int j=0;j<Header.wNbTracks;j++)
	{
		pTrack->Puke(fp);
		pTrack = pTrack->next;
	}

	fclose(fp);
}

void CMidiFile::Split(char **timeSequence,char outFileNameStart,char timeSeqOffset,char splitCount)
{
	char buffer[80];
	CMidiTrack *trksrc=pFirstTrack;
	CMidiFile *pmf;
	CMidiEvent *p,*q;


	int i=strlen(pszFileName) ,j;
	memcpy(buffer,pszFileName,i);
	//memset(buffer+i-4,0,1);
	//所有输出文件保存在同一个文件夹
	while(buffer[--i]!='\\');
	strcpy(&buffer[++i],"MidSeg\\");//输出到MidSeq文件夹
	mkdir(buffer);
	i=strlen(buffer);//重新计算i
	memcpy(&buffer[i+3],".mid\0",5);

	unsigned long tick,tsum=0,tsumbkup=0;
	
	j=0;

	//复制剩下的track
	trksrc=pFirstTrack->next;

	CMidiTrack *pt;
	int timeSeqI=0;
	
	while(trksrc)
	{
		p=trksrc->pFirstEvent;
		tsum = 0;

		for(timeSeqI=0;timeSeqI<timeSeqOffset;timeSeqI++)//跳过timeSeqOffset个时间序列
		{
			tick = TimeToTick(timeSequence[timeSeqI],pFirstTrack);

			while(tsum<tick && p->next)//copy before this time，jump last event
			{
				tsum+=p->dwDelay;
				p=p->next;
			}

		}

		while(timeSequence[timeSeqI][0]&&j<splitCount)
		{
			j++;//段数+1

			//计算文件名
			int tmp=outFileNameStart;
			buffer[i+2]=tmp%10+0x30;
			tmp/=10;
			buffer[i+1]=tmp%10+0x30;
			tmp/=10;
			buffer[i]=tmp+0x30;
			outFileNameStart++;

			remove(buffer);
			pmf = new CMidiFile(buffer);

			//全局音轨
			pmf->Clone(this);

			tick = TimeToTick(timeSequence[timeSeqI++],pFirstTrack);
			
			pt = pmf->pFirstTrack->next;
			while(tsum<tick && p->next)//copy before this time，jump last event
			{
				pt->AppendEvent(new CMidiEvent(*p));
				tsum+=p->dwDelay;
				p=p->next;
			}

			//整理文件
			pmf->Format();
			pmf->Save();
		}
		trksrc=trksrc->next;
	}

}

void CMidiFile::SplitByTick(UInt64 tickSequence[],char outFileNameStart,char tickSeqOffset,char splitCount)
{
	char buffer[80];
	CMidiTrack *trksrc=pFirstTrack;
	CMidiFile *pmf;
	CMidiEvent *p,*q;


	int i=strlen(pszFileName) ,j;
	memcpy(buffer,pszFileName,i);
	//memset(buffer+i-4,0,1);
	//所有输出文件保存在同一个文件夹
	while(buffer[--i]!='\\');
	strcpy(&buffer[++i],"MidSeg\\");//输出到MidSeq文件夹
	mkdir(buffer);
	i=strlen(buffer);//重新计算i
	memcpy(&buffer[i+3],".mid\0",5);

	unsigned long tick,tsum=0,tsumbkup=0;

	j=0;

	//复制剩下的track
	trksrc=pFirstTrack->next;

	CMidiTrack *pt;
	int tickSeqI=0;
	DWORD dwStartOffset=0,dwEndingOffset=0;
	char bStartOffsetNotSetted=1;

	while(trksrc)
	{
		p=trksrc->pFirstEvent;
		tsum = 0;
		tickSeqI = tickSeqOffset;

		if(tickSeqOffset>0)
		{
			tickSeqI--;//跳过timeSeqOffset个时间序列
			tick = tickSequence[tickSeqI];

			while( p->next)//copy after this tick，jump last event
			{
				tsum+=p->dwDelay;
				if(tsum >= tick)//停在以 tick 开头的这一个音符，复制从这个音符开始
				{
					//计算复制起点应该偏移的tick
					dwStartOffset = tsum-tick;
					//回退
					tsum-=p->dwDelay;
					break;
				}
				p=p->next;
			}

			tickSeqI++;
		}
		
		while(tickSequence[tickSeqI]&&j<splitCount)
		{
			j++;//段数+1

			//计算文件名
			int tmp=outFileNameStart;
			buffer[i+2]=tmp%10+0x30;
			tmp/=10;
			buffer[i+1]=tmp%10+0x30;
			tmp/=10;
			buffer[i]=tmp+0x30;
			outFileNameStart++;

			remove(buffer);
			pmf = new CMidiFile(buffer);

			//全局音轨
			pmf->Clone(this);

			tick = tickSequence[tickSeqI++];

			pt = pmf->pFirstTrack->next;
			while(p->next)//copy before this time，jump last event
			{
				tsum+=p->dwDelay;
				if(tsum>=tick)//这个音符的起点已经超过或等于预期 tick(可能是音符结束事件）
				{
					//查找可能存在的结束事件
					while(p->next && (p->bEvent<0x90||p->bEvent>0x9f))//复制余下的事件，包括停止发音，直到下一个音符开始
					{
						pt->AppendEvent(new CMidiEvent(*p));
						p=p->next;
					}
					pt->SetEndingOffset(tsum-tick);
					break;
				}
				q=new CMidiEvent(*p);
				if(bStartOffsetNotSetted)//设置第一个事件的偏移
				{
					q->dwDelay = dwStartOffset;//设置起点偏移
					bStartOffsetNotSetted = 0;
				}
				pt->AppendEvent(q);
				p=p->next;
			}

			//整理文件
			pmf->Format();
			pmf->Save();
			delete pmf;
		}
		trksrc=trksrc->next;
	}
}

void CMidiFile::Clone(const CMidiFile *src)
{
	//复制全局音轨
	CloneTrack(src->pFirstTrack);
	//复制剩余音轨
	CMidiTrack *pt = src->pFirstTrack->next;
	BYTE tpList[] = {0xb0,0xcf,0xf0,0xff,0};
	Header = src->Header;

	while(pt)
	{
		CloneTrack(pt,tpList);
		pt=pt->next;
	}
}

void CMidiFile::CloneTrack(const CMidiTrack *src ,BYTE *tpList/*=NULL*/)
{
	CMidiTrack *pt = pLastTrack;
	CMidiEvent *pe = src->pFirstEvent;
	BYTE *pb;

	pt->next = new CMidiTrack;
	
	while(pe->next)
	{
		if(tpList)
		{
			pb = tpList;

			while(*pb)
			{
				if(pe->bEvent<=*pb && pe->bEvent>(*pb-0x10))
				{
					pt->CopyAppendEvent(pe);
					break;
				}
				pb++;
			}
		}
		else pt->CopyAppendEvent(pe);
		pe=pe->next;
	}
}

void CMidiFile::AppenTrack(CMidiTrack *p)
{
	if(pLastTrack==NULL)
	{
		pFirstTrack = p;
		pLastTrack = p;
	}
	else
	{
		pLastTrack->next = p;
		pLastTrack = p;
	}
	pLastTrack->next = NULL;
	Header.wNbTracks++;
}

void CMidiFile::Format()
{
	Header.wNbTracks = 0;
	CMidiTrack *p = pFirstTrack;
	while(p)
	{
		Header.wNbTracks++;
		p->CalcLen();
		p=p->next;
	}
}

void CMidiFile::AmazingInstruments(BYTE *pInstruments)
{
	char buffer[80];
	CMidiTrack *trksrc=pFirstTrack;
	CMidiFile *pmf;
	CMidiEvent *p,*q;


	int i=strlen(pszFileName) ,j;
	memcpy(buffer,pszFileName,i);
	//memset(buffer+i-4,0,1);
	//所有输出文件保存在同一个文件夹
	while(buffer[--i]!='\\');
	strcpy(&buffer[++i],"MidSeg\\");//输出到MidSeq文件夹
	mkdir(buffer);
	i=strlen(buffer);//重新计算i
	memcpy(&buffer[i+3],".mid\0",5);

	unsigned long tick,tsum=0,tsumbkup=0;

	j=0;

	//复制剩下的track
	trksrc=pFirstTrack->next;

	CMidiTrack *pt;
	int tickSeqI=0;
	DWORD dwStartOffset=0,dwEndingOffset=0;
	char bStartOffsetNotSetted=1;

	//计算文件名
	memcpy(&buffer[i],"000",3);

	remove(buffer);
	pmf = new CMidiFile(buffer,*this);
	trksrc = pmf->pFirstTrack->next;
	//全部进行百变乐器处理
	while(trksrc)
	{
		trksrc->AmazingInstruments(pInstruments);
		trksrc=trksrc->next;
	}
	//整理文件
	pmf->Format();
	pmf->Save();
	//delete pmf;
}

void CMidiFile::Copy(const CMidiFile &src)
{
	Header = src.Header;
	CMidiTrack *p = src.pFirstTrack;

	Header.wNbTracks = 0;//计数清零
	while(p)
	{
		AppenTrack(new CMidiTrack(*p));
		p = p->next;
	}
}

BOOL CMidiFile::OutputTickSequence()
{
	char buffer[128];
	char ID[]="xiangrikui";
#define WRITES(x) {fwrite(x,1,strlen(x),fp);}
#define WRITEBUFF fwrite(buffer,1,strlen(buffer),fp);
#define WRITEHEX(x) {_itoa(x,buffer,16);WRITEBUFF;}
#define WRITEDEC(x) {_itoa(x,buffer,10);WRITEBUFF;}
	strcpy(buffer,pszFileName);
	int n = strlen(buffer);
	n-=3;
	strcpy(&buffer[n],"txt");
	FILE *fp = fopen(buffer,"wb");
	WRITES("uchar code seq[]={\r\n");
	//计算定时器初值
#define SYS_SCLK 11.0592 //MHz
#define DIV 10
	DWORD TN=pFirstTrack->dwQuarterTus*DIV*SYS_SCLK/Header.wTicksPerCrotchet/12;
	//DWORD ms = pFirstTrack->dwQuarterTus/1000/Header.wTicksPerCrotchet;
	TN=0x10000-TN;
 	BYTE TH0=((BYTE*)&TN)[1],TL0=((BYTE*)&TN)[0];
	WRITES("0x");
	WRITEHEX(TH0);
	WRITES(",0x");
	WRITEHEX(TL0);
	WRITES(",");

	int m = pFirstTrack->next->OutputTickSequence(fp);
	WRITES("0x00};//共");
	WRITEDEC(m+2);
	WRITES("个字节，前两个字节是定时器值TH0,TL0，一次DIV个tick\r\n");

	fclose(fp);
	return TRUE;
}
