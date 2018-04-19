//=============================================================================
//作者：0xAA55
//论坛：http://www.0xaa55.com/
//版权所有(C) 2013-2014 技术宅的结界
//请保留原作者信息，否则视为侵权。
//-----------------------------------------------------------------------------
#include<stdio.h>
#include<malloc.h>

//统一类型长度
typedef signed int      MIDIInt,*MIDIIntP;
typedef signed char     MIDIInt8,*MIDIInt8P;
typedef signed short    MIDIInt16,*MIDIInt16P;
typedef signed long     MIDIInt32,*MIDIInt32P;
typedef unsigned int    MIDIUInt,*MIDIUIntP;
typedef unsigned char   MIDIUInt8,*MIDIUInt8P;
typedef unsigned short  MIDIUInt16,*MIDIUInt16P;
typedef unsigned long   MIDIUInt32,*MIDIUInt32P;

typedef MIDIUInt8       BYTE;
typedef MIDIUInt16      WORD;
typedef MIDIUInt32      DWORD;

//MIDI文件头的结构体
typedef struct
{
    DWORD   dwFlag;             //MThd标识
    DWORD   dwRestLen;          //剩余部分长度
    WORD    wType;              //类型
    WORD    wNbTracks;          //音轨数
    WORD    wTicksPerCrotchet;  //每四分音符的Tick数
}MIDIHeader,*MIDIHeaderP;

//MIDI文件中出现过的标识
#define MIDI_MThd   0x6468544D
#define MIDI_MTrk   0x6B72544D

//MIDI文件头的“类型”
#define MIDIT_SingleTrack   0   /*单音轨*/
#define MIDIT_MultiSync     1   /*同步多音轨*/
#define MIDIT_MultiAsync    2   /*异步多音轨*/

//各种长度的Big-Endian到Little-Endian的转换
#define BSwapW(x)   ((((x) & 0xFF)<<8)|(((x) & 0xFF00)>>8))
#define BSwap24(x)  ((((x) & 0xFF)<<16)|((x) & 0xFF00)|(((x) & 0xFF0000)>>16))
#define BSwapD(x)   ((((x) & 0xFF)<<24)|(((x) & 0xFF00)<<8)|(((x) & 0xFF0000)>>8)|(((x) & 0xFF000000)>>24))

//将音符字节转换成字符串的函数
char*NoteToString(BYTE bNote);

//读取字符串然后打印
size_t ReadStringAndPrint(FILE*,size_t);

//=============================================================================
//ReadDynamicBytes:
//读取动态字节，最多读取4字节。返回读取的字节数
//-----------------------------------------------------------------------------
size_t ReadDynamicBytes(FILE*fp,DWORD*pOut)
{
    size_t bBytesRead;//已读取的字节数
    *pOut=0;
    for(bBytesRead=1;bBytesRead<=4;bBytesRead++)//最多读取4字节
    {
        int iValue=fgetc(fp);
        if(iValue==EOF)
            return 0;
        *pOut=(*pOut<<7)|(iValue & 0x7F);//新读入的是低位
        if(!(iValue & 0x80))//如果没有后续字节
            break;//就停止读取。
    }
    return bBytesRead;//返回读取的字节数
}

//=============================================================================
//ParseMIDI:
//分析MIDI文件。失败返回零，成功返回非零
//-----------------------------------------------------------------------------
int ParseMIDI(char*pszFileName)
{
#   define READERR {fprintf(stderr,"读取文件%s遇到错误\n",pszFileName);goto BadReturn;}
#   define FMTERR {fprintf(stderr,"%s不是一个正常的MIDI文件\n",pszFileName);goto BadReturn;}
#   define READ(x) if(fread(&(x),1,sizeof(x),fp)!=sizeof(x))READERR

    FILE*fp;
    MIDIHeader midiHeader;
    size_t i;

    //打开文件
    fp=fopen(pszFileName,"rb");
    if(!fp)
        READERR;

    //MIDI文件头就是一个MIDIHeader结构体。
    //但是要注意其中的数值都是Big-Endian存储的
    //需要进行转换

    //读取MIDI文件头
    READ(midiHeader);
    
    //检查文件格式
    if(midiHeader.dwFlag!=MIDI_MThd)//标识必须是"MThd"
        FMTERR;

    //转换为Little-Endian
    midiHeader.dwRestLen=           BSwapD(midiHeader.dwRestLen);
    midiHeader.wType=               BSwapW(midiHeader.wType);
    midiHeader.wNbTracks=           BSwapW(midiHeader.wNbTracks);
    midiHeader.wTicksPerCrotchet=   BSwapW(midiHeader.wTicksPerCrotchet);

    //分析文件头
    switch(midiHeader.wType)
    {
    case MIDIT_SingleTrack:
        fputs("类型:单音轨\n",stdout);
        break;
    case MIDIT_MultiSync:
        fputs("类型:同步多音轨\n",stdout);
        break;
    case MIDIT_MultiAsync:
        fputs("类型:异步多音轨\n",stdout);
        break;
    default:
        fprintf(stdout,"类型:未知(0x%04X)\n",midiHeader.wType);
        break;
    }

    //打印音轨数等信息
    fprintf(stdout,
        "音轨数:%u\n"
        "每四分音符时钟数:%u\n",
        midiHeader.wNbTracks,
        midiHeader.wTicksPerCrotchet);

    //正确跳转到MIDI音轨的位置，体现midiHeader.dwRestLen的作用……
    fseek(fp,8+midiHeader.dwRestLen,SEEK_SET);

    //准备读取各个音轨
    for(i=0;i<midiHeader.wNbTracks;i++)
    {
        DWORD dwTrackFlag;
        DWORD dwTrackLen;

        //每个音轨的开头都是一个dwTrackFlag和一个dwTrackLen
        //dwTrackFlag的值必须是MIDI_MTrk
        //dwTrackLen标记了下一个音轨的位置

        size_t TrackStartPos;

        fputs("读取音轨\n",stdout);

        READ(dwTrackFlag);  if(dwTrackFlag!=MIDI_MTrk)FMTERR;//检查文件格式
        READ(dwTrackLen);   TrackStartPos=ftell(fp);//记录当前位置

        //转换Big-Endian
        dwTrackLen=BSwapD(dwTrackLen);

        //显示每个音轨的字节数
        fprintf(stdout,"音轨大小:%u 字节\n",dwTrackLen);

        //音轨的重要内容是事件数据
        for(;;)//循环读取事件
        {
            DWORD dwDelay;
            BYTE bEvent;

            //每个事件的构成都是：
            //延时，事件号，参数
            //其中的延时是动态字节，参数大小随事件号而定

            //上一个事件号
            BYTE bLastEvent;

            //读取延时
            if(!ReadDynamicBytes(fp,&dwDelay))
                READERR;

            //读取事件号
            READ(bEvent);

            //每读取一个事件，打印这个事件的时间
            fprintf(stdout,"时间:%u,",dwDelay);

            //分析事件
ProcEvent:  if(bEvent <= 0x7F)
            {//0到0x7F:和上一个事件的事件号相同，而读取的这个字节就是上一个事件号的参数的第一个字节
                fseek(fp,-1,SEEK_CUR);//回退一个字节
                bEvent=bLastEvent;
				
                goto ProcEvent;
            }
            else if(bEvent <= 0x8F)
            {//0x80到0x8F:松开音符
                BYTE bNote,bVel;

                READ(bNote);//音符
                READ(bVel); //力度

                fprintf(stdout,"(通道%u)松开音符:%s, 力度:%u\n",
                    bEvent & 0xF,//低4位表示通道数
                    NoteToString(bNote),//音高
                    bVel);//力度
            }
            else if(bEvent <= 0x9F)
            {//0x90到0x9F:按下音符
                BYTE bNote,bVel;

                READ(bNote);//音符
                READ(bVel); //力度

                fprintf(stdout,"(通道%u)按下音符:%s, 力度:%u\n",
                    bEvent & 0xF,//低4位表示通道数
                    NoteToString(bNote),//音高
                    bVel);//力度
            }
            else if(bEvent <= 0xAF)
            {//0xA0到0xAF:触后音符
                BYTE bNote,bVel;

                READ(bNote);//音符
                READ(bVel); //力度

                fprintf(stdout,"(通道%u)触后音符:%s, 力度:%u\n",
                    bEvent & 0xF,//低4位表示通道数
                    NoteToString(bNote),//音高
                    bVel);//力度
            }
            else if(bEvent <= 0xBF)
            {//0xB0到0xBF:控制器
                BYTE bReg,bVal;

                READ(bReg);//寄存器号
                READ(bVal);//寄存器值

                fprintf(stdout,"(通道%u)控制器:%u,值=%u\n",
                    bEvent & 0xF,//低4位表示通道数
                    bReg,//寄存器
                    bVal);//值
            }
            else if(bEvent <= 0xCF)
            {//0xC0到0xCF:改变乐器
                BYTE bInstrument;

                READ(bInstrument);//乐器号
                
                fprintf(stdout,"(通道%u)改变乐器:%u\n",
                    bEvent & 0xF,//低4位表示通道数
                    bInstrument);//乐器号
            }
            else if(bEvent <= 0xDF)
            {//0xD0到0xDF:触后通道
                BYTE bChannel;

                READ(bChannel);//通道号
                
                fprintf(stdout,"(通道%u)触后通道:%u\n",
                    bEvent & 0xF,//低4位表示通道数
                    bChannel);//通道号
            }
            else if(bEvent <= 0xEF)
            {//0xE0到0xEF:滑音
                WORD wPitch;//滑音参数是Little-Endian的WORD

                READ(wPitch);
                
                fprintf(stdout,"(通道%u)滑音:%u\n",
                    bEvent & 0xF,//低4位表示通道数
                    wPitch);//滑音
            }
            else if(bEvent == 0xF0)
            {//0xF0:系统码
                BYTE bSysCode=0;

                fputs("系统码:",stdout);
                for(;;)
                {
                    READ(bSysCode);
                    if(bSysCode!=0xF7)
                        fprintf(stdout,"0x%02X,",bSysCode);
                    else
                    {
                        fprintf(stdout,"0x%02X\n",bSysCode);
                        break;//读取到0xF7结束
                    }
                }
            }
            else if(bEvent == 0xFF)
            {//元事件
                BYTE bType,bBytes;
                size_t CurrentPos;
                
                READ(bType);//元数据类型
                READ(bBytes);//元数据字节数

                CurrentPos=ftell(fp);//记录元数据读取的位置

                fputs("元数据 - ",stdout);

                switch(bType)
                {
                case 0x00://设置轨道音序
                    {
                        WORD wTrackSeq;
                        READ(wTrackSeq);
                        wTrackSeq=BSwapW(wTrackSeq);
                        fprintf(stdout,"设置轨道音序:0x%04X\n",wTrackSeq);
                    }
                    break;
                case 0x01://歌曲备注
                    fputs("备注:",stdout);
                    ReadStringAndPrint(fp,bBytes);
                    fputc('\n',stdout);
                    break;
                case 0x02://版权
                    fputs("版权:",stdout);
                    ReadStringAndPrint(fp,bBytes);
                    fputc('\n',stdout);
                    break;
                case 0x03://歌曲标题
                    fputs("标题:",stdout);
                    ReadStringAndPrint(fp,bBytes);
                    fputc('\n',stdout);
                    break;
                case 0x04://乐器名称
                    fputs("乐器名:",stdout);
                    ReadStringAndPrint(fp,bBytes);
                    fputc('\n',stdout);
                    break;
                case 0x05://歌词
                    fputs("歌词:",stdout);
                    ReadStringAndPrint(fp,bBytes);
                    fputc('\n',stdout);
                    break;
                case 0x06://标记
                    fputs("标记:",stdout);
                    ReadStringAndPrint(fp,bBytes);
                    fputc('\n',stdout);
                    break;
                case 0x07://开始点
                    fputs("开始点:",stdout);
                    ReadStringAndPrint(fp,bBytes);
                    fputc('\n',stdout);
                    break;
                case 0x21://音轨开始标识
                    fputs("音轨开始标识.\n",stdout);
                    break;
                case 0x2F://音轨结束标识
                    fputs("音轨结束标识.\n",stdout);
                    goto EndOfTrack;
                case 0x51://速度
                    {
                        BYTE bVelocity1,bVelocity2,bVelocity3;

                        //速度是24位整数，一个四分音符的微秒数。
                        READ(bVelocity1);
                        READ(bVelocity2);
                        READ(bVelocity3);

                        //而且是Big-Endian
                        fprintf(stdout,"速度: 每四分音符%u微秒.\n",
                            bVelocity3|(bVelocity2<<8)|(bVelocity1<<16));
                    }
                    break;
                case 0x58://节拍
                    {
                        BYTE bBeatNumerator,bBeatDenominator,bMetronomeTimer,bNb32ndNotesInCrotchet;

                        READ(bBeatNumerator);//分子
                        READ(bBeatDenominator);//分母
                        READ(bMetronomeTimer);//节拍器时钟
                        READ(bNb32ndNotesInCrotchet);//每四分音符有多少三十二分音符

                        fprintf(stdout,"节拍:%u/%u. 节拍器时钟:%u. 每四分音符有%u个三十二分音符.\n",
                            bBeatNumerator,bBeatDenominator,bMetronomeTimer,bNb32ndNotesInCrotchet);
                    }
                    break;
                case 0x59://调号
                    {
                        MIDIInt8 bTunePitch;
                        BYTE bTuneType;

                        READ(bTunePitch);//升降号
                        READ(bTuneType);//大调、小调

                        fprintf(stdout,"升降号:%d, ",bTunePitch);
                        switch(bTuneType)
                        {
                        case 0://大调
                            fputs("大调\n",stdout);
                            break;
                        case 1://小调
                            fputs("小调\n",stdout);
                            break;
                        default://不知道
                            fprintf(stdout,"未知调(0x%02X).\n",bTuneType);
                            break;
                        }
                    }
                    break;
                case 0x7F://音序特定信息
                    fputs("音序特定信息:",stdout);
                    ReadStringAndPrint(fp,bBytes);
                    fputc('\n',stdout);
                    break;
                default:
                    fprintf(stdout,"未知元数据类型(0x%02X)\n",bType);
                    break;
                }

                fseek(fp,CurrentPos+bBytes,SEEK_SET);//读取完成后正确跳到下一个事件的位置。
            }
            else//其它事件，未知事件
            {
                fprintf(stdout,"未知事件:0x%08X. 停止当前音轨的分析。\n",bEvent);
                break;
            }

            //记录上一个事件号
            bLastEvent=bEvent;
        }//回去继续读取事件。

EndOfTrack:
        fseek(fp,TrackStartPos+dwTrackLen,SEEK_SET);//转到下一个音轨
    }

    fputs("MIDI文件读取结束。\n",stdout);

    fclose(fp);
    return 1;
BadReturn:
    fclose(fp);
    return 0;

#   undef READ
#   undef READERR
}

//=============================================================================
//NoteToString:
//将音符音高转换为字符串
//其实每次返回的都是同一缓冲区，只是每次都改写了缓冲区的值。
//-----------------------------------------------------------------------------
char*NoteToString(BYTE bNote)
{
    static char szBuf[5];//顶多5字符
    switch(bNote % 12)
    {
    case 0:
        sprintf(szBuf,"C%d",(int)(bNote / 12)-2);
        break;
    case 1:
        sprintf(szBuf,"C%d#",(int)(bNote / 12)-2);
        break;
    case 2:
        sprintf(szBuf,"D%d",(int)(bNote / 12)-2);
        break;
    case 3:
        sprintf(szBuf,"D%d#",(int)(bNote / 12)-2);
        break;
    case 4:
        sprintf(szBuf,"E%d",(int)(bNote / 12)-2);
        break;
    case 5:
        sprintf(szBuf,"F%d",(int)(bNote / 12)-2);
        break;
    case 6:
        sprintf(szBuf,"F%d#",(int)(bNote / 12)-2);
        break;
    case 7:
        sprintf(szBuf,"G%d",(int)(bNote / 12)-2);
        break;
    case 8:
        sprintf(szBuf,"G%d#",(int)(bNote / 12)-2);
        break;
    case 9:
        sprintf(szBuf,"A%d",(int)(bNote / 12)-2);
        break;
    case 10:
        sprintf(szBuf,"A%d#",(int)(bNote / 12)-2);
        break;
    case 11:
        sprintf(szBuf,"B%d",(int)(bNote / 12)-2);
        break;
    }
    return szBuf;
}

//=============================================================================
//ReadStringAndPrint
//读取字符串然后打印。返回读取的字节数
//-----------------------------------------------------------------------------
size_t ReadStringAndPrint(FILE*fp,size_t szLength)
{
    char*pBuf;
    pBuf=(char*)malloc(szLength+1);
    if(pBuf)//分配内存一次读取
    {
        size_t Ret=fread(pBuf,1,szLength,fp);
        pBuf[szLength]=0;//结尾的\0
        fputs(pBuf,stdout);
        free(pBuf);
        return Ret;
    }
    else
    {
        fputs("无法将字符串读入内存。\n",stderr);
        return 0;
    }
}

//=============================================================================
//main:
//入口
//-----------------------------------------------------------------------------
int main(int argc,char**argv)
{
    if(argc<2)
    {
        fprintf(stderr,
            "Usage:\n"
            "%s midifile.mid\n",argv[0]?argv[0]:"MIDIFile");
        return 1;
    }

    ParseMIDI(argv[1]);
    return 0;
}