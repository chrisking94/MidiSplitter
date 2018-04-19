#include "StdAfx.h"
#include "RecordHeapFile.h"


CRecordHeapFile::CRecordHeapFile(void)
{
}

CRecordHeapFile::CRecordHeapFile(LPCTSTR szFileName):CHeapFile(szFileName)
{
	
}


CRecordHeapFile::~CRecordHeapFile(void)
{
}

void CRecordHeapFile::WriteStruct(char *szFormat,byte * pStruct)
{
	Stack_PtrStruct stkp,stks;
	stkp.top = stks.top =  -1;
	byte *p = pStruct;
	int md ,size;
	//int max = 1;
	int type = 0;

	while(*szFormat)
	{
		size = 0;
		type = 1;
		switch(*szFormat++)
		{
			//basic type
		case 's'://string,the next data is char *
			size = sizeof(char**);
			type = 2;
			break;
		case 'S'://LPCTSTR
			size = sizeof(LPCTSTR*);
			type = 4;
			break;
		case 'c'://char
			size = sizeof(char);
			break;
		case 't'://short
			size = sizeof(short);
			break;
		case 'i'://int
			size = sizeof(int);
			break;
		case 'l'://long
			size = sizeof(long);
			break;
		case 'L'://long long
			size = sizeof(long long);
			break;
		case 'd'://double
			size = sizeof(double);
			break;
		case 'f':
			size = sizeof(float);
			break;
		case '\\'://size tag
			size = *szFormat++ - 0x30;
			break;

		case '<'://ptr ,use like this <p>,means *p,<<p>>means **p,etc...
			size = sizeof(long*);
			type = 3;
			break;
		case '>'://ptr end
			p = stkp.data[stkp.top--];
			pStruct = stks.data[stks.top--];
		default:
			type = 0;
		}

		if(type)
		{
			md = (p - pStruct)%size;
			p += md?size-md:0;//padding
			//max = size>max?size:max;   struct size is the integral multiple of the max-size data
			switch(type)
			{
			case 1://data
				CHeapFile::Write(p,size);
				break;
			case 2://string
				CHeapFile::WriteString(*(char**)p);
				break;
			case 4:
				CHeapFile::WriteString(*(LPCTSTR*)p);
				break;
			case 3://struct pointer
				stkp.data[++stkp.top] = p+size;//increase p and push into stack
				stks.data[++stks.top] = pStruct = p = (byte *)(*(long*)p);
				p -= size;//offset the p+size
			}

			p+=size;
		}
	}
}

void CRecordHeapFile::ReadStruct(char *szFormat,byte * pStruct)
{
	Stack_PtrStruct stkp,stks;
	stkp.top = stks.top =  -1;
	byte *p = pStruct;
	int md ,size;
	//int max = 1
	int type = 0;

	while(*szFormat)
	{
		size = 0;
		type = 1;
		switch(*szFormat++)
		{
			//basic type
		case 's'://string,the next data is char *
			size = sizeof(char**);
			type = 2;
			break;
		case 'S'://LPCTSTR
			size = sizeof(LPCTSTR*);
			type = 4;
			break;
		case 'c'://char
			size = sizeof(char);
			break;
		case 't'://short
			size = sizeof(short);
			break;
		case 'i'://int
			size = sizeof(int);
			break;
		case 'l'://long
			size = sizeof(long);
			break;
		case 'L'://long long
			size = sizeof(long long);
			break;
		case 'd'://double
			size = sizeof(double);
			break;
		case 'f':
			size = sizeof(float);
			break;
		case '\\'://size tag
			size = *szFormat++ - 0x30;
			break;

		case '<'://ptr
			size = sizeof(long*);
			type = 3;
			break;
		case '>'://
			p = stkp.data[stkp.top--];
			pStruct = stks.data[stks.top--];
		default:
			type = 0;
		}

		if(type)
		{
			md = (p - pStruct)%size;
			p += md?size-md:0;//padding
			//max = size>max?size:max;
			switch(type)
			{
			case 1://data
				CHeapFile::Read(p,size);
				break;
			case 2://string
				CHeapFile::ReadString(*(char**)p);
				break;
			case 4:
				CHeapFile::ReadString(*(LPCTSTR*)p);
				break;
			case 3://struct pointer
				stkp.data[++stkp.top] = p+size;//increase p and push into stack
				stks.data[++stks.top] = pStruct = p = (byte *)(*(long*)p);
				p -= size;
			}

			p+=size;
		}
	}
}

