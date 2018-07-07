
// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the FASTUDX_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// FASTUDX_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.

#ifndef FASTUDX_H
#define FASTUDX_H

#define UDXMAXVER	1
#define UDXSLAVER	998


// //// //// //// //// //// //// //// //
// 1.84 主要把内核改成了多线程，接收和发送在IO,逻辑上都区分开了，并发支持更好，响应速度提高
// 1.86	对新内核的一次各方面优化和修改BUG,较稳定版本
// 1.87 修改了定时器的bug,并对每个联接增加了，UdxTrackData,包括20个用户公共保存区，长度为sizeof(INT64)*20
// 1.88 修改了64位编译的bug,增加静态64位lib,.a文件
// 1.90 扩展了最大窗口为16k，在高延迟，GM高速、高延迟网络吞吐量，增加了ACK分片功能。缺点（不支持早其1.90以前版本）
// 1.91 对拥塞控制进行了优化，包括ack发送频率调整，防止不必要的重传
// 1.92 优化内存池回收，其他小BUG调整，比如缓冲控制部分
// 1.93 去掉了文件传输接口的Accept方法，已经过时，弃用了，另外实现了cancel方法，以前版本没有实现,但是引入了联接超时事件不能触发的BUG
// 1.94 增加IMulcardUdx支持多个网络（网卡,3G卡)绑定一个UDX对象，进行单个流的收发，主要用在移动设备，车载系统中
// 1.95 修改1.93引入的联接超时不能触发事件的bug. 去掉几个重复弃用的变量
// 1.96 修改了内存COPY的效率问题，减少了一次COPY，本地提高5% CPU执行效率，及线程调度，包括联接释放过程，增加了稳定性
// 1.97 增强了文件输过程中，存在同样文件时，立即返回发送成功，另外修改了流量探测参数
// 1.98 IFastUdx接口增加了是否分片选项，另外把回调事件独立出来，完全不影响，数据包的组包处理。
// 1.991 增加了多线程并行计算策略，修复了在SetFloatSize超过16MB时，ACK超过1500字节的BUG，增加IMediaPush接口发送音视频，防止花屏,简化速度计算算法
// 1.992 修复非合并包填包BUG,增加了文件校验机制
// 1.993 修改linux下由于fork子进程时，时钟被提前释放问题
// 1.994 修改OnStreamNeedMoreData(this)->OnStreamNeedMoreData(this,needdata),增加IMediaPush回调OnMediaPushFrameEvent，当发送缓冲满时，提示音视频有多少侦缓冲，可通过侦数缓冲，减少延迟
//		 增加事件OnStreamFinalRelease，当联接被释放前，回调用于清理一些关联用户自定义数据
// 1.995 对发送队列过行了一些调整优化，对事件句柄管理进行调整，修改IMediaPush的包头结构把SID从short改成int,配合分布式传输
//		 调整缓冲出队入队规则，对内存池总量限制，增加log禁止功能，及对外输出功能
// 1.996 修正1.995中，为CBuffmaplist中优化发送队列引入流量突起的新bug。
// 1.997 修正ACK中分片问题
// 1.998 修改IMulcardUdx的实现，全面支持多卡
// // 

// MS VC++ 10.0 _MSC_VER = 1600
// MS VC++ 9.0 _MSC_VER = 1500
// MS VC++ 8.0 _MSC_VER = 1400
// MS VC++ 7.1 _MSC_VER = 1310
// MS VC++ 7.0 _MSC_VER = 1300
// MS VC++ 6.0 _MSC_VER = 1200
// MS VC++ 5.0 _MSC_VER = 1100


#ifdef WIN32
	#if _MSC_VER >= 1600 // for vc8, or vc9 vs 2008 ~ vs2013

#include <WinSock2.h>
#include <windows.h>

#include <MMSystem.h>
#include <assert.h>

#ifndef _cplusplus
#include <atlbase.h>
#endif

#include <fcntl.h>

#include <process.h>
#include <io.h>

typedef unsigned int UDP_LONG ;
typedef unsigned short UDP_SHORT ;
typedef BYTE UDP_BYTE ; 

#pragma comment(lib,"ws2_32.lib")


	#else
		#include "udxos.h"
	#endif
#else
	#include "udxos.h"
#endif


enum ERROCODE
{
	//errocode : 0,成功，1，新的联接到来，2，远程拒绝联接,3超时
	UDX_CON_SUCCEED,
		UDX_CON_NEWCON,
		UDX_CON_EJECT,
		UDX_CON_TIMEOUT,
		UDX_CON_SELF
};

#ifdef WIN32
#pragma pack( push, 1 )
#define UDXPACKED 
#else
#define UDXPACKED	__attribute__((packed, aligned(1)))
#endif

typedef void (CALLBACK UDXPRC)(int eventtype ,int erro,long s, BYTE* pData, int len);
typedef void (CALLBACK UDXP2PPRC)(void* addrRemote,int errocode,char* user1,char* user2,INT64 dwuser);

typedef UDXPRC FAR *LPUDXPRC;
typedef UDXP2PPRC FAR *LPUDXP2PPRC;


	//对地址的一些转换，一般不必使用
class IUdxTools
{
public:
	virtual void IPPort2Addr(char* pIP, WORD wPort, SOCKADDR* pAddr) = 0; //将ip,port转化为地址
	virtual INT64 Addr2Int64(SOCKADDR* pAddr,WORD streamId) = 0;//将地址与一个WORD转化为一个64位常量
	virtual void Int642Addr(INT64 key,SOCKADDR* pAddr,WORD &streamId) = 0;//由Addr2Int64常量转化为地址
	virtual void GetIdInfo(INT64 id,char* buff,WORD &streamid) = 0;//通过ip信息得到流ID
	virtual void TraceAddr(SOCKADDR* pAddr) = 0;//打印
	virtual void GetSpeedStr(char * buff,INT64 speed  ) = 0;//得到发送或接收的速度字符串
	virtual void Trace(const char* strLog) = 0;
};

#define UDXCHANNELCOUNT	2	//通道个数
#define MSGID	0			//消息通道
#define DATAID	1			//数据通道

class IUdxInfo									//udx当前信息，大部分信息为主通道信息
{
public:
	INT64 m_dwRead;								//当前接收到的数据长度,已经确认的
	INT64 m_dwWrite;							//当前接发送的数据长度,已经确认的
	INT64 m_ReadCount;							//当前接收到的包数,已经确认的
	INT64 m_WriteCount;							//当前接发送的包数,已经确认的
	INT64 m_SendTotalCount;						//总共发送的总包数,包括重传部分
	INT64 m_ReSendCount;						//快速重传的包数

	INT64 m_errocount;							//校验错的包数
	INT64 m_dwDumpCount;						//收到的重包数
	INT64 m_dwOutRange;							//收到的,不在接收窗口中的包数
	DWORD m_start;						//当前起始时间
	DWORD s1,e1,s2,e2,m_sendindex;				//当前收发的起始序号,及当前缓冲区中的待发最大送序号
	DWORD m_ackcount;							//收到的ACK数量
	INT64 m_currentspeedread;					//当前接收速度,字节/秒,已经确认的
	INT64 m_currentspeedsend;					//当前发送速度,字节/秒,已经确认的
	DWORD m_lastUpdateTime;						//上次更新udxinfo时间 
	
	int m_ttl;									//当前的往返时间
	int m_minttl;								//此链轮的最小RTT
	int m_SecSendSize;							//每个统计周期发送的长度
	int m_SecReSendSize;						//每个统计周期重发的数据长度
	
	INT64 m_uncheckcount;						//发送了但没有确认的包数
	INT64 m_checked;							//已经确认的包数
	INT64 m_expect;								//期 望发送的包数
	INT64 m_buffsize;							//当前缓冲中存在的包数
	INT64 m_SendBewControl;						//每个统计周期发送的速度
	INT64 m_WillBeSendSize;						//即使需要发送的速度（量）

	int m_sendsyncount;							//发送的同步包个数
	int m_readsyncount;							//接收到的同步包个数

	INT64 m_SendBuffCount[UDXCHANNELCOUNT];		//成功调用sendbuff的次数
	INT64 m_WriteBuffCount[UDXCHANNELCOUNT];	//发送成功的次数
	INT64 m_ReadBuffCount[UDXCHANNELCOUNT];		//接收到由sendbuff产生的接收包次数
	
	virtual void GetSpeedStr(char * buff,BOOL bSend = TRUE,BOOL bCurrent = FALSE)=0;			//得到实时/平均速度,字符串
	virtual DWORD GetCurrentSpeed(BOOL bSend = TRUE)=0;											//得到当前速度
	virtual DWORD GetSpeed(BOOL bSend = TRUE)=0;												//得到平均速度
	virtual char* GetInfo()=0;																	//得到字符串,当前调试信息
	virtual void Reset()=0;																		//重新计时
	virtual void UpDateCurrentSpeed()=0;														//刷新各种信息
	virtual float GetLostRate() = 0;															//当前丢包率
	
	//备注:
	//当联接建立以后,各种参数都会自动在UDX内部去不断更新,但是,应用层也可以调用reset进行重新计算,但是调用RESET不会引
	//响UDX内部传输,只是作为应用层的参考数据.
}UDXPACKED;


struct IUdxCfg									//单个UDX的一些设置
{
	int mode;//设置Fastudx.cfg.mode=1;表示，所有生成的UDXTCP必须CPY 全局配置，否则按默认配置来
	int maxdatabuffwnd[2];// 
	int submss;									//当前连接的MSS,目前默认为1024，最大为1400，一般情况下不需要更改这个值。
	int maxlostrate;							//最大丢包率，default(350/1000==35% ),输入范围(1~1000)
	int expectbew;								//预估流量（B/秒），
												//当实际流量超过时会作用拥塞算法，否则拥塞算法影响较小,可用在实时的应用中，
												//因为可以估算大致流量，这样不会因为初使窗口过小，造成多次传输，影响到延迟
	int maxsendbew;					//单条连接最大流量(B/S)
	int flagfriend;					//友好标志，非fastmode的时候，1.表示，当发生连续丢包，超过丢包率时，会主动放慢发送速度
	int mergeframe;					//组包发送 1.当有多个小于MSS的数据包，还来不及发送时，可能会被UDX合并成一个MSS发送，以节约包头，提高发送效率. 0.不合并
	int brokenframe;				//分包发送 1.当包送时，可能，多个MSS合并时，最后一个包，可能会被分隔成两个包，比如：（800 + 600 > mss(1024)),同样二个包(800,600)会被分成(1024,1400<800+600> - 1024<mss> = 376) 0. 反之
	int fastmode;					//只追求最大速度，不理会丢包，0.默认配置，由预定丢包率控制。1.忽略丢包率控制，只追求最大有效数据。
	int fixbew;						//固定流量
	int filetransmode;				//文件传输模式
	int delaytosend;				//延迟发送，当发送的数据<MSS时，会延迟50MS发送，合并多个小包一起发送，默认会没有打开
	int segmentspace;				//失序间隔，默认是3，（1~5)，不建议修改
	int maxackcheckcount;			//默认UDX50MS应答一批包，设置这个后，可以规定多少包后立即进行回应ACK
	int maxlocalackchecktime;			//默认UDX50MS应答一批包，设置这个后，可以规定多少MS立即进行回应ACK
	int maxremoteackchecktime;			//默认UDX50MS应答一批包，设置这个后，可以规定多少MS立即进行回应ACK
}UDXPACKED;

class IUdxLogSink
{
public: 
	virtual void Log(char* str) = 0;//内部UDX写日志的信息，通过这个接口导出，应用层可以从这个派生后，写LOG
};

struct IUdxGlobalCfg
{
	IUdxGlobalCfg(){memset(this,0,sizeof(IUdxGlobalCfg));};
	int bInit;
	int mastver;
	int slaverver;
	int udxclock;				//udx内部时钟,默认25ms
	int udxcheckacktimer;		//udx内部ack回包频率,默认35毫秒
	int udxmintimeout;			//udx内部最小超时300ms
	int udxmaxtimeout;			//udx内部最大超时10000ms
	int udxackcount;			//多少个包回应一个ACK
	int udxdebug;
	DWORD maxsendbew;			//全局上传流量，包括丢的包
	char ext[10];
	IUdxLogSink* pLog;
	int bDisableLog;
}UDXPACKED;

class IUdxUnkownPackSink		//对于非UDX的UDP包，设置了这个回调后，会回调出这些包，给应用处理
{
public:
	virtual void OnUnkownData(SOCKADDR * pAddr,BYTE* pData,long len){};
	virtual void OnThreadExit(){};
};

class IWaitEvent				//UDX通道是否可发数据事件通知
{
public:
	virtual int Wait(DWORD ms) = 0;
	virtual void SetEvent() = 0;
};


struct FileInfo
{
	INT64 len;
	char name[256];
	UDP_BYTE context1[10];
	UDP_BYTE context2[10];
	UDP_BYTE context3[10];
}UDXPACKED;

class IUdxFSink					//文件回调接口
{
public:
	//当远端有新的文件到来请求事件
	virtual int OnNewFile(FileInfo * pInfo,BOOL bContinued){return bContinued + 1;};//1 save new file ,2 continue,else cancel
	//经接收方同意，传输开始，针对传输方
	virtual void OnTransFileBegin(char* filename,BOOL bSend){} ;
	//远程取消息发送
	virtual int OnRemoteCancel(BOOL bSend){return 0;} ;
	//文件传输完成
	virtual void FileTransmitDone(char* filename,BOOL bSend){};//bSend = false时为接收，当收到此事件时，表示文件接收完成，不能立即删除掉UDX对象，
	//当接收完成时，会发送一个通知到发送方，触发发送方的FileTransmitDone消息。过早的关掉接收UDX,会导致发送方收不到发送完成事件。
	//所以，在文件传输时，应该当发送方收到FileTransmitDone事件时再关掉UDX，可以保证两边都可以收到此事件，并保证文件传输完整
	
	//文件数据发送续传点
	virtual void OnFileContinue(INT64 startpoint,BOOL bSend){};
	//文件数据到来多少
	virtual void OnFileReadByts(DWORD dwSize){};
	//文件数据发送多少
	virtual void OnFileWriteByts(DWORD dwSize){};
	
	//提供加密接口
	virtual void OnEncoderByts(BYTE*pData,DWORD dwSize){};
	
	//提供解密接口
	virtual void OnDecoderByts(BYTE*pData,DWORD dwSize){};

	//接收方有一个同名文件，返回1，表示强制重新传 ，其他表示正常流程, 如果正常流程就会取，前10，后10，中间10个字节（三点采样）比对，发现相同就认为是同一个文件，立即返回成功
	virtual int OnCheckSameNameFile(FileInfo * pInfo){return 0;} ;

	//出错信息
	virtual void OnTransFileErro(int errocode){};

	virtual void OnCancelAck(BOOL bSend){};
};

class IUdxFileTransmitor 
{
public:
	//得到当前收发了百分比
	virtual float GetPercent(BOOL bSend) = 0;
	//发送文件
	virtual void SendFile(char* filename) = 0;
	//发送是否完成,当接收方返回真时，不能立即删除UDX对象，保证在发送方进行删除UDX对象
	virtual BOOL IsDone(BOOL bSend) = 0;
	//设置保存文件路径目录
	virtual void SetSaveFileDir(char* savedir) = 0;
	//设置保存文件名，绝对路径
	virtual void SetSaveFileName(char* savename) = 0;
	//设置文件事件回调接口
	virtual void SetFSink(IUdxFSink * pSink) = 0;
	//取消发送或接收取消发送或接收
	virtual void Cancel(BOOL bSend) = 0;
	//得到打开的文件的总长度
	virtual INT64 GetTotalSendfileLen() = 0;
	virtual INT64 GetTotalReadfileLen() = 0;	
	//设置断点间隔长度，如果lBp小于1024字节，结果相当于lBp = 1024.默认是2M，每2M数据写一次文件
	virtual void SetBPLength(long lBp) = 0;
	//得到BP长度
	virtual long GetBPLength() = 0;

	virtual char * GetSendFileName() = 0;
	virtual char * GetReadFileName() = 0;
	//发送文件
	virtual void SendFileW(wchar_t* filename) = 0;
};

class IUdxP2pSink//p2p事件回调接口
{
public:
	virtual void OnP2pConnect(SOCKADDR addrRemote,int errocode,char* user1,char* user2,INT64 dwuser){};//联接回调
};
class IUdxP2pClient//P2P发起接口
{
public:
	virtual void SetSink(IUdxP2pSink * pSink) = 0;
	virtual void SetNatServer(char*mip,long mport) = 0;
	virtual void SetTimeOut(DWORD ms = 30000) = 0;
	virtual BOOL ConnectServer(char* user1,char* user2,INT64 dwUser) = 0;
	virtual SOCKADDR GetLocalAddr() = 0;
	virtual void Start() = 0;
	virtual void Stop() = 0;
	virtual void Clear() = 0;//立即清除之前联接的用户，也可以不管，内部会定时清理
	virtual void SetCB(LPUDXP2PPRC pcb) = 0;//设置P2P 回调指针
};
#define UdxTrackData_Len	30

struct UdxTrackData
{
	INT64 data[UdxTrackData_Len];
};

#ifndef _streammedialib_h

#define AUDIOFRAME_A	0
#define VIDEOFRAME_I	1
#define VIDEOFRAME_P	2
#define DATAFRAME_I		3

struct UdxFrameType
{
	int			  sid;
	unsigned char type1:2;
	unsigned char type2:6;
}UDXPACKED;

#endif

#ifdef WIN32
#pragma pack( pop)
#endif

class IMediaPush
{
public:
	//往UDX里面填入音视频侦,内部会call SendFrames,也可以外部主动SendFrames
	//当接收方收到数据时，数据第一字节为一个BYTE表示，这里的type,数据在紧跟在后面，也就是OnStreamRead(pData + 1<实际数据指针>,len - 1 <实际长期度> );
	virtual void SendFrame(int sid,BYTE* pData,int len,int type1/* AUDIOFRAME_A or VIDEOFRAME_I or VIDEOFRAME_P */,int type2) = 0;

	//设置最大缓冲的音频侦数，当网络拥塞时，能缓存的最大包数，用户可以根据，采集的周期 * maxcount = 最大语音延迟时间
	virtual void SetAudioFrameMaxCount(int maxcount) = 0;

	virtual void SendFrames() = 0;
	//清除缓存的音视频数据，恢复到初使状态，此时可以强制编码器产生I侦，进行缓冲音视频
	virtual void Reset() = 0;
};

class IFastUdx;
class IUdxTcp : public IUdxFileTransmitor //==============    UDX 单条连接接口       ====================
{
public:
	virtual long AddLife() = 0;							//给连接增加引用计数，类是于com组件，这样可以在第二个线程中操作这个联接，而不会出问题
	virtual void Destroy() = 0;							//断开这个联接
	virtual BOOL IsConnected() = 0;						//联接是否正常
	virtual BOOL IsFullBuffs(int ich) = 0;				//现在是否不能发数据了
	virtual void SetBuffWindow(int ich,DWORD size) = 0;	//设置缓冲大小，以字节为单位,缓冲长度受用户设置SetFloatDataSize的限制
	virtual void SetMaxDataWindowSize(int ich,DWORD size) = 0;//设置窗口大小，以包数量为单位，可以为（8，16，32，64，...，8092~ 最大（8092*2））
	virtual BOOL SendBuff(BYTE* pData,int len) = 0;		//返回真，表示，指定数据已经成功拷贝到UDX缓冲中，缓冲长度受用户设置SetFloatDataSize的限制
	virtual BOOL SendMsg(BYTE* pData,int len) = 0;		//发送消息
	virtual IUdxInfo* GetUdxInfo() = 0;					//得到内部一些信息
	virtual IUdxCfg * GetUdxCfg() = 0;					//单个联接的，相关配置项
	virtual UDP_SHORT GetStreamID() = 0;				//本地的流ID
	virtual UDP_SHORT GetDesStreamID() = 0;				//联接成功后，远端的本地流ID
	virtual INT64 GetUserData() = 0;					//与联接关联的用户数据，可以是指针，自定义数据
	virtual void SetUserData(INT64 dwUser) = 0; 
	virtual IWaitEvent* GetWaitEvent() = 0;				//联接，当有数据可发时的事件对象指针
	virtual SOCKADDR *GetRemoteAddr() = 0;				//联接成功后，远端的IP信息
	virtual int __DSendUdxBuff(SOCKADDR * pAddr ,BYTE* pData,int len) = 0;		//可以用来发送非可靠数据
	virtual DWORD GetAppBuffAndUdxWndBuffSize() = 0;	//得到当前拷贝到udx内部的数据长度，这些数据还没有得到确认
	virtual DWORD GetBuffWindow(int ich) = 0;			//得到应用层设置的缓冲长度，这个不包括内部窗口中含有的长度
	virtual void SetTimeOut(int con,int hardbeat,int contimeout) = 0;		//设置超时，分别为，联接超时，心跳间隔，和联接保活时间，秒为单位
	virtual UdxTrackData* GetUdxTrackData() = 0;		//得到额外的与联接共享的内存关联
	virtual void SetFloatDataSize(int floatdatasize) = 0;		//设置应用缓冲长度,如果没有设置时为0，就例用默认缓冲长度8M，只针对数据通道有效，消息通道不受这个影响
	virtual int GetFloatDataSize() = 0;					//得到之前设置的缓冲大小。
	virtual void Close() = 0;							//如果联接是正常的情况下，会断开联接，否则，什么也不会做
	virtual long ReleaseLife() = 0;						//减少由addlife引起的计数
	virtual void DetectReadedBuffSize(long &readsize,long &writesize) = 0;	//用户检测，在没有触发OnStreamwrite/OnStreamRead事件中的未处理的数据长度
	virtual IMediaPush* GetMediaPush() = 0;				//得到push接口,用此接口来推送音视频侦，可以防止花屏
	virtual IFastUdx * GetFastUdx() = 0;				//返回IFastUdx对象
	virtual BOOL IsTransLink() = 0;//返回是否为中转联接
	virtual SOCKADDR *GetTransServerAddr() = 0;//返回当前走的中转IP
};

//技巧：
// Q1 单线程中，如果我创建了TCP对象，调用了AddLife
// 关闭时 我可以
// 1.1 先调用ReleaseLife然后调用Close；
// 1.2 先调用Close 然后调用ReleaseLife
// 1.3 直接调用Destory。
// 
// Q2 单线程中如果 我创建了TCP对象，没有调用AddLife
// 关闭时 调用destroy,不必严格遵守计数，因为没有第二个线程访问同一个对象。
// 
// Q3 多线程中，如果想复制TCP，那么有几个线程，就要调用
// 几次AddLife，根据需要，不同的线程退出前可以调用Destory ，
// 或者ReleaseLife，但是必须有一个线程最终要调用关闭即：
//    Destory 或者 Close + ReleaseLife


class IMultCardTcp;
class IUdxTcpSink
{
public:
	virtual void OnStreamPreConnect(SOCKADDR *pAddr,IUdxTcp * pTcp,int erro){};	//在此接口中，可设置各项参数
	virtual void OnStreamConnect(IUdxTcp * pTcp,int erro){};
	virtual void OnFileStreamConnect(IUdxTcp * pTcp,int erro){};				//0,表示新的一个文件传输会话，1，会话结束
	virtual void OnStreamBroken(IUdxTcp * pTcp){};								//断开
	virtual void OnStreamRead(IUdxTcp * pTcp,BYTE* pData,int len){};			//读
	virtual void OnStreamWrite(IUdxTcp * pTcp,BYTE* pData,int len){};			//写
	virtual void OnStreamMsgRead(IUdxTcp * pTcp,BYTE* pData,int len){};			//消息的读
	virtual void OnStreamMsgWrite(IUdxTcp * pTcp,BYTE* pData,int len){};		//消息的写
	virtual void OnStreamNeedMoreData(IUdxTcp *pTcp,int needdata){};			//可以发起新的写,有needdata(<=0表示可以发数据)字节空间供发送，这个一般在有数据被确认后触发
																				//这个回调有点类是于完成通知，这个只是有包被确认的通知，所以，应用层可以利用他不断的进行投递
																				//而不需要启动额外的线程来完成发送。第一次sendbuff后可以通过这个事件不断叠加发送。

	virtual void OnInteranlThreadExit(){};										//内部线程释放时，处理，这一般给象java,com,jni中使用，用于释放资源
	virtual void OnStreamLinkIdle(IUdxTcp *pTcp){};								//当链路保持，但是应用层没有数据可发时，
																				//会收到心跳，这是心条的处理函授，用户可以打印调试信息，证明联接正常。一般不处理。
	virtual void OnStreamChancetoFillBuff(IUdxTcp *pTcp){};						//UDX内部发送线程提供机会，进行发送数据,每50MS固定频率调用
	virtual void OnMediaPushFrameEvent(int sid,int type,int framecount){};		//sid,当发送时，出现SendFrame失败时，回调并报告实际缓冲了多少侦数据,0语音包1视频侦
	virtual void OnStreamFinalRelease(IUdxTcp * pTcp){};						//每个联接最后释放事件，可以处理一些关联的对象释放,不同于OnStreamBroken指收到断开消息就会调用，broken事件我们可以(清除掉所有引用releaselife)，使其进入OnStreamFinalRelease
																				//程序内部可能还有可能其他保存了对应的联接的信息，如果做过早的释放动作会引起不同步的问题
																				//这个回调是每一个联接的最后释放入口，当所有应用层对这个联接没有了引用保护(addlife)
	virtual void OnMultCardConnect(IMultCardTcp * pTcp,int erro,int linkcount){};
	virtual void OnMultCardBroken(IMultCardTcp * pTcp){};
	virtual void OnMultCardRead(IMultCardTcp * pTcp,BYTE*pData,int len){};
};

class IUdxTrans
{
public:
	virtual void OpenChannel(char*ip,UDP_SHORT port,char* strCName) = 0;		//打开一个中转通道
	virtual void CloseChannel(char*ip,UDP_SHORT port,char* strCName) = 0;		//关掉一个中转通道
	virtual void CloseAllChannels() = 0;										//关掉所有的中转通道
};

struct IFastPreCfg : public IUdxCfg//在生成一个IUdxTcp接口时，预先设置的一些固有属性 
{
};

class IFastUdx//UDX实例对象 ==============    UDX 核心对象       ====================
{
public:
	virtual BOOL Create(char* ip = NULL,UDP_SHORT port = 0) = 0;
	virtual BOOL AddBinding(char* ip,UDP_SHORT port) = 0;
	virtual IUdxTcp* Connect(char* ip,UDP_SHORT port,BOOL bSync = FALSE,INT64 dwUser = 0,INT64 expectbew = 0,int lostrat = 50,char*strCName = NULL,IMultCardTcp* pMultCard = NULL) = 0;
	virtual	BOOL Attach(SOCKET s) = 0;//绑定已经存在的UDP套接字
	virtual int Dettach() = 0;
	virtual void Destroy() = 0;//销毁UDX对象
	virtual void SetUnkownPackSink(IUdxUnkownPackSink *pUnkownPackSink) = 0;//设置非UDX的UDP包时回调类
	virtual void SetSink(IUdxTcpSink * pSink) = 0;//设置基本事件的回调类
	virtual void SetServerBlockSize(int size) = 0;//设置内部数据流程处理线程数，比如，逻辑处理数，事件回调线程数，IO线程数等
	virtual SOCKADDR *GetLocalAddr() = 0;//得到默认UDX端口占用信息
	virtual IUdxP2pClient* GetP2pClient() = 0;//得到P2P接口
	virtual int __DSendUdxBuff(SOCKADDR * pAddr ,BYTE* pData,int len) = 0;//直接向指定的地址发送非UDX 原始数据包
	virtual IUdxTrans* GetUdxTrans() = 0;//得到中转传输接口
	virtual void SetUdpSendThreadCount(int count) = 0;//设置内部发送UDP的线程数
	virtual void SetConnectTimeOut(int secs) = 0;//设置新联接超时
	virtual void Enable_DONTFRAGMENT(BOOL bEanble) = 0;//设置底层UDP是否允许分片，默认不允许分片
	virtual int GetClientsCount() = 0;//返回总的联接数，但这个联接数不是实时的
	virtual IUdxCfg* GetPreCreateUdxTcpCfg() = 0;//设置一个预先配置的配置
	virtual void SetCB(LPUDXPRC pcb) = 0;
	virtual IMultCardTcp* CreateMultCard() = 0;
};

class IMultCardTcp //多个网卡 绑定后，同时为一个流发送数据，或接收数据
{
public:
	virtual BOOL AddBinding(int index,char* ip,UDP_SHORT port) = 0;//增加绑定IP
	virtual BOOL Connect(char*ip,UDP_SHORT port) = 0;//通过指定的IP联接对方IP + port
	virtual void Close() = 0;//断开联接
	virtual void Destroy() = 0;
	virtual BOOL SendBuff(BYTE* pData,int len) = 0;
	virtual void AddLife() = 0;
	virtual BOOL IsConnected() = 0;
	virtual UDP_SHORT GetMultCardID() = 0;//取得本联接的ID号
	virtual UDP_SHORT GetMultCardDesID() = 0;//取得本联接的对端ID号
	virtual void SetFloatSize(int floatsize ) = 0;//设置缓冲数
	virtual BOOL GetRemoteIPList(char* pList,int &bufflen) = 0;//（IP:PORT空格） ip-port,返回false表示缓冲不够，bufflen返回需要的长度,空格分隔
	virtual BOOL GetLocalIPList(char* pList,int &bufflen) = 0;// ip-port,返回false表示缓冲不够，bufflen返回需要的长度,空格分隔
	virtual IWaitEvent* GetWaitEvent() = 0;
};


class IUdxLock//为了在编写跨平台代码时，提供的锁
{
public:
	virtual void Lock() = 0;
	virtual void Unlock() = 0;
	virtual void Destroy() = 0;
};

class IUdxEvent //事件对象
{
public:
	virtual void SetEvent() = 0;
	virtual void ResetEvent() = 0;
	virtual BOOL Wait(int ms) = 0;
	virtual void Destroy() = 0;
};

class IUdxBuff
{
public:
	virtual BYTE* ChangePoint(int iLen) = 0;//动态扩展缓存大小
	virtual BYTE* GetBuff(void) = 0;//取得缓存头指针
	virtual int GetLen(void) = 0;//返回缓冲实际使用长度
	virtual int GetMaxLen(void) = 0;//返回最大缓冲长度
	virtual void Zero() = 0;//清0
	virtual void FreeMem() = 0;//释放内存
	virtual void Reset() = 0;//清0，并把使用长度清0
	virtual void Bind(BYTE* pData,int len) = 0;//把指定内存copy到本对象中
	virtual BOOL Pop(BYTE* pData,int len) = 0;//从指针头，弹出指定长度len的数据，内容cpy到pData中，原缓冲长度会减少len
	virtual void AppendBuff(BYTE* pData,int iLen) = 0;//追加一部分缓冲
	virtual void Destroy() = 0;//释放自己
	virtual long AddLife() = 0;//增加引用计数
};

class IUdxFifoList //先进先出队列
{
public:
	virtual void AddBuff(IUdxBuff * pBuff) = 0;//增加一个buff对象
	virtual IUdxBuff* GetBuff() = 0;//取出一个完整的ibuff对象
	virtual void AddBuff(BYTE * pData,int len) = 0;//压入指定的长度数据
	virtual void GetBuff(BYTE** ppData,int &len) = 0;//取出指定长度数据
	virtual void FreeBuff(BYTE* pData) = 0;//释放由getbuff(BYTE**)指针所指内存
	virtual int GetBuff(BYTE* pData,int len) = 0;//给定指度长度的缓冲填充pData,原数据批针后移len
	virtual int GetBuffSize() = 0;//返回list中的数据总长度
	virtual int GetBuffCount() = 0;//返回list中IBuff的数量
	virtual void EnableEvent(BOOL bEnable) = 0;//是否与事件关联
	virtual BOOL Wait(int ms) = 0;//当与事件关联后，当有新数据加入时，会触发事件
	virtual void Clear() = 0;//清除所有数据
	virtual void Destroy() = 0;//释放自己
	virtual void SetWaitEvent(IUdxEvent* pEvent) = 0;//关接接收事件
};

class IUdxTime //时间tick
{
public:
	virtual DWORD GetTickCount() = 0;
	virtual void Destroy() = 0;
};

class IUdxThreadSink //线程回调
{
public:
	virtual void UdxRun(){};
};
class IUdxThread  //线程接口
{
public:
	virtual void Start() = 0;
	virtual void Stop() = 0;
	virtual void SetCB(IUdxThreadSink* pSink) = 0;//设置线程回调类，该类必须实现UdxRun()
	virtual void Destroy() = 0;
};

class IUdxFile  //文件操作接口
{
public:
	virtual BOOL OpenFile(char* strFile,BOOL app = FALSE) = 0;//打开已经存在的文件
	virtual BOOL OpenFileW(wchar_t* strFile,BOOL app = FALSE) = 0;
	virtual BOOL CreateFile(char* strFile) = 0;//创建文件
	virtual void Close(void) = 0;//关闭
	virtual BOOL IsOpen(void) = 0;//文件是否已经被打开
	virtual INT64 GetFileLength() = 0;//返回文件长度
	virtual INT64 Write(BYTE* pData, int len) = 0;//写入文件
	virtual INT64 Read(BYTE* pData, int len) = 0;//读文件，返回真正读入的长度
	virtual void Seek(int type = 1) = 0;//seek到头0，或尾1
	virtual void SeekTo(int type,INT64 len) = 0;//type 0从头开始，1尾开始 len，可为负值，是反方向
	virtual void Destroy() = 0;//释放自己，并关掉文件
};

IUdxGlobalCfg* GetUdxGlobalCfg();		//UDX的全局变量接口，用来设置一些全局参数
IFastUdx* CreateFastUdx();				//创建一个UDX对象
IUdxTools* GetUdxTools();				//UDX提供的辅助工具，一般用不到
IUdxLock* CreateUdxLock();				//创建一个锁
IUdxEvent* CreateUdxEvent();			//创建事件
IUdxFifoList* CreateUdxList();			//创建一个先进先出队列
IUdxTime* CreateUdxTime();				//创建tick对象
IUdxThread* CreateUdxThread();			//创建线程对象
IUdxFile* CreateUdxFile();				//创建udx文件对象
IUdxBuff * CreateUdxBuff(int type);		//创建一个buff对象
void UdxClean();						//一般情况下，不需要调用，当在WIN下面如果你的模块是dll,ocx,sys,ax必须调用这个清理动作，防止退出死锁，这是由于
										//waitforsingleobject引起，PS: LINUX不需要调用

#endif
