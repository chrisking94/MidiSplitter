#pragma once
#include "heapfile.h"

class __declspec(dllexport) CRecordHeapFile :
	public CHeapFile
{
public:
	CRecordHeapFile(void);
	~CRecordHeapFile(void);
	CRecordHeapFile(LPCTSTR szFileName);

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
	void WriteStruct(char *szFormat,byte * pStruct);

	//read struct
	void ReadStruct(char *szFormat,byte * pStruct);

private:
	typedef struct
	{
		byte *data[256];//maximum supported nesting level is 256.start at level 0. stp contain the ptr of struct start
		int top;
	}Stack_PtrStruct;
};

