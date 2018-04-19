// MidiSpliter.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include "MidiFile.h"
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <stdio.h>
#include <time.h>

int getMidiFileList(const char* path ,char fileListRet[][40])
{
	struct _finddata_t data;
	int count=0;

	long hnd = _findfirst( path, &data );    // �����ļ�����������ʽchRE��ƥ���һ���ļ�
	if ( hnd < 0 )
	{
		perror( path );
	}
	int  nRet = (hnd <0 ) ? -1 : 1;
	while ( nRet >= 0 )
	{
		if ( data.attrib == _A_SUBDIR )  // �����Ŀ¼
			printf("   [%s]*\n", data.name );
		else
		{
			strcpy(fileListRet[count++],data.name);
		}
		nRet = _findnext( hnd, &data );
	}
	_findclose( hnd );     // �رյ�ǰ���

	fileListRet[count][0]=0;
	return count;
}

int getTimeSequeceCount(char **timeSequece)
{
	int i=0;
	
	while(timeSequece[i][0]) i++;
	return i;
}

void buildMidiPlaySequence( const char* path, char** timeSequece )
{
	struct _finddata_t data;
	char buff[80]="H:\\Career\\GuitarPro\\midis\\";
	char fileList[30][40];

	int i=strlen(buff);

	int j,k,l;

	//����ַ�
	int seqCountPerFile,fileCountPerSeq;
	int tSeqCnt,nMidCnt,nMidRemainCnt=4;//���ܱ����ĸ�����Mid����ϣ��ļ�
	tSeqCnt = getTimeSequeceCount(timeSequece);
	nMidCnt = getMidiFileList(path,fileList);


	k=0;//�ӵ�һ���ļ���ʼ
	l=0;//�����ļ���Ŵ�0��ʼ
	
	for(int segNum=0;segNum<tSeqCnt;segNum++)
	{
		//Ϊ��i��ʱ��Ƭ���� nMidRemainCnt ��midi����
		for(j=0;j<nMidRemainCnt;j++)
		{
			//ȡһ���ļ�
			strcpy(&buff[i],fileList[k++]);
			if(k==nMidCnt) k=0;//ѭ��ȡ�ļ�
			//��ʼ�����ļ�
			CMidiFile mf(buff);
			mf.Split(timeSequece,l,segNum,1);
			l++;
		}
	}
}

int getTickSequenceCount(UInt64 tickSequence[])
{
	int i=0;
	while(tickSequence[i]) i++;
	return i;
}

void buildMidiPlaySequence( const char* path, UInt64 tickSequence[] )
{
	struct _finddata_t data;
	char buff[80]="H:\\Career\\GuitarPro\\midis\\";
	char fileList[30][40];

	int i=strlen(buff);

	int j,k,l;

	//����ַ�
	int seqCountPerFile,fileCountPerSeq;
	int tSeqCnt,nMidCnt;
	int nMidRemainCnt=1;//���ܱ���5������Mid����ϣ��ļ�
	tSeqCnt = getTickSequenceCount(tickSequence);
	nMidCnt = getMidiFileList(path,fileList);


	k=0;//�ӵ�һ���ļ���ʼ
	l=0;//�����ļ���Ŵ�0��ʼ

	for(int segNum=0;segNum<tSeqCnt;segNum++)
	{
		//Ϊ��i��ʱ��Ƭ���� nMidRemainCnt ��midi����
		for(j=0;j<nMidRemainCnt;j++)
		{
			//ȡһ���ļ�
			//strcpy(&buff[i],fileList[k++]);
			//if(k==nMidCnt) k=0;//ѭ��ȡ�ļ�
			strcpy(&buff[i],fileList[getRandNum(0,nMidCnt-1)]);//���ȡ�ļ�

			//��ʼ�����ļ�
			CMidiFile mf(buff);
			mf.SplitByTick(tickSequence,l,segNum,1);
			l++;
		}
	}
}

int _tmain(int argc, _TCHAR* argv[])
{
	goto run;

	//CMidiFile tmf("H:\\Career\\GuitarPro\\midis\\lab\\test.mid");
	//tmf.Format();
	//tmf.Save();

	FILE *p,*q;
	unsigned long n;
	p=fopen("H:\\Career\\GuitarPro\\midis\\test.mid","rb");
	q=fopen("H:\\Career\\GuitarPro\\midis\\test\\a0.mid","rb");
	
	char c,d;
	fseek(p,0,SEEK_END);
	n=ftell(p);
	fseek(p,0,SEEK_SET);
	for(int i=0;i<n;i++)
	{
		c=fgetc(p);
		d=fgetc(q);

		printf("%-d %02x %02x\n",ftell(q),c,d);
		if(c!=d)
		{
			printf("%d\n",ftell(p));
			int k=1;
		}
	} 
	fclose(p);
	fclose(q);

run:
	const char *timeSequence[]=
	{
		"00:30.00",
		"00:60.00",
		"01:30.00",
		"01:60.00",
		"02:30.00",
		"02:60.00",
		"03:30.00",
		"\0"
	};

	UInt64 tickSequence[]=
	{
		1920,
		3840,
		9600,
		14400,
		17280,
		18720,
		22800,
		26880,
		30600,
		35280,
		38400,
		42240,
		48000,
		51840,
		56640,
		60240,
		64320,
		71760,
		74640,
		78720,
		80160,
		82560,
		88320,
		94080,
		97920,
		101640,
		103680,
		107520,
		111840,
		115920,
		120480,
		LONG_MAX,
		0
	};

	BYTE pInstruments[]=
	{
		0,
		25,
		36,
		40,
		45,
		46,
		84,
		105,
		106,
		107,
		110,
		0x80
	};
	//buildMidiPlaySequence("H:\\Career\\GuitarPro\\midis\\*.mid",(char**)timeSequence);
	//buildMidiPlaySequence("H:\\Career\\GuitarPro\\midis\\*.mid",tickSequence);
	CMidiFile mf("H:\\Career\\GuitarPro\\���-���տ���Լ��.mid");
	//CMidiFile mf("H:\\Career\\GuitarPro\\�����.mid");
	mf.OutputTickSequence();
	//mf.AmazingInstruments(pInstruments);
	system("pause");

	return 0;
}

