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
	//���Tracks
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
	//���ļ�
	fp=fopen(pszFileName,"rb");
	if(!fp) return 0;

	//MIDI�ļ�ͷ����һ��MIDIHeader�ṹ�塣
	//����Ҫע�����е���ֵ����Big-Endian�洢��
	//��Ҫ����ת��

	//��ȡMIDI�ļ�ͷ
	ReadStruct("\\4ittt",&Header,fp);

	//����ļ���ʽ
	if(Header.dwFlag!=MIDI_MThd)//��ʶ������"MThd"
	{
		fclose(fp);
		return 0;
	}
	//��ȷ��ת��MIDI�����λ�ã�����Header.dwRestLen�����á���
	fseek(fp,8+Header.dwRestLen,SEEK_SET);

	CMidiTrack *p;
	int i=Header.wNbTracks;

	Header.wNbTracks = 0;//append�����Ὣ������
	while(i--)
	{
		p = new CMidiTrack;
		if(!p->Eat(fp))
		{
			return 0;//��ȡ��������
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
	//���ļ�
	fp=fopen((const char*)buffer,"wb");
	if(!fp)
	{
		printf("Can't create file %s.\n",pszFileName);
		return;
		}

	//д��MIDI�ļ�ͷ
	WriteStruct("\\4ittt",&Header,fp);

	//��ȷ��ת��MIDI�����λ�ã�����Header.dwRestLen�����á���
	fseek(fp,8+Header.dwRestLen,SEEK_SET);

	pTrack=pFirstTrack;

	//׼��д����������
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
	//��������ļ�������ͬһ���ļ���
	while(buffer[--i]!='\\');
	strcpy(&buffer[++i],"MidSeg\\");//�����MidSeq�ļ���
	mkdir(buffer);
	i=strlen(buffer);//���¼���i
	memcpy(&buffer[i+3],".mid\0",5);

	unsigned long tick,tsum=0,tsumbkup=0;
	
	j=0;

	//����ʣ�µ�track
	trksrc=pFirstTrack->next;

	CMidiTrack *pt;
	int timeSeqI=0;
	
	while(trksrc)
	{
		p=trksrc->pFirstEvent;
		tsum = 0;

		for(timeSeqI=0;timeSeqI<timeSeqOffset;timeSeqI++)//����timeSeqOffset��ʱ������
		{
			tick = TimeToTick(timeSequence[timeSeqI],pFirstTrack);

			while(tsum<tick && p->next)//copy before this time��jump last event
			{
				tsum+=p->dwDelay;
				p=p->next;
			}

		}

		while(timeSequence[timeSeqI][0]&&j<splitCount)
		{
			j++;//����+1

			//�����ļ���
			int tmp=outFileNameStart;
			buffer[i+2]=tmp%10+0x30;
			tmp/=10;
			buffer[i+1]=tmp%10+0x30;
			tmp/=10;
			buffer[i]=tmp+0x30;
			outFileNameStart++;

			remove(buffer);
			pmf = new CMidiFile(buffer);

			//ȫ������
			pmf->Clone(this);

			tick = TimeToTick(timeSequence[timeSeqI++],pFirstTrack);
			
			pt = pmf->pFirstTrack->next;
			while(tsum<tick && p->next)//copy before this time��jump last event
			{
				pt->AppendEvent(new CMidiEvent(*p));
				tsum+=p->dwDelay;
				p=p->next;
			}

			//�����ļ�
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
	//��������ļ�������ͬһ���ļ���
	while(buffer[--i]!='\\');
	strcpy(&buffer[++i],"MidSeg\\");//�����MidSeq�ļ���
	mkdir(buffer);
	i=strlen(buffer);//���¼���i
	memcpy(&buffer[i+3],".mid\0",5);

	unsigned long tick,tsum=0,tsumbkup=0;

	j=0;

	//����ʣ�µ�track
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
			tickSeqI--;//����timeSeqOffset��ʱ������
			tick = tickSequence[tickSeqI];

			while( p->next)//copy after this tick��jump last event
			{
				tsum+=p->dwDelay;
				if(tsum >= tick)//ͣ���� tick ��ͷ����һ�����������ƴ����������ʼ
				{
					//���㸴�����Ӧ��ƫ�Ƶ�tick
					dwStartOffset = tsum-tick;
					//����
					tsum-=p->dwDelay;
					break;
				}
				p=p->next;
			}

			tickSeqI++;
		}
		
		while(tickSequence[tickSeqI]&&j<splitCount)
		{
			j++;//����+1

			//�����ļ���
			int tmp=outFileNameStart;
			buffer[i+2]=tmp%10+0x30;
			tmp/=10;
			buffer[i+1]=tmp%10+0x30;
			tmp/=10;
			buffer[i]=tmp+0x30;
			outFileNameStart++;

			remove(buffer);
			pmf = new CMidiFile(buffer);

			//ȫ������
			pmf->Clone(this);

			tick = tickSequence[tickSeqI++];

			pt = pmf->pFirstTrack->next;
			while(p->next)//copy before this time��jump last event
			{
				tsum+=p->dwDelay;
				if(tsum>=tick)//�������������Ѿ����������Ԥ�� tick(���������������¼���
				{
					//���ҿ��ܴ��ڵĽ����¼�
					while(p->next && (p->bEvent<0x90||p->bEvent>0x9f))//�������µ��¼�������ֹͣ������ֱ����һ��������ʼ
					{
						pt->AppendEvent(new CMidiEvent(*p));
						p=p->next;
					}
					pt->SetEndingOffset(tsum-tick);
					break;
				}
				q=new CMidiEvent(*p);
				if(bStartOffsetNotSetted)//���õ�һ���¼���ƫ��
				{
					q->dwDelay = dwStartOffset;//�������ƫ��
					bStartOffsetNotSetted = 0;
				}
				pt->AppendEvent(q);
				p=p->next;
			}

			//�����ļ�
			pmf->Format();
			pmf->Save();
			delete pmf;
		}
		trksrc=trksrc->next;
	}
}

void CMidiFile::Clone(const CMidiFile *src)
{
	//����ȫ������
	CloneTrack(src->pFirstTrack);
	//����ʣ������
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
	//��������ļ�������ͬһ���ļ���
	while(buffer[--i]!='\\');
	strcpy(&buffer[++i],"MidSeg\\");//�����MidSeq�ļ���
	mkdir(buffer);
	i=strlen(buffer);//���¼���i
	memcpy(&buffer[i+3],".mid\0",5);

	unsigned long tick,tsum=0,tsumbkup=0;

	j=0;

	//����ʣ�µ�track
	trksrc=pFirstTrack->next;

	CMidiTrack *pt;
	int tickSeqI=0;
	DWORD dwStartOffset=0,dwEndingOffset=0;
	char bStartOffsetNotSetted=1;

	//�����ļ���
	memcpy(&buffer[i],"000",3);

	remove(buffer);
	pmf = new CMidiFile(buffer,*this);
	trksrc = pmf->pFirstTrack->next;
	//ȫ�����аٱ���������
	while(trksrc)
	{
		trksrc->AmazingInstruments(pInstruments);
		trksrc=trksrc->next;
	}
	//�����ļ�
	pmf->Format();
	pmf->Save();
	//delete pmf;
}

void CMidiFile::Copy(const CMidiFile &src)
{
	Header = src.Header;
	CMidiTrack *p = src.pFirstTrack;

	Header.wNbTracks = 0;//��������
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
	//���㶨ʱ����ֵ
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
	WRITES("0x00};//��");
	WRITEDEC(m+2);
	WRITES("���ֽڣ�ǰ�����ֽ��Ƕ�ʱ��ֵTH0,TL0��һ��DIV��tick\r\n");

	fclose(fp);
	return TRUE;
}
