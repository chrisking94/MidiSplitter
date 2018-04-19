#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <io.h>
#include <string.h>
#define C1 60 //C调1的键名值
#define FOURPAINUM 64 //1/4音符计数
#define MIDICLOCK 24 //每1/64音符的MIDICLOCK数
#define JumpNullChar(x)  {while(*x==' '||*x=='\t'||*x=='\n' ||*x=='|') x++;}

 enum ERRORCODE{ //处理错误信息
	 ChangeOK, //转换成功
	 TextFileNotOpen, //文本文件不能打开
	 MidiFileCanNotCreate, //指定的MIDI文件不能建立
	 TextFileToBig, //文本文件太大
	 MallocError, //内存分配错误
	 InvalideChar, //在文本文件中出现了非法字符
	 NotFoundTrack, //没有找到指定的磁道信息
	 NotMIDITextFile, //文本文件不是MIDI文本文件
 };
 void SWAP(char *x,char *y) //两数据交换
 { char i;
 i=*x;
 *x=*y;
 *y=i;
 }
 union LENGHT
 { long length;
 char b[4];
 } ;
 struct MH { //MIDI文件头
	 char MidiId[4]; //MIDI文件标志MThd
	 long length; //头部信息长度
	 int format; //存放的格式
	 int ntracks; //磁道数目
	 int PerPaiNum; //每节计算器值
 };
 struct TH //音轨头
 { char TrackId[4]; //磁道标志MTrk
 long length; //信息长度
 } ;

 class MIDI
 {
 public:
	 char ErrorMsg[100]; //错误信息
 private:
	 char *TextFileBuf,
		 *TextFileOldBuf;
	 char *MidiFileBuf,
		 *MidiFileOldBuf;
	 char OneVal; //某调时,1的健值
	 char PaiNum; //第一小节节拍总数
	 char OnePaiToneNum; //用几分音符作为一基本拍
 public:
	 //将符全MIDI书定格式的文本文件生成MIDI文件
	 int ChangeTextToMidi(char *TextFileName,
		 char *MidiFileName);
	 char *GetErrorMsg() //获取错误信息
	 { return(ErrorMsg);}
 private:
	 char GetCurPaiSpeed(int n); //取当前拍的按下强度
	 void WriteSoundSize(char ntrack,unsigned int );
	 void SetOnePaiToneNum(int n)
	 { OnePaiToneNum=n; };
	 void SetOneVal(char *m) ; //取m大调或小调时,1的实际键值
	 char GetToneNum(char c, //取记名对应的键值
		 char flag) ;
	 void WriteMHToFile(long length, //建立MIDI文件头
		 int format,
		 int ntracks,
		 int PerPaiNum,
		 FILE *fp);
	 void WriteTHToFile(long lenght,
		 FILE *fp); //建立MIDI磁道头
	 void WriteTrackMsgToFile(FILE *fp);
	 //将磁道音乐信息定入文件中
	 void WriteSpeed(int speed);
	 void SetPaiNum(int n)
	 { PaiNum=n;}
	 long NewLong(long n); //新的long值
	 int NewInt(int n) //新的int值
	 { return(n<<8|n>>8);}
	 //将n改为可变长度,内入buf处
	 void WriteLenghtToBuf(unsigned long n,
		 char *buf);
	 void ChangePrommgram(char channel, //设置音色
		 char promgram);
	 void NoteOn (char n, //演奏乐音
		 char speed,
		 unsigned long delaytime);
	 void WriteNoteOn(char,char,char ,unsigned long);
	 void WriteTextMsg(char *msg); //定一串文本信息
	 void WriteTimeSignature(char n, //设置时间信息
		 char d);
	 void WriteTrackEndMsg(); //设置磁道结束信息
 };

