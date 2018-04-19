#pragma once
#include "stdafx.h"
#include <stdlib.h>

typedef unsigned char       BYTE;
typedef unsigned short      WORD;
typedef unsigned int		DWORD;
typedef long long			Int64;
typedef unsigned long long	UInt64;
typedef DWORD				BOOL;

typedef short	CNT_TYPE;
typedef BYTE	DAT_TYPE;

#define FALSE 0
#define TRUE 1

DAT_TYPE *_malloc(CNT_TYPE x);//alloc a block of memory and record the size
void _free(DAT_TYPE *p);
CNT_TYPE _sizeof(DAT_TYPE *p);
void _memcpy(DAT_TYPE *&dst,DAT_TYPE *src);//create memory for dst
//write struct to file from a struct pointer,the szFormat indicates structure
//s:string
//c:char
//l:long
//L:long long
//\n:this is a special type which indicates the size of data,unit:byte
//etc..
//<:ptr ,use like this <p>,means *p,<<p>>means **p,etc...
//>:ptr end
//we can also use this to decript a ptr of strut,use like this :<csl...>
void WriteStruct(char *szFormat,void * pStruct,FILE *fp);
//read struct
void ReadStruct(char *szFormat,void * pStruct,FILE *fp);
void BSConvert(BYTE* p,size_t size);
int Pow(int x,int y);
int getRandNum(int from,int to);

