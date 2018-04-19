#pragma once
#include "common.h"

class CHeapFile:protected CFile//support a infinite size file
{
public:
	typedef OpenFlags OpenFlags;
private:
	//File operation
	void Open();
	void AmplifyBuffer();//extend size
public:
	~CHeapFile(void);
	CHeapFile(char *szFilePath);
	CHeapFile(LPCTSTR szFile);
	
	int seek(LONG i);//-1 to end
	void write(char *buffer,ULONG len);
	char read(char *buffer,ULONG len);//read from m_p,then m_p+=len;
protected:
	char *buffer;
	ULONG m_fLen;//bytes
private:
	char *m_p;
	ULONG m_bufLen;
	int m_pageSize;//page size
	LPCTSTR m_lpszFilePath;
};

