#include "stdafx.h"
#include "Common.h"
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>

DAT_TYPE *_malloc(CNT_TYPE x)//alloc a block of memory and record the size
{
	DAT_TYPE *p = (DAT_TYPE *)malloc(sizeof(CNT_TYPE)+x);
	*((CNT_TYPE*)p)=x;
	p+=sizeof(CNT_TYPE);
	return p;
}

void _free(DAT_TYPE *p)
{
	free((p-sizeof(CNT_TYPE)));
}

CNT_TYPE _sizeof(DAT_TYPE *p)
{
	return *(CNT_TYPE*)(p-sizeof(CNT_TYPE));
}

void _memcpy(DAT_TYPE *&dst,DAT_TYPE *src)
{
	dst = _malloc(_sizeof(src));
	memcpy(dst,src,_sizeof(src));
}

void WriteStruct(char *szFormat,void * pStruct,FILE *fp)
{
	BYTE *p = (BYTE*)pStruct;
	int md ,size;
	//int max = 1;
	int type = 0;
	DWORD dw,n;
	BYTE t;
	int i,j;
	BYTE buffer[4];

	while(*szFormat)
	{
		size = 0;
		type = 1;
		switch(*szFormat++)
		{
			//basic type
		case 'c'://char
			size = sizeof(char);
			break;
		case 't'://short
			size = sizeof(short);
			type=2;
			break;
		case 'i'://int
			size = sizeof(int);
			type=2;
			break;
		case 'l'://long
			size = sizeof(long);
			type=2;
			break;
		case 'L'://long long
			size = sizeof(long long);
			type=2;
			break;
		case '\\'://size tag
			if(*szFormat=='?')//get size from struct
			{
				size=(*p)+1;
			}
			else if(*szFormat=='!')//get size from (p-?),? is sizeof(CNT_TYPE),CNT_TYPE is defined in common.h
			{
				size=_sizeof(p);
			}
			else
			{
				size = *szFormat++ - 0x30;
			}
			break;

		case 'y'://dynamic bytes ,y2:length of data needs to be written is 2-byte
			n=*szFormat++ - 0x30;
			memcpy(&dw,p,n);

			i=0;
			do 
			{
				buffer[i]=dw&0x7f;
				dw>>=7;
				i++;
			} while (dw!=0);
			BSConvert(buffer,i);
			for(j=0;j<i-1;j++)
			{
				buffer[j]|=0x80;
			}
			fwrite(buffer,1,i,fp);

			p+=n;
			type=0;
			continue;
		default:
			type = 0;
		}

		if(type)
		{
			md = (p - pStruct)%size;
			p += md?size-md:0;//padding
			//max = size>max?size:max;   struct size is the integral multiple of the max-size data
			if(type==2)
			{
				memcpy(buffer,p,size);
				BSConvert(buffer,size);
				fwrite(buffer,1,size,fp);
			}
			else
			{
				fwrite(p,1,size,fp);
			}

			p+=size;
		}
	}
}

void ReadStruct(char *szFormat,void * pStruct,FILE *fp)
{
	BYTE *p = (BYTE*)pStruct;
	int md ,size;
	//int max = 1
	int type = 0;
	DWORD *pOut;
	size_t bBytesRead;//已读取的字节数

	while(*szFormat)
	{
		size = 0;
		type = 1;
		switch(*szFormat++)
		{
			//basic type
		case 'c'://char
			size = sizeof(char);
			break;
		case 't'://short
			type=2;
			size = sizeof(short);
			break;
		case 'i'://int
			type=2;
			size = sizeof(int);
			break;
		case 'l'://long
			type=2;
			size = sizeof(long);
			break;
		case 'L'://long long
			type=2;
			size = sizeof(long long);
			break;
		case '\\'://size tag
			if(*szFormat=='?')//length information is in file
			{
				size=fgetc(fp)+1;//include this byte
				fseek(fp,-1,SEEK_CUR);//seek back
			}
			else
			{
				size = *szFormat++ - 0x30;
			}
			break;
		case 'y'://dynamic bytes
			pOut=(DWORD*)p;
			*pOut=0;
			for(bBytesRead=1;bBytesRead<=4;bBytesRead++)//最多读取4字节
			{
				int iValue=fgetc(fp);
				if(iValue==EOF)
					break;
				*pOut=(*pOut<<7)|(iValue & 0x7F);//新读入的是低位
				if(!(iValue & 0x80))//如果没有后续字节
					break;//就停止读取。
			}
			p+=4;
			continue;
		default:
			type = 0;
		}

		if(type)
		{
			md = (p - pStruct)%size;
			p += md?size-md:0;//padding
			//max = size>max?size:max;
			fread(p,1,size,fp);
			if(type==2)//endian convert
			{
				BSConvert(p,size);
			}
			p+=size;
		}
	}
}

void BSConvert(BYTE* p,size_t size)
{
	BYTE* q=p+size-1;
	BYTE t;
	while(p<q)
	{
		t=*p;
		*p=*q;
		*q=t;
		p++;
		q--;
	}
}

int Pow(int x,int y)
{
	if(y%2)
	{
		if(y==1) return x;
		return Pow(x,y/2)*x;
	}
	else
	{
		if(y==2) return x*x;
		return Pow(x,y/2);
	}
}

int getRandNum(int from,int to)
{
	static char fileFlag[30]={0};//用于指示该文件是否被使用过

	srand((unsigned int)time(NULL));
	int d=to-from+1;
	int n = rand()%d;

	for(int i=0,j=n;i<d;i++)
	{
		if(!fileFlag[j])//找到一个未用过的数
		{
			n=j;
			fileFlag[j]=1;//标记已用
			break;
		}
		if(j==n-1)//全部都已经使用
		{
			memset(fileFlag,0,30);//清零
		}
		j++;
		if(j==d) j=0;//循环
	}
	n+=from;

	return n;
}