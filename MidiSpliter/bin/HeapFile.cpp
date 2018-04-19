#include "HeapFile.h"


CHeapFile::CHeapFile(char *szFilePath):buffer(NULL),m_p(NULL),m_fLen(0)
{
	USES_CONVERSION;
	m_lpszFilePath = A2T(szFilePath);
	Open();
}

CHeapFile::CHeapFile(LPCTSTR szFile):buffer(NULL),m_p(NULL),m_fLen(0)
{
	m_lpszFilePath = szFile;
	Open();
}

CHeapFile::~CHeapFile(void)
{
	CFile::Close();
	if(buffer) delete []buffer;//free buffer
}

void CHeapFile::Open()
{
	ULONG i;
	//open file
	if(GetFileAttributes(m_lpszFilePath)<0)//file not exists
	{
		CFile::Open(m_lpszFilePath,CFile::OpenFlags.modeCreate);
		//create default buffer
		buffer=new char[m_pageSize];
	}
	else
	{
		CFile::Open(m_lpszFilePath,CFile::OpenFlags.modeReadWrite);
		//read to buffer
		m_fLen = CFile::GetLength();
		m_bufLen=(m_fLen/m_pageSize+1)*m_pageSize;//reamin a page with some space free
		buffer = new char[m_bufLen];
		CFile::Read(buffer,m_fLen);
	}
}
