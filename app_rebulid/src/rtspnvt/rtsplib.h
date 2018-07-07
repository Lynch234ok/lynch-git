/******************************************************************************

  Copyright (C), 2013-2020, GuangZhou JUAN Electronics Co., Ltd.

 ******************************************************************************
  File Name    : rtsplib.h
  Version       : Initial Draft
  Author        : kejiazhw@gmail.com(kaga)
  Created       : 2013/04/25
  Last Modified : 2013/04/25
  Description   : rtsp  utils , reference to rfc2326
 
  History       : 
  1.Date        : 2013/04/25
    	Author      : kaga
 	Modification: Created file	
******************************************************************************/
#ifndef __RTSPLIB_H__
#define __RTSPLIB_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <time.h>
//#include "slog.h"
#include "authentication.h"
#include "sdplib.h"
#include "rtplib.h"
#include "rtcplib.h"
//#include "netstream.h"

/****************************************************************
* configure mirco
****************************************************************/
#define RTSP_BUF_SIZE			(1024*8)
#define RTSP_CLIENT_PORT_BEGIN	(5200)
#define RTSP_SERVER_PORT_BEGIN	(7200)
#define RTSP_CHANNEL_BEGIN		(0)

/*
 rtsp version changed log:'
 2014-1-10 : "minirtsp (by kaga)" -> "minirtsp 2.0 (by kaga)"
  			almost add Server : {RTSP_USER_AGENT} ON OPTIONS response", with anything about agent or
  			server info this time before.
2014-5-5 : -> "minirtsp 2.1 (by kaga)"
		   add hearbreak in rtsp session, support( any rtsp option/get parameter/rtcp)
2015-5-6: -> "minirtsp 2.2 (by kaga)"
		 add rtsplib server for zhongshiwenye
		 server-side api
		 support build only-server codes
*/
#define RTSP_USER_AGENT		"minirtsp 2.2 (by kaga)"
#define RTSP_ALLOW_METHODS	"SETUP,OPTIONS,DESCRIBE,PLAY,TEARDOWN,GET_PARAMETER"
/*************************************************************************
* const micro relative to rtsp, must not modified
**************************************************************************/
#define RTSP_DEFAULT_PORT	(554)
#define RTSP_VERSION		"RTSP/1.0"

#define RTSP_MODE_PLAY		(0)
#define RTSP_MODE_RECORD	(1)

// transport
#define RTSP_RTP_OVER_UDP	(FALSE)
#define RTSP_RTP_OVER_RTSP	(TRUE)// interleaved mode
#define RTSP_RTP_AUTO		(2)// using rtp over udp first ,if failed try rtp over rtsp
#define RTSP_RTP_DEF_TRANSPORT (RTSP_RTP_OVER_RTSP)

// stream type
#define RTSP_STREAM_VIDEO	(0x01) 
#define RTSP_STREAM_AUDIO	(0x02)

typedef enum{
	RTSP_STATE_INIT,
	RTSP_STATE_READY,
	RTSP_STATE_PLAYING,
	RTSP_STATE_RECORDING,
	RTSP_STATE_CNT
}RtspState_t;

typedef enum{
	RTSP_METHOD_DESCRIBE = 0,
	RTSP_METHOD_ANNOUNCE,
	RTSP_METHOD_GET_PARAMETER,
	RTSP_METHOD_OPTIONS,
	RTSP_METHOD_PAUSE,
	RTSP_METHOD_PLAY,
	RTSP_METHOD_RECORD,
	RTSP_METHOD_REDIRECT,
	RTSP_METHOD_SETUP,	//8	
	RTSP_METHOD_SET_PARAMETER,
	RTSP_METHOD_TEARDOWN,
	RTSP_METHOD_CNT
}RtspMethod_t;

/*
enum {
	RTSP_EVENT_CONNECTED = 0,
	RTSP_EVENT_CONNECT_FAIL,
	RTSP_EVENT_DISCONNECTED,
	RTSP_EVENT_DESTROYED,
	RTSP_EVENT_PLAY,   // only for server
	RTSP_EVENT_RECORD, // only for server
	RTSP_EVENT_PAUSE, // only for server
	RTSP_EVENT_DATA_RECEIVED, // only for client
	RTSP_EVENT_AUTH_REQUIRE,  // only for client
	RTSP_EVENT_AUTH_FAILED,
	RTSP_EVENT_RTCP_SENDER_REPORT,
	RTSP_EVENT_RTCP_RECEIVER_REPORT,
	RTSP_EVENT_CHECK_ALIVE_FAILED,
};
*/

typedef void (*fRTSP_EVENT_HOOK)(int eventType, int lParam, void *rParam, void *customCtx);


typedef struct _RtspPacket
{
	int cseq;
	int body_size;
	char *body;
}RtspPacket_t;

typedef enum
{
	RTP_AUTO,
	RTP_UDP,
	RTP_TCP,
}enRTP_TRANSPORT;

typedef struct _RtspTransport
{
	int b_interleavedMode;
	int transport;	// udp or tcp
	int client_port;
	int server_port;
	int channel;
	int cast_type;//unicast or multicast
	int work_mode;//record or play
	uint32_t ssrc;
}RtspTransport_t;

typedef int (*fGetH264AVCHook)(char *stream, void *data);

typedef struct _Rtsp
{
	// rtsp role , client or server
	int role;
	int blocal;
	int stream_type; // invalid for rtsp player
	int buffer_time;//unit : ms, only valid for rtsp player
	unsigned int buffer_size;
	int toggle;// run or not
	int data_available;
	time_t m_sync;
	int code_Type;//H265 : RTP_TYPE_DYNAMIC_H265     H264 : RTP_TYPE_DYNAMIC
	//
	int sock;
	int trigger;
	//uri
	char url[200];
	char contentBase[128];
	char ip_me[20];
	int port;
	char stream[128];
	int rtspurl_select;
	//transport
	char peername[20];
	int b_interleavedMode;
	int low_transport;	// udp or tcp
	enRTP_TRANSPORT transport;
	int client_port;
	int server_port;
	int channel;
	int cast_type;//unicast or multicast
	int work_mode;//record or play
	uint32_t ssrc;
	
	RtspState_t state;
	int errcode;
	char session_id[32];
	uint32_t session_timeout;
	unsigned int rtpseq;
	unsigned int rtptime;

	int cseq;
	char allow_method[128];
	int payload_size;
	char payload[RTSP_BUF_SIZE + 1];
	int readed;

	Authentication_t *auth;	
	int bLogin;		// login success or not
	char user[32];
	char pwd[32];
	//
	//RtspStream_t s;

	SessionDesc_t *sdp;
	Rtp_t *rtp_video;
	Rtp_t *rtp_audio;
	Rtcp_t *rtcp_audio;
	Rtcp_t *rtcp_video;

	fGetH264AVCHook get_avc;
	fRTSP_EVENT_HOOK eventHook;
	void *eventCustomCtx;
	//
	char agent[128]; // option , UserAgent or Server domain value
}Rtsp_t;



// 
#define RTSP_MAX_STREAM		(64)
#define RTSP_MAX_STREAM_LEN	(64)
typedef struct _streamtable
{
	int entries;
	char stream[RTSP_MAX_STREAM][RTSP_MAX_STREAM_LEN+1];
	char media[RTSP_MAX_STREAM][RTSP_MAX_STREAM_LEN+1];
}RtspStreamTable_t;

typedef struct _h264avc
{
	unsigned char sps[256];
	unsigned char pps[256];
	int sps_size;
	int pps_size;
	int with_startcode;
}H264AVC_t;

extern int RTSP_keep_liveness(Rtsp_t *rtsp);
extern int RTSP_request_play(Rtsp_t *r);
extern int RTSP_request_pause(Rtsp_t *r);

// global interface for rtsp client and rtsp server
// rtsp player
extern Rtsp_t* RTSP_CLIENT_init(const char *url,char *user,char *pwd,enRTP_TRANSPORT  transport,int iBufferTime, int bAudio);
extern int RTSP_set_url(Rtsp_t *r, const char *url);
extern int RTSP_set_buffer_size(Rtsp_t *r, int id, unsigned int size);
extern Rtsp_t* RTSP_connect_server(
	Rtsp_t *rtsp, // if given NULL, it would not malloc a new struture of rtsp_t
	char *url,// FORMAT: rtsp://ip:port/stream_name
	char *user,char *pwd, // user login
	enRTP_TRANSPORT transport,//RTSP_RTP_OVER_UDP or RTSP_RTP_OVER_RTSP OR RTSP_RTP_OVER_AUTO
	int iBufferTime);// unit : millisecond
extern int RTSP_connect_server2(//it similar to preivous interface, except if error occur,it,can't destroy itself
	Rtsp_t *rtsp, // if given NULL, it would not malloc a new struture of rtsp_t
	char *url,// FORMAT: rtsp://ip:port/stream_name
	char *user,char *pwd, // user login
	enRTP_TRANSPORT transport,//RTSP_RTP_OVER_UDP or RTSP_RTP_OVER_RTSP OR RTSP_RTP_OVER_AUTO
	int iBufferTime);// unit : millisecond
	
// rtsp server
extern Rtsp_t* RTSP_SERVER_init(int fd,// rtsp socket
	int bAudio);// send audio or not
extern Rtsp_t* RTSP_SERVER_init2(int fd,// rtsp socket
	int bAudio,int typeCode);// send audio or not

//common
//extern int RTSP_init(Rtsp_t *r,int sock,int role);
extern int RTSP_destroy(Rtsp_t *r);
extern int RTSP_cleanup(Rtsp_t *r);//similar to destroy but it don't free rtsp_t


extern int RTSP_read_message(Rtsp_t *r);
extern int RTSP_read_message2(Rtsp_t *r, fRtpParsePacket frtp, fRtcpParsePacket frtcp);
extern int RTSP_parse_message(Rtsp_t *r,
	fRtpParsePacket fRTP,//for interleaved mode, only use for rtsp client,if not use,give it NULL
	fRtcpParsePacket fRTCP);// for interleaved mode, support for client ans server,it give it NULL,it would not parse rtcp packet
// rtsp stream table
extern int RTSP_add_stream(const char *stream_name,const char *media_name);
extern int RTSP_remove_stream(const char *stream_name);
extern int RTSP_find_stream(const char *stream_name,char *media_name);


extern void RTSP_set_event_hook(Rtsp_t *thiz, fRTSP_EVENT_HOOK hook, void *customCtx);

extern int is_rtsp_packet(const void* msg, int msg_sz);

#ifdef __cplusplus
}
#endif
#endif

