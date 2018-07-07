#ifndef JAP2P_SOUP_BASE_H
#define JAP2P_SOUP_BASE_H

#include <pthread.h>
#include <sys/syscall.h>
#include "p2p.h"
#include "common.h"
#include "linkedlist.h"
#include "udxa.h"


#ifndef PLAT_X64
#include "media_buf.h"
#include "sdk/sdk_api.h"
#include "p2p_base/Crypto.h"  //crpto chip
#include "ptz.h"

#ifdef P2P_FOR_IPC
#include "usrm.h"
#define MAX_CAM_CH 1
#else
#include "user.h"
#include "conf.h" //include MAX_CAM_CH
#endif

#ifdef P2P_FOR_DVR
typedef SDK_ENC_BUF_ATTR_t stSDK_ENC_BUF_ATTR;
#endif  //end of P2P_DEVICE_DVR


#define SDK_ENC_BUF_DATA_H264 (0x00000000)
#define SDK_ENC_BUF_DATA_H265 (0x00000001)
#define SDK_ENC_BUF_DATA_JPEG (0x00000002)
		// audio
#define SDK_ENC_BUF_DATA_PCM (0x80000000)
#define SDK_ENC_BUF_DATA_G711A (0x80000001)
#define SDK_ENC_BUF_DATA_G711U (0x80000002)
#define SDK_ENC_BUF_DATA_AAC (0x80000003)



#endif



#define SOUP_CMD_PTZ 1
#define SOUP_CMD_SETTINGS 2
#define SOUP_CMD_STEAMREQ 3
#define SOUP_CMD_AUTH 4
#define SOUP_CMD_DEVINFO 5
#define SOUP_CMD_VCON 6



#define SOUP_SUCCESS 0
#define SOUP_ERR_UNSUPPORT 1
#define SOUP_ERR_PASSWD 2
#define SOUP_ERR_NOPERMISSION 3
#define SOUP_ERR_HEADERROR 4
#define SOUP_ERR_UNDEF = 1024 




#if 0
typedef struct _tagFrameHead{
	uint32_t magic;			// magic number 固定为 0x534f55ff , "SOU·“
	uint32_t version;		// 版本信息，当前版本为1.0.0.0，固定为0x01000000
	uint32_t frametype;	// 码流帧类型，当前版本支持三种类型：0x00--音频 0x01--视频I帧 0x02--视频P帧
	uint32_t framesize;	// 码流帧的裸数据长度
	uint64_t pts;				// 帧时间戳，64位数据，精度到微秒
	uint32_t externsize;	// 扩展数据大小，当前版本为0
	union{
		struct _tagVideoParam{
			uint32_t width;	// 视频宽
			uint32_t height;	// 视频高
			UINT32 enc;	// 视频编码，四个ASIIC字符表示，当前支持的有"H264"
		}v;
		struct _tagAudioParam{
			uint32_t samplerate;	// 音频采样率
			uint32_t samplewidth;	// 音频采样位宽
			UINT32 enc;	// 音频编码，四个ASIIC字符表示，当前支持的有"G711"
		}a;
	}_U;
}SoupFrameHead;

#endif

typedef struct{
	LListNode *m_soup_proto;

	IUdxTcp *m_pTcp;	
	UINT32 m_chn;
	UINT32 m_streamId;
	UINT32 m_streamOpt;	

	//for snd thread
	pthread_t m_tid_snd;
	bool m_bSessionRun; //thread trigger
	bool m_bStreamStop; //stream send trigger
	void *m_snd_buf;
	UINT32 m_snd_buf_sz;
	void *m_mediabuf_user;
	UINT32 m_mediabuf_id;


	LListNode *m_vcons; //vcon session,diff them with the vcon id,from client
	void *m_vcon_load;
	UINT32 m_vcon_loadlen;

	//buf size specify
	UINT32 m_mainstream_buf_size;
	UINT32 m_substream1_buf_size;
	UINT32 m_substream2_buf_size;
}SoupSession;

typedef struct{
	CHAR m_service[64];
	UINT32 m_type;
	struct sockaddr m_addr;
}SoupVconServiceInfo;

typedef struct{
	UINT32 m_id;
	bool m_bTrigger;
	SoupSession* m_pSoupSession;
	SoupVconServiceInfo *m_pServiceInfo;
	INT32 m_sock; //for tcp/udp

}SoupVcon;


class CSoup : public IUdxaHooks
{
public:
	//udxa 's hooks
	virtual void OnUdxaConnect(IUdxTcp *pTcp, int erro);
	virtual void OnUdxaRecv(IUdxTcp *pTcp, BYTE *pData, int len);
	virtual void OnUdxaMsgRecv(IUdxTcp *pTcp, BYTE *pData, int len);
	virtual void OnUdxaDisconnect(IUdxTcp *pTcp);

public:
	LListNode *m_soup_sessions; //soup sessions
	LListNode *m_vcon_services;	 //local services over vcon
	
	CUdxa *m_pUdxa;
};



//soup util's interface
CHAR* soup_getSN(void);
SINT32 soup_ptz(SoupSession *thiz, CHAR *chl, CHAR *act, CHAR *param1, CHAR *param2, CHAR *ticket);
SINT32 soup_streamreq(SoupSession *thiz);
bool soup_auth(SoupSession *thiz, CHAR *usr, CHAR *psw);
SINT32 soup_devinfo(SoupSession *thiz);
SINT32 soup_settings(SoupSession *thiz, CHAR *chl, CHAR *method, CHAR *ticket);
SINT32 soup_vcon_create(SoupSession *thiz, UINT32 id, SoupVconServiceInfo *pServiceInfo);
//relay the data to local service which recved from client
SINT32 soup_vcon_relay(SoupSession *thiz, UINT32 id, void *buf, UINT32 len);
SINT32 soup_vcon_destroy(SoupSession *thiz, UINT32 id);


#endif  // end of soup_base.h
