#include "p2pdevice.h"
#include "p2psdk.h"

#include "p2p_json_parse.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include  <unistd.h>
#include <secure_chip.h>
//#include "stdinc.h"
#include "media_buf.h"
#include "soup_base.h"

#include "netsdk.h"

#include "app_debug.h"
#include "custom.h"
#include "tfcard.h"
#include "fisheye.h"
extern "C" {
#include "sound_queue.h"
#include "app_gsensor.h"
}
#ifdef VIDEO_CTRL
#include "app_video_ctrl.h"
#endif
#include "model_conf.h"
#include "../tfcard/tfcard_play.h"
#include "global_runtime.h"

#define msleep(x) usleep(x*1000)
#define BUFFER_MAX_SIZE 	(320 * 2)
#define BUFFER_SIZE		(320)

#define P2P_STRUCT_MEMBER_POS(t,m)  ((unsigned long)(&(((t *)(0))->m)))
#define P2P_REF_STREAM_NUM	(64)


#ifdef GSENSOR
#define GSENSOR_INT_FRAME_COUNT     15  // GSENSOR_INT_FRAME_COUNTå¸§è§†é¢‘*66ms(å¸§çŽ‡15)
#endif

typedef struct Rec_frame{
	stSDK_ENC_BUF_ATTR *rec_attr;
	void *raw_buff;
	unsigned frame_maxlen;
	int flag;
	int recType;
	int utc_t;	
}Recframe_f;


typedef struct Recctx{
	stSDK_ENC_BUF_ATTR *rec_attr;
	void *raw_buff;
	size_t frame_maxlen;
	int flag;
	int recType;
	int utc_t;
}Recctx_f;

enum{
	P2P_BYPASS_FRAME_TYPE_FISHEYE_PARAM = 0,
	P2P_BYPASS_FRAME_TYPE_LENS_PARAM,
	P2P_BYPASS_FRAME_TYPE_GSENSOR_PARAM,
	P2P_BYPASS_FRAME_TYPE_CNT,
};

enum{
	eP2P_IMAGE_FISHEYE_FIX_MODE_WALL=0,
	eP2P_IMAGE_FISHEYE_FIX_MODE_CEIL,
	eP2P_IMAGE_FISHEYE_FIX_MODE_TABLE,
	eP2P_IMAGE_FISHEYE_FIX_MODE_NONE,
};

typedef struct fisheyeParam
{
	struct
    {
        int CenterCoordinateX;
        int CenterCoordinateY;
        int Radius;
        int AngleX;
        int AngleY;
        int AngleZ;
    }param[2];
	int fixMode;
	int version;
	int Reverse;
}ST_FISHEYE_PARAM, *LP_FISHEYE_PARAM;

typedef struct fisheyeParam2
{
	struct
    {
        float CenterCoordinateX;
        float CenterCoordinateY;
        float Radius;
        float AngleX;
        float AngleY;
        float AngleZ;
    }param[2];
	int fixMode;
	int version;
	int Reverse;
}ST_FISHEYE_PARAM2, *LP_FISHEYE_PARAM2;

typedef struct byPassFrame{
	int frameType;
	int dataSize;
	void *data;
}ST_BYPASS_FRAME, *LP_BYPASS_FRAME;

typedef struct bindWidth
{
    void *session;
    float bandWidth;
    struct bindWidth *next;
}st_bindWidth, *lp_bindWidth;
lp_bindWidth _headNodeBindWidth = NULL;
static inline uint64_t _get_time_ms()
{
	uint64_t time_ms = 0;
	struct timeval tv;
	gettimeofday(&tv, NULL);
	time_ms = tv.tv_sec;
	time_ms*=1000;
	time_ms += tv.tv_usec/1000;
	return time_ms;
}

int OnAuth(void *ctx, const char *usr, const char *pwd)
{
    printf("Device Auth<%s:%s>\n", usr, pwd);
	
	if(usr || (0 != strlen(usr))){
		if(USRM_GREAT == USRM_check_user(usr, pwd)){
			return 0;
		}
	}
    return -1;
}

int OnPtzCtrl(void *ctx, char *chl, char *act, char* param1, char* param2)
{
    printf("PTZ Ctrl:ch%s|%s\n", chl, act);
    return 0;
}

// FIXME, this callback sounds bad
// App Developer just know the device's stream quality, but the things how to build this xml package
// this will be misundertanding  the things what the sdk want it

int OnSettingsReadStreamInfo(void *ctx, char *vinNo, char *streaminfo, int len)
{
    char info_buf[2048] = {0};
    snprintf(info_buf, sizeof(info_buf),
             "<SOUP version=\"1.0\">"
             "<settings method=\"read\" ticket=\"%u\">"
             "<%s>"
             "<stream0 name=\"720p.264\" size=\"1280x720\" x1=\"yes\" x2=\"yes\" x4=\"yes\" />"
             "<stream1 name=\"360p.264\" size=\"640x360\" x1=\"yes\" x2=\"yes\" x4=\"yes\" />"
             "<stream2 name=\"qvga.264\" size=\"320x240\" x1=\"yes\" x2=\"yes\" x4=\"yes\" />"
             "</%s>"
             "</settings>"
             "</SOUP>",
             123456789, vinNo, vinNo);
	//FIXME below
    if (len >= strlen(info_buf)){
        snprintf(streaminfo, len, "%s", info_buf);
        return 0;
    }
    return -1;
}



struct StreamCtx{
	bool isOldTypeRec;
    int mediabuf_id;
    lpMEDIABUF_USER mediabuf_user;
	int chn;
	int streamNo;
	bool bypass_frame_flag;
	struct P2PDeviceDemo *pDevice;
	char byPassFrameBuf[1024];
#ifdef GSENSOR
    int gsensor_frame_flag;
    unsigned int gsensor_int_frame_count;
    uint64_t gsensor_ts_us;
#endif
};

void* OnAttachStream(void *ctx, int chn, int streamNo)
{
    if (!ctx) {
        printf("DeviceAttachStream  failed:nil hooks_ctx\n");
        return NULL;
    }
	int ch_num = chn;
	int stream_num = streamNo;
    	
    //limited the client use ch0_0.264
    struct P2PDeviceDemo *pDemo = (struct P2PDeviceDemo*)ctx;
    
	struct StreamCtx *pCtx = (struct StreamCtx*)calloc(1,sizeof(struct StreamCtx));    

	if (!pCtx) {
		printf("DeviceAttachStream failed: create StreamCtx\n");
        return NULL;
    }

	char stream_name[64] = {0};
	int stream_num_ex = 0;
	printf("AttachStream:%d-%d\n", ch_num, stream_num);
	stream_num_ex = stream_num;
	if(P2P_REF_STREAM_NUM == stream_num){
		stream_num = 0;
	}
    snprintf(stream_name, sizeof(stream_name), "ch%d_%d.264", ch_num, stream_num);

	printf("Force AttachStream: %s\n", stream_name);
    pCtx->mediabuf_id = MEDIABUF_lookup_byname(stream_name);
    if (pCtx->mediabuf_id < 0) {
        printf("DeviceAttachStream failed:lookup stream %s\n", stream_name);
        free(pCtx);
		pCtx = NULL;
        return NULL;
    }
    pCtx->mediabuf_user = MEDIABUF_attach(pCtx->mediabuf_id);

    if(NULL == pCtx->mediabuf_user) {
        printf("DeviceAttachStream: mediabuf full!\n");
        free(pCtx);
		pCtx = NULL;
        return NULL;
    }

	pCtx->bypass_frame_flag = true;
	pCtx->pDevice = pDemo;
	pCtx->chn = ch_num;
	pCtx->streamNo = stream_num_ex;
#ifdef GSENSOR
    pCtx->gsensor_frame_flag = false;
    pCtx->gsensor_int_frame_count = 0;
    pCtx->gsensor_ts_us = 0;
#endif
	GLOBAL_enter_live();

    return pCtx;
}

static int recover_bps(void * stream_ctx)
{
#if !defined(ADT)        // ç›®å‰åªç”¨äºŽadityaé¡¹ç›®
    return 0;
#endif

	struct StreamCtx *pCtx = (struct StreamCtx*)stream_ctx;
	ST_NSDK_VENC_CH venc_ch;
	//int vin = pCtx->chn;
	//int stream = pCtx->streamNo;
	int vin = 0;
    int stream = 0;
	int *pts_bps = NULL;
	ST_SDK_ENC_STREAM_ATTR stream_attr;

	memset(&stream_attr,0,sizeof(ST_SDK_ENC_STREAM_ATTR));
	memset(&venc_ch, 0, sizeof(ST_NSDK_VENC_CH));
	
	NETSDK_conf_venc_ch_get((vin+1)*100+stream+1, &venc_ch);
	
	SDK_ENC_get_stream(vin, stream, &stream_attr);
	switch(stream_attr.enType){
		default:
		case kSDK_ENC_BUF_DATA_H264:
			pts_bps = &stream_attr.H264_attr.bps;
			break;
		case kSDK_ENC_BUF_DATA_H265:
			pts_bps = &stream_attr.H265_attr.bps;
			break;
	}
	*pts_bps = venc_ch.constantBitRate;
	if(*pts_bps > 128){
		SDK_ENC_set_stream(vin, stream, &stream_attr);
	}
	return 0;
}

int OnDetachStream(void *stream_ctx)
{
    struct StreamCtx *pCtx = (struct StreamCtx*)stream_ctx;
	
	if (pCtx && pCtx->mediabuf_user) {
	printf("file=%s, func=%s\n", __FILE__, __FUNCTION__);
		MEDIABUF_detach(pCtx->mediabuf_user);
		pCtx->mediabuf_user = NULL;

		recover_bps(stream_ctx);
		if(pCtx)
		{
			free(pCtx);
			pCtx = NULL;
		}
		GLOBAL_leave_live();
        return 0;
    } else {
        return -1;
    }
}

void fill_frame_head(unsigned int headType, struct P2PFrameHead *pFrameHead, const stSDK_ENC_BUF_ATTR *attr)
{
    pFrameHead->magic = 0x50325000;/*P2P\0*/
    pFrameHead->version = 0x10000000;
	pFrameHead->headtype = headType;
	pFrameHead->framesize = attr->data_sz;
	
	if (headType == P2P_FRAMEHEAD_HEADTYPE_REPLAY)
	{
		if(attr->type == SDK_ENC_BUF_DATA_PCM || attr->type == SDK_ENC_BUF_DATA_G711A || attr->type == SDK_ENC_BUF_DATA_G711U || attr->type == SDK_ENC_BUF_DATA_AAC){
			pFrameHead->frametype = P2P_FRAMEHEAD_FRAMETYPE_AUDIO;
	    }
		else if(((attr->type == SDK_ENC_BUF_DATA_H264)&&(attr->h264.keyframe == 0)) || ((attr->type == SDK_ENC_BUF_DATA_H265)&&(attr->h265.keyframe == 0))){
	        pFrameHead->frametype = P2P_FRAMEHEAD_FRAMETYPE_PFRAME;
	    }
		else
		{
			 pFrameHead->frametype = P2P_FRAMEHEAD_FRAMETYPE_IFRAME;
		}
	}else{
		if( attr->type == SDK_ENC_BUF_DATA_PCM || attr->type == SDK_ENC_BUF_DATA_G711A || attr->type == SDK_ENC_BUF_DATA_G711U || attr->type == SDK_ENC_BUF_DATA_AAC){
	        pFrameHead->frametype = P2P_FRAMEHEAD_FRAMETYPE_AUDIO;
	    }else if((0 != attr->h264.keyframe) || (0 != attr->h265.keyframe)){
	        pFrameHead->frametype = P2P_FRAMEHEAD_FRAMETYPE_IFRAME;
	    }else{
	        pFrameHead->frametype = P2P_FRAMEHEAD_FRAMETYPE_PFRAME;
	    }
	}

	if(headType == P2P_FRAMEHEAD_HEADTYPE_REPLAY)
	{
    	pFrameHead->ts_ms = attr->timestamp_us;
	}
	else
	{
		//fix me:need to get from media buffer
		if(pFrameHead->frametype == P2P_FRAMEHEAD_FRAMETYPE_AUDIO){
			//AUDIO
			pFrameHead->ts_ms = attr->timestamp_us/1000;
		}else{
			//VIDEO
			pFrameHead->ts_ms = _get_time_ms();//attr->time_us / 1000;
		}
	}
	
    if(P2P_FRAMEHEAD_FRAMETYPE_IFRAME == pFrameHead->frametype || P2P_FRAMEHEAD_FRAMETYPE_PFRAME == pFrameHead->frametype){
		if(attr->type == SDK_ENC_BUF_DATA_H264){
			pFrameHead->v.width = attr->h264.width;
	        pFrameHead->v.height = attr->h264.height;
	        memcpy(&pFrameHead->v.enc,"H264",strlen("H264"));
	        pFrameHead->v.fps = attr->h264.fps;
		}else{//if(attr->type == SDK_ENC_BUF_DATA_H265){
			pFrameHead->v.width = attr->h265.width;
	        pFrameHead->v.height = attr->h265.height;
	        memcpy(&pFrameHead->v.enc,"H265",strlen("H265"));
	        pFrameHead->v.fps = attr->h265.fps;
		}
    }else{
        pFrameHead->a.samplerate = attr->g711a.sample_rate;
        pFrameHead->a.samplewidth = attr->g711a.sample_width;
        pFrameHead->a.channels = 1;
        pFrameHead->a.compress = attr->g711a.compression_ratio;
		if(attr->type == SDK_ENC_BUF_DATA_G711A){
        	snprintf(pFrameHead->a.enc, sizeof(pFrameHead->a.enc), "%s", "G711");
		}else if(attr->type == SDK_ENC_BUF_DATA_AAC){
			snprintf(pFrameHead->a.enc, sizeof(pFrameHead->a.enc), "%s", "AAC");
		}
    }
    
    //sth else for record stream
    if (headType == P2P_FRAMEHEAD_HEADTYPE_REPLAY) {
        pFrameHead->rectype = 8;
        pFrameHead->recchn = 0;
    }
}

int setBypassFrame(struct P2PFrameHead *pFrameHead, void *raw_frame)
{
	ST_BYPASS_FRAME byPassFrame;
	ST_FISHEYE_PARAM fisheye;
	stFISHEYE_config fisheye_config;

	pFrameHead->magic = P2P_FRAMEHEAD_MAGIC;
    pFrameHead->version = P2P_FRAMEHEAD_VERSION;
	pFrameHead->headtype = P2P_FRAMEHEAD_HEADTYPE_LIVE;
	pFrameHead->frametype = P2P_FRAMEHEAD_FRAMETYPE_OOB;

	//for fisheye correction
	pFrameHead->framesize = sizeof(ST_FISHEYE_PARAM) + sizeof(ST_BYPASS_FRAME) - sizeof(void *);
	byPassFrame.frameType = P2P_BYPASS_FRAME_TYPE_FISHEYE_PARAM;
	byPassFrame.dataSize = sizeof(ST_FISHEYE_PARAM);

    /* ¸ù¾Ýproduct typeÈ·¶¨ÊÇ·ñÐèÒªÉèÖÃfisheye²ÎÊý */
    /* Ã»ÓÐ¶Áµ½product type£¬ÔòÉèÖÃfisheye²ÎÊý */
    FISHEYE_config_get(&fisheye_config);
	memcpy(fisheye.param, fisheye_config.param, sizeof(fisheye_config.param));

	fisheye.fixMode = FISHEYE_get_fix_mode();
	fisheye.version = 0x01000003;//1.0.0.3
	fisheye.Reverse = 0;

	//copy to p2p buffer
	memcpy(raw_frame, &byPassFrame, P2P_STRUCT_MEMBER_POS(ST_BYPASS_FRAME, data));
	memcpy(raw_frame + P2P_STRUCT_MEMBER_POS(ST_BYPASS_FRAME, data), &fisheye, sizeof(ST_FISHEYE_PARAM));

	return 0;
}

int setBypassFrameEx(void *raw_frame)
{
	ST_BYPASS_FRAME byPassFrame;
	ST_FISHEYE_PARAM fisheye;
	stFISHEYE_config fisheye_config;
	ST_SoupFrameHead pFrameHead;

	pFrameHead.magic = 0x534f55ff;
	pFrameHead.version = 0x01000000;
	pFrameHead.frametype = 15;//OOB
	pFrameHead.ts_sec = 0;
	pFrameHead.ts_usec = 0;
	pFrameHead.externsize = 0;

	//for fisheye correction
	pFrameHead.framesize = sizeof(ST_FISHEYE_PARAM) + sizeof(ST_BYPASS_FRAME) - sizeof(void *);
	byPassFrame.frameType = P2P_BYPASS_FRAME_TYPE_FISHEYE_PARAM;
	byPassFrame.dataSize = sizeof(ST_FISHEYE_PARAM);
	FISHEYE_config_get(&fisheye_config);
	memcpy(fisheye.param, fisheye_config.param, sizeof(fisheye_config.param));

	fisheye.fixMode = FISHEYE_get_fix_mode();
	fisheye.version = 0x01000003;//1.0.0.3
	fisheye.Reverse = 0;

	//copy to p2p buffer
	memcpy(raw_frame, &pFrameHead, sizeof(ST_SoupFrameHead));
	memcpy(raw_frame+sizeof(ST_SoupFrameHead), &byPassFrame, P2P_STRUCT_MEMBER_POS(ST_BYPASS_FRAME, data));
	memcpy(raw_frame+sizeof(ST_SoupFrameHead) + P2P_STRUCT_MEMBER_POS(ST_BYPASS_FRAME, data), &fisheye, sizeof(ST_FISHEYE_PARAM));

	return pFrameHead.framesize + sizeof(ST_SoupFrameHead);
}

/* Ö÷Òª´¦ÀíPx-720µÄÔ²ÐÄÐ£Õý²ÎÊýºÍ¾µÍ·»û±ä²ÎÊý */
int setBypassFrame2Ex(void *raw_frame)
{
	ST_BYPASS_FRAME byPassFrame;
	ST_FISHEYE_PARAM2 fisheye;
	stFISHEYE_config fisheye_config;
	ST_SoupFrameHead pFrameHead;
	LP_FISHEYE_LENS_PARAM fisheye_lens_param = NULL;
	int lens_param_len = 0;

	pFrameHead.magic = 0x534f55ff;
	pFrameHead.version = 0x01000000;
	pFrameHead.frametype = 15;//OOB
	pFrameHead.ts_sec = 0;
	pFrameHead.ts_usec = 0;
	pFrameHead.externsize = 0;

	//for fisheye correction
	byPassFrame.frameType = P2P_BYPASS_FRAME_TYPE_LENS_PARAM;
	lens_param_len = FISHEYE_lens_param_len_get();
	FISHEYE_config_get(&fisheye_config);
	memcpy(fisheye.param, fisheye_config.param2, sizeof(fisheye_config.param2));

	fisheye.fixMode = FISHEYE_get_fix_mode();
	fisheye.version = 0x01000003;//1.0.0.3
	fisheye.Reverse = 0;

	fisheye_lens_param = FISHEYE_lens_param_get();

	if(fisheye_lens_param != NULL) {
		byPassFrame.dataSize = sizeof(ST_FISHEYE_PARAM2) + lens_param_len * sizeof(ST_FISHEYE_LENS_PARAM);
		pFrameHead.framesize = byPassFrame.dataSize + sizeof(ST_BYPASS_FRAME) - sizeof(void *);
	}
	else {
		byPassFrame.dataSize = sizeof(ST_FISHEYE_PARAM2);
		pFrameHead.framesize = byPassFrame.dataSize + sizeof(ST_BYPASS_FRAME) - sizeof(void *);
	}
	//copy to p2p buffer
	memcpy(raw_frame, &pFrameHead, sizeof(ST_SoupFrameHead));
	memcpy(raw_frame + sizeof(ST_SoupFrameHead), &byPassFrame, P2P_STRUCT_MEMBER_POS(ST_BYPASS_FRAME, data));
	memcpy(raw_frame + sizeof(ST_SoupFrameHead) + P2P_STRUCT_MEMBER_POS(ST_BYPASS_FRAME, data), &fisheye, sizeof(ST_FISHEYE_PARAM2));
	if(fisheye_lens_param != NULL) {
		memcpy(raw_frame + sizeof(ST_SoupFrameHead) + P2P_STRUCT_MEMBER_POS(ST_BYPASS_FRAME, data) + sizeof(ST_FISHEYE_PARAM2), fisheye_lens_param, lens_param_len * sizeof(ST_FISHEYE_LENS_PARAM));
	}

    return pFrameHead.framesize + sizeof(ST_SoupFrameHead);
}

int setGsensorFrame(void *raw_frame, uint64_t ts_us)
{
#ifdef GSENSOR
    ST_GSENSOR_FRAME gsensorFrame;
    ST_GSENSOR_ANGLES gsensorAngles;
    stGsensor_angles angles;
    ST_SoupFrameHead pFrameHead;

    pFrameHead.magic = 0x534f55ff;
    pFrameHead.version = 0x01000000;
    pFrameHead.frametype = P2P_FRAMEHEAD_FRAMETYPE_OOB;//OOB
    pFrameHead.ts_sec = ts_us/1000000;
    pFrameHead.ts_usec = ts_us%1000000;
    pFrameHead.externsize = 0;

    //for gsensor correction
    if(0 != APP_GSENSOR_get_angles_frame(raw_frame + sizeof(ST_SoupFrameHead), &pFrameHead.framesize)) {
        return -1;
    }


    //copy to p2p buffer
    memcpy(raw_frame, &pFrameHead, sizeof(ST_SoupFrameHead));

    return pFrameHead.framesize + sizeof(ST_SoupFrameHead);
#endif
    return 0;
}

int getRefFrame(int stream_num, lpMEDIABUF_USER mediabuf_user, struct P2PFrameHead *pFrameHead, void *raw_frame, size_t raw_frame_max_sz)
{
	size_t out_size = 0;
	size_t	data_sz = 0;
	const stSDK_ENC_BUF_ATTR *attr = NULL;
	if(0 == MEDIABUF_out(mediabuf_user, (void**)&attr, NULL, &out_size)) {
		if(out_size > raw_frame_max_sz){
			printf("thiz->m_snd_buf_sz too small:%d-%d\n", out_size, raw_frame_max_sz);
			return -1;
		}
		if(P2P_REF_STREAM_NUM == stream_num && 1 == attr->h264.ref_counter){
			data_sz = getRefFrame(stream_num, mediabuf_user, pFrameHead, raw_frame, raw_frame_max_sz);
		}else{
			const void* raw_ptr = (void*)(attr + 1);
			data_sz = attr->data_sz;
			memcpy(raw_frame, raw_ptr, data_sz);
			/*here pack the soup head*/
			fill_frame_head(P2P_FRAMEHEAD_HEADTYPE_LIVE, pFrameHead, attr);
		}
		return data_sz;
	}
	else
	{	 
		//printf("mediabuf out err! %u\n", pMEDIABUF->In_speed(pCtx->mediabuf_id));
		msleep(10);
		return	0;
	}

}


int getRefFrameEx(struct StreamCtx *pCtx, lpMEDIABUF_USER mediabuf_user, void **ppRawFrame)
{
	size_t out_size = 0;
	size_t	data_sz = 0;
	const stSDK_ENC_BUF_ATTR *attr = NULL;
	if(0 == MEDIABUF_out(mediabuf_user, (void**)&attr, NULL, &out_size)) {
		if(P2P_REF_STREAM_NUM == pCtx->streamNo && 1 == attr->h264.ref_counter){
			data_sz = getRefFrameEx(pCtx, mediabuf_user, ppRawFrame);
		}else{
            data_sz = attr->data_sz + sizeof(ST_SoupFrameHead);
            *ppRawFrame = (void*)&(attr->soupFrameHead);
#ifdef GSENSOR
            if(APP_GSENSOR_is_support()) {
                if((attr->soupFrameHead.frametype == 1) || (attr->soupFrameHead.frametype == 2)) {
                    if(pCtx->gsensor_frame_flag == false) {
                        pCtx->gsensor_frame_flag = true;
                        pCtx->gsensor_int_frame_count = 0;
                    }
                    else {
                        pCtx->gsensor_int_frame_count++;
                        if(pCtx->gsensor_int_frame_count >= GSENSOR_INT_FRAME_COUNT) {
                            pCtx->gsensor_ts_us = attr->time_us; // èŽ·å–è¿™ä¸€æ¬¡çš„è§†é¢‘å¸§æ—¶é—´æˆ³,ä¸‹ä¸€å¸§å‘é€Gsensor oobä½¿ç”¨è¯¥æ—¶é—´æˆ³
                        }
                    }
                }
            }
#endif
		}
		return data_sz;
	}
	else
	{
		//printf("mediabuf out err! %u\n", pMEDIABUF->In_speed(pCtx->mediabuf_id));
		return	0;
	}

}


ssize_t OnReadFrame(void *stream_ctx, struct P2PFrameHead *pFrameHead, void *raw_frame, size_t raw_frame_max_sz)
{
    struct StreamCtx *pCtx = (struct StreamCtx*)stream_ctx;
	if (!pCtx) {
		printf("DeviceReadFrame failed:nil ctx/pMEDIABUF\n");
		return -1;
	}
    lpMEDIABUF_USER  mediabuf_user = pCtx->mediabuf_user;

	//send bypass frame
	if(NK_False == GLOBAL_sn_front()){
		pCtx->bypass_frame_flag = false;
	}
	
	if(pCtx->bypass_frame_flag){
#if defined(PX_720)
		//setBypassFrame2(pFrameHead, raw_frame);
#else
		setBypassFrame(pFrameHead, raw_frame);
#endif
		pCtx->bypass_frame_flag = false;
		return pFrameHead->framesize;
	}
	int  data_sz = 0;
	if(NULL != mediabuf_user)
	{	
	    if(0 == MEDIABUF_out_lock(mediabuf_user)){
	        //const SDK_ENC_BUF_ATTR_t *attr = NULL;
			data_sz = getRefFrame(pCtx->streamNo, mediabuf_user, pFrameHead, raw_frame, raw_frame_max_sz);
			MEDIABUF_out_unlock(mediabuf_user);
			return data_sz;
	    }
		else
		{
			msleep(10);
			printf("DeviceReadFrame failed: mediabuf lock err, user:%p\n", mediabuf_user);
			return -1;
		}
	}
	else
	{
		msleep(10);	
		printf("DeviceReadFrame failed: mediabuf_user:null\n");
		return -1;
	}

}


ssize_t OnReadFrameEx(void *stream_ctx, void **ppRawFrame)
{
    struct StreamCtx *pCtx = (struct StreamCtx*)stream_ctx;
	if (!pCtx) {
		printf("DeviceReadFrame failed:nil ctx/pMEDIABUF\n");
		return -1;
	}
    lpMEDIABUF_USER  mediabuf_user = pCtx->mediabuf_user;

    if(NK_False == GLOBAL_sn_front()) {
        pCtx->bypass_frame_flag = false;
    }

	//send bypass frame
	if(pCtx->bypass_frame_flag){
		int ret_size = 0;
#if defined(PX_720)
		ret_size= setBypassFrame2Ex(pCtx->byPassFrameBuf);
#else
		ret_size= setBypassFrameEx(pCtx->byPassFrameBuf);
#endif

		*ppRawFrame = pCtx->byPassFrameBuf;
		pCtx->bypass_frame_flag = false;
		return ret_size;
	}
#ifdef GSENSOR
    else if(pCtx->gsensor_frame_flag && (pCtx->gsensor_int_frame_count >= GSENSOR_INT_FRAME_COUNT)) {
        int ret_size = 0;
        ret_size= setGsensorFrame(pCtx->byPassFrameBuf, pCtx->gsensor_ts_us);
        *ppRawFrame = pCtx->byPassFrameBuf;
        pCtx->gsensor_frame_flag = false;
        pCtx->gsensor_int_frame_count = 0;
        return ret_size;
    }
#endif
	int  data_sz = 0;
	if(NULL != mediabuf_user)
	{
	    if(0 == MEDIABUF_out_lock(mediabuf_user)){
	        //const SDK_ENC_BUF_ATTR_t *attr = NULL;
			data_sz = getRefFrameEx(pCtx, mediabuf_user, ppRawFrame);
			//MEDIABUF_out_unlock(mediabuf_user);
			return data_sz;
	    }
		else
		{
			printf("DeviceReadFrame failed: mediabuf lock err, user:%p\n", mediabuf_user);
			return -1;
		}
	}
	else
	{
		printf("DeviceReadFrame failed: mediabuf_user:null\n");
		return -1;
	}

}

void OnreleaseMediaBufFrame(void *stream_ctx, void *pRawFrame)
{
	struct StreamCtx *pCtx = (struct StreamCtx*)stream_ctx;
	if (!pCtx && NULL != pCtx->mediabuf_user) {
		printf("DeviceReadFrame failed:nil ctx/pMEDIABUF\n");
		return;
	}
	MEDIABUF_out_unlock(pCtx->mediabuf_user);
}


int OnDevOnline(void *ctx, const char *id)
{
	if (!ctx || !id) {
		printf("OnDeviceOnline err:nil hooks_ctx/id\n");
		return -1;
	}
	struct P2PDeviceDemo* pDemo = (struct P2PDeviceDemo*)ctx;
	snprintf(pDemo->eseeId, sizeof(pDemo->eseeId), "%s", id);
	if (pDemo->OnDevOnline)
		pDemo->OnDevOnline (id, pDemo->ctx);
	return 0;
}

int OnDevConnectReq(void *ctx, int type, int action, unsigned int ip, unsigned short port, unsigned int random)
{
	printf("P2P OnDevConnectReq type = %d, action =%d, ip=%d, port =%d, random=%d\n", type, action, ip, port, random);
}

int OnFetchRecList(void *ctx, int chnCnt, char chn[], int types, time_t startTime, time_t endTime, int quality, RecList *pLists)
{
#if defined(TFCARD)
	stTFCARD_History_List historyList[100] = {0};
	NK_Int historyCnt = sizeof(historyList) / sizeof(historyList[0]);
	int i = 0;
	if(pLists == NULL) {
		return -1;
	}
	int start_index = pLists->recordIdx;
	//char type[10] ={0};
	//snprintf(type, sizeof(type), "%d", types);

	ST_NSDK_SYSTEM_TIME systime = {0};
	NETSDK_conf_system_get_time(&systime);
	int timezone_second = ((abs(systime.greenwichMeanTime) / 100) * (3600)
							+(abs(systime.greenwichMeanTime) % 100) * 60)
							* (systime.greenwichMeanTime > 0 ? 1 : -1);
	printf("timezone_second = %d\n", timezone_second);
	printf("systime.greenwichMeanTime = %d\n", systime.greenwichMeanTime);
	printf("startTime - timezone_second = %d\n", startTime - timezone_second);
	printf("endTime - timezone_second = %d\n", endTime - timezone_second);

	/**
	 * ´Ë´¦µ÷ÓÃ»ñÈ¡Â¼ÏñÁÐ±íÊ±,typeÉèÎª¿Õ,Ä¬ÈÏÌáÈ¡ËùÓÐÂ¼ÏñÀàÐÍ
	 */
	if(0 == NK_TFCARD_get_history(startTime - timezone_second, endTime - timezone_second,
								  NULL, historyList, start_index, &historyCnt))
	{
		//APP_TRACE("start_index : %d, historyCnt : %d",start_index,historyCnt);
		for (i = 0; i < pLists->maxRecord && i < historyCnt; ++i)
		{
			pLists->pRecord[i].chn = 0;
			pLists->pRecord[i].type = types;
			pLists->pRecord[i].startTime = historyList[i].beginTm + timezone_second;//+8*60*60;
			pLists->pRecord[i].endTime = historyList[i].endTm + timezone_second;//+8*60*60;
			pLists->pRecord[i].quality = quality;
		}
		pLists->recordCnt = i;
		pLists->recordTotal = historyCnt;
		APP_TRACE("__FUNCTION__:%s, LINE:%d, pLists->recordIdx:%d, pLists->listCnt:%d, pLists->maxRecord:%d, history_count:%d\n",
			__FUNCTION__, __LINE__, pLists->recordIdx, pLists->recordCnt, pLists->maxRecord, historyCnt);
		
		return 0;
	}else{
		APP_TRACE("p2p playback VIDEO history is can't search!!!");
	}
#endif
	return -1;
}


void* OnOpenRecFiles(void *ctx, int chnCnt, char chn[], int recType, time_t startTime, time_t endTime, int quality)
{
#if defined(TFCARD)
#if 0
	// play start 
	printf("__FUNCTION__:%s, LINE:%d, startTime:%d\n", __FUNCTION__, __LINE__, startTime);
	Recctx_f *recframe = (Recctx_f *)calloc(1,sizeof(Recctx_f));
	char type[10] ={0};
	int play_ret = -1;
	ST_NSDK_SYSTEM_TIME systime;
	NETSDK_conf_system_get_time(&systime);
	int timezone_second = ((abs(systime.greenwichMeanTime) / 100) * (3600)
							+(abs(systime.greenwichMeanTime) % 100) * 60)
							* (systime.greenwichMeanTime > 0 ? 1 : -1);
	printf("timezone_second = %d\n", timezone_second);
	printf("systime.greenwichMeanTime = %d\n", systime.greenwichMeanTime);

	recframe->rec_attr = (stSDK_ENC_BUF_ATTR*)calloc(1,sizeof(stSDK_ENC_BUF_ATTR));
	recframe->raw_buff = calloc(1, SIZE_BUFF);

	recframe->frame_maxlen = SIZE_BUFF;
	recframe->flag = 0;
		
	snprintf(type, sizeof(type), "%d", recType);
	play_ret = g_pTFer->play(g_pTFer, TRUE, type, startTime-timezone_second, (void*)recframe);

	//printf("__FUNCTION__:%s, LINE:%d, play_ret:%d\n", __FUNCTION__, __LINE__, play_ret);
	recframe->recType = recType;
	recframe->utc_t = startTime-timezone_second;

	return (void*)recframe;
#else
	/**
	 * µ±TF¿¨ÎÞ¹ÒÔØÊ±,ÎÞ·¨½øÐÐÂ¼Ïñ»Ø·Å
	 */
	if(!NK_TFCARD_is_mounted()){
		return NULL;
	}
	/**
	 * ÉÏÏÂÎÄ¶¨ÒåÒ»¸öÕûÐÎ±äÁ¿Ö¸Õë¾ä±ú,ÔÝÊ±²»¿¼ÂÇ¶àÈËÍ¬Ê±»Ø·Å
	 */
	struct StreamCtx *recCtx = (struct StreamCtx*)calloc(1,sizeof(struct StreamCtx));  
	ST_NSDK_SYSTEM_TIME systime;
	NETSDK_conf_system_get_time(&systime);
	NK_Int timezone_second = ((abs(systime.greenwichMeanTime) / 100) * (3600)
							+(abs(systime.greenwichMeanTime) % 100) * 60)
							* (systime.greenwichMeanTime > 0 ? 1 : -1);
	printf("timezone_second = %d\n", timezone_second);
	printf("systime.greenwichMeanTime = %d\n", systime.greenwichMeanTime);

	recCtx->isOldTypeRec = true;
	recCtx->bypass_frame_flag = false;//remove bypass frame in playback in case of avoiding phone app crashing
	if(0 != NK_TFCARD_play_start(startTime-timezone_second,NULL)){
		if(recCtx){
			free(recCtx);
		}
		return NULL;
	}
	else{
		return (void *)recCtx;
	}
#endif
#endif
	return NULL;
}

int OnCloseRecFiles(void *ctx)
{

#if defined(TFCARD)
#if 0

	// play stop
	Recctx_f *my_frame = (Recctx_f *)ctx;
	char type[10] ={0};
	snprintf(type, sizeof(type), "%d", my_frame->recType);

	if(0 == g_pTFer->play(g_pTFer, FALSE, type, my_frame->utc_t, NULL))
	{
		if(my_frame->rec_attr)
		{
			free(my_frame->rec_attr);
			my_frame->rec_attr = NULL;
		}
		if(my_frame->raw_buff)
		{
			free(my_frame->raw_buff);
			my_frame->raw_buff = NULL;
		}
		if(my_frame)
		{
			free(my_frame);
			my_frame = NULL;
		}
		return 0;
	}
#else
	struct StreamCtx *recCtx = (struct StreamCtx *)ctx;

	if(recCtx->isOldTypeRec == true) {
		if(recCtx){
			free(recCtx);
		}
		NK_TFCARD_play_stop();

		return 0;
	}
	else {
		return -1;
	}
#endif
#endif
	return -1;
}

ssize_t OnReadRecFrame(void *ctx, P2PFrameHead* pFrameHead, void *raw_frame, size_t raw_frame_max_sz)
{

#if defined(TFCARD)
#if 0
	if(NULL == ctx || NULL == g_pTFer)
	{
		usleep(20000);
		return -1;
	}
	Recctx_f *my_frame = (Recctx_f *)ctx;
	if(my_frame->flag == -1)
	{
		printf("__FUNCTION__:%s, LINE:%d, ctx=%p, flag:%d\n", __FUNCTION__, __LINE__, ctx, my_frame->flag);
		return -1;
	}
	else if(my_frame->flag == 1)
	{
		unsigned int data_len = 0;
		if(my_frame && my_frame->raw_buff)
		{
			data_len =  my_frame->rec_attr->data_sz;

			if(data_len > raw_frame_max_sz)
			{
				printf("raw_frame_max_sz = %d", data_len);
				my_frame->flag = 0;
				usleep(20000);
				return -1;
			}
			memcpy(raw_frame, my_frame->raw_buff, data_len);
			fill_frame_head(P2P_FRAMEHEAD_HEADTYPE_REPLAY, pFrameHead, my_frame->rec_attr);
		}
		my_frame->flag = 0;
		usleep(1000);
		return data_len;
	}
#else
	stRecord_Frame_Head frameHead = {0};
	NK_Int dataSize = 0;

	/**
	 * µ±TF¿¨ÎÞ¹ÒÔØÊ±,ÎÞ·¨½øÐÐÂ¼Ïñ»Ø·Å
	 */
	if(!NK_TFCARD_is_mounted()){
		APP_TRACE("TFcard not mounted!");
		return -1;
	}

	struct StreamCtx *pCtx = (struct StreamCtx*)ctx;
	if (!pCtx) {
		printf("DeviceReadFrame failed:nil ctx/pMEDIABUF\n");
		return -1;
	}
	if(pCtx->isOldTypeRec == false) {
		return -1;
	}
	//send bypass frame
	if(NK_False == GLOBAL_sn_front()){
		pCtx->bypass_frame_flag = false;
	}
	
	if(pCtx->bypass_frame_flag){
#if defined(PX_720)
		//setBypassFrame2(pFrameHead, raw_frame);
#else
		setBypassFrame(pFrameHead, raw_frame);
#endif
		pCtx->bypass_frame_flag = false;
		return pFrameHead->framesize;
	}

	dataSize = NK_TFCARD_play_read_frame(&frameHead,(NK_PByte)raw_frame, raw_frame_max_sz);
	if(dataSize <= 0){
		APP_TRACE("read frame error : dataSize = %d, raw_frame_max_sz = %d",dataSize,raw_frame_max_sz);
		return -1;
	}

	pFrameHead->magic = 0x50325000;
	pFrameHead->version = 0x01000000;
	pFrameHead->headtype = 1;//Êý¾ÝÀàÐÍ 0:Ö±²¥ 1:Â¼Ïñ
	pFrameHead->framesize = frameHead.dataSize;
	pFrameHead->ts_ms = frameHead.sysTime_ms;

	//²âÊÔÓÃ,µÈÎÈ¶¨ºóÔÙÉ¾³ý.
/*	if(frameHead.codec == 96 || frameHead.codec == 97){
	printf("--------message in frame head------- \n\
codec : %s\n\
coderStamp_ms : %lld\n\
sysTime_ms : %lld\n\
dataSize : %u\n\
headSize : %d\n\
headArr : %p\n",
frameHead.codec==96?"264":frameHead.codec==97?"265":"audio",
frameHead.coderStamp_ms,
frameHead.sysTime_ms,
frameHead.dataSize,
sizeof(stRecord_Frame_Head),
&frameHead);
	}
*/
	if(frameHead.codec == NK_TFCARD_VCODEC_H264){
		snprintf((char *)pFrameHead->v.enc, sizeof(pFrameHead->v.enc), "%s", "H264");
		pFrameHead->v.fps = frameHead.fps;
		pFrameHead->v.width = frameHead.width;
		pFrameHead->v.height = frameHead.height;
		pFrameHead->frametype = (frameHead.isKeyFrame ? 1 : 2);
	}else if(frameHead.codec == NK_TFCARD_VCODEC_H265){
		snprintf((char *)pFrameHead->v.enc, sizeof(pFrameHead->v.enc), "%s", "H265");
		pFrameHead->v.fps = frameHead.fps;
		pFrameHead->v.width = frameHead.width;
		pFrameHead->v.height = frameHead.height;
		pFrameHead->frametype = (frameHead.isKeyFrame ? 1 : 2);
	}else if(frameHead.codec == NK_TFCARD_ACODEC_AAC){
		snprintf((char *)pFrameHead->a.enc, sizeof(pFrameHead->a.enc), "%s", "AAC");
		pFrameHead->a.samplerate = frameHead.sampleRate;
		pFrameHead->a.samplewidth = frameHead.sampleWidth;
		pFrameHead->a.compress = frameHead.compressionRatio;
		pFrameHead->a.channels = 1;
		pFrameHead->frametype = 0;
	}else if(frameHead.codec == NK_TFCARD_ACODEC_G711A){
		snprintf((char *)pFrameHead->a.enc, sizeof(pFrameHead->a.enc), "%s", "G711A");
		pFrameHead->a.samplerate = frameHead.sampleRate;
		pFrameHead->a.samplewidth = frameHead.sampleWidth;
		pFrameHead->a.compress = frameHead.compressionRatio;
		pFrameHead->a.channels = 1;
		pFrameHead->frametype = 0;
	}else{
		APP_TRACE("unknow frameHead.codec = %d!", frameHead.codec);
		dataSize = 0;
	}
	pFrameHead->rectype = P2P_REC_TYPE_ALL;//8;
	pFrameHead->recchn = 0;

	//²âÊÔÓÃ,µÈÎÈ¶¨ºóÔÙÉ¾³ý.
/*	if(frameHead.codec == 96 || frameHead.codec == 97){
		printf("2--------message in frame head------- \n\
codec : %s\n\
dataSize : %u\n",
pFrameHead->v.enc,
pFrameHead->framesize);
	}
*/
	return dataSize;
#endif
#endif
	return 0;	
}

int OnFetchRecListNew(void *ctx, int chnCnt, char chn[], int types, time_t startTime, time_t endTime, int quality, RecList *pLists)
{
	APP_TRACE("on fetch rec list... (startTime: %ld, endTime: %ld, types %x)", startTime, endTime, types);
#if defined(TFCARD)
    int TotalRecs;
    int i;
    int startIndex;
    size_t recListSize;

    if(pLists == NULL) {
        APP_TRACE("pLists can't be NULL!");
        return -1;
    }

    if(pLists->maxRecord <= 0) {
        APP_TRACE("pLists->maxRecord should > 0!");
        return -1;
    }

    recListSize = 100;
    stTFCARD_History_List historyList[recListSize];

    startIndex = pLists->recordIdx;

    TotalRecs = REC_PLAY_get_history(startTime, endTime,
                                     types, startIndex,
                                     historyList, &recListSize);
    if (TotalRecs <= 0) {
		/* ¼ìË÷¾É¸ñÊ½Â¼Ïñ */
		if(GLOBAL_isOldTypeRecord()) {
			return OnFetchRecList(ctx, chnCnt, chn, types, startTime, endTime, quality, pLists);
		}

        APP_TRACE("Failed to search record");
        pLists->recordCnt = 0;
        pLists->recordTotal = 0;
        return -1;
    }
//    APP_TRACE("TotalRecs: %d, recListSize: %lu", TotalRecs, recListSize);

	pLists->recordCnt = 0;
    for (i = 0; i < pLists->maxRecord && i < recListSize; i++) {
        pLists->pRecord[i].chn = 0;
		if(historyList[i].recordType[0] == 'T'){
			pLists->pRecord[i].type = NK_REC_TIMER;
		}
		else if(historyList[i].recordType[0] == 'M'){
			pLists->pRecord[i].type = NK_REC_MOTION;
		}
		else{
			pLists->pRecord[i].type = NK_REC_TIMER;
		}
        pLists->pRecord[i].startTime = historyList[i].beginTm;
        pLists->pRecord[i].endTime = historyList[i].endTm;
        pLists->pRecord[i].quality = quality;
        pLists->recordCnt ++;
    }
    pLists->recordTotal = TotalRecs;

	APP_TRACE("on fetch rec list done, return rec cnt: %d, total: %d, start at index: %d",
			  pLists->recordCnt, pLists->recordTotal, pLists->recordIdx);
    return 0;

#else
    APP_TRACE("firmware do not support tf card!");
    if(pLists == NULL) {
        APP_TRACE("pLists can't be NULL!");
        return -1;
    }
    pLists->recordCnt = 0;
    pLists->recordTotal = 0;
    return -1;
#endif
}

void* OnOpenRecFile2(void *ctx, int chnCnt, char chn[], int recType, time_t startTime, time_t endTime, int quality, int open_type)
{
	void *ret = NULL;

	APP_TRACE("on open rec... (startTime: %ld, endTime: %ld)", startTime, endTime);

#if defined(TFCARD)

	if(GLOBAL_enter_playback() == 0){
		ret =  REC_PLAY_start(startTime, endTime, recType, open_type);
		if (NULL != ret) {
			APP_TRACE("on open rec done (rec ctx: %p)", ret);
		} else {
			if(GLOBAL_isOldTypeRecord()) {
				ret = OnOpenRecFiles(ctx, chnCnt, chn, recType, startTime, endTime, quality);
			}
			if(NULL == ret) {
				GLOBAL_leave_playback();
				APP_TRACE("open rec failed");
			}
		}
	}
	else{
		APP_TRACE("open rec failed, user is full");
	}

	return ret;
#else
	APP_TRACE("firmware do not support tf card!");
    return NULL;
#endif
}

int OnCloseRecFilesNew(void *ctx)
{
	int ret = -1;

    APP_TRACE("on close rec... (rec ctx: %p)", ctx);

#if defined(TFCARD)
	if(!ctx){
		return -1;
	}
	GLOBAL_leave_playback();

    ret = REC_PLAY_stop(ctx);
    if (0 == ret) {
        APP_TRACE("on close rec done (rec ctx: %p)", ctx);
    } else {
		if(GLOBAL_isOldTypeRecord()) {
			if(0 == OnCloseRecFiles(ctx)) {
				APP_TRACE("on close old rec done (rec ctx: %p)", ctx);
				return 0;
			}
		}
		APP_TRACE("close rec failed");
    }

	return ret;
#else
    APP_TRACE("firmware do not support tf card!");
    return -1;
#endif
}

ssize_t OnReadRecFrameNew(void *ctx, P2PFrameHead* pFrameHead, void *raw_frame, size_t raw_frame_max_sz)
{
//	APP_TRACE("on read frame... (rec ctx: %p)", ctx);
#if defined(TFCARD)
    ssize_t ret, dataSize = 0;
    stRecord_Frame_Head frameHead;

	if(!ctx){
		APP_TRACE("on read frame, ctx is null !");
		return -1;
	}

    ret = REC_PLAY_read_frame(ctx, &frameHead, (uint8_t *)raw_frame, raw_frame_max_sz);
    if (ret < 0) {
		if(GLOBAL_isOldTypeRecord()) {
			return OnReadRecFrame(ctx, pFrameHead, raw_frame, raw_frame_max_sz);
		}
		APP_TRACE("on read frame, failed to read frame, ret: %ld (rec ctx: %p)", ret, ctx);
        return -1;
    }

    pFrameHead->magic = 0x50325000;
    pFrameHead->version = 0x01000000;
    pFrameHead->headtype = 1;//Êý¾ÝÀàÐÍ 0:Ö±²¥ 1:Â¼Ïñ
    pFrameHead->framesize = frameHead.dataSize;
    pFrameHead->ts_ms = frameHead.sysTime_ms;
    dataSize = frameHead.dataSize;
//    APP_TRACE("pFrameHead->ts_ms: %llu", pFrameHead->ts_ms);

    if(frameHead.codec == NK_TFCARD_VCODEC_H264){
        snprintf((char *)pFrameHead->v.enc, sizeof(pFrameHead->v.enc), "%s", "H264");
        pFrameHead->v.fps = frameHead.fps;
        pFrameHead->v.width = frameHead.width;
        pFrameHead->v.height = frameHead.height;
        pFrameHead->frametype = (frameHead.isKeyFrame ? 1 : 2);
    }else if(frameHead.codec == NK_TFCARD_VCODEC_H265){
        snprintf((char *)pFrameHead->v.enc, sizeof(pFrameHead->v.enc), "%s", "H265");
        pFrameHead->v.fps = frameHead.fps;
        pFrameHead->v.width = frameHead.width;
        pFrameHead->v.height = frameHead.height;
        pFrameHead->frametype = (frameHead.isKeyFrame ? 1 : 2);
    }else if(frameHead.codec == NK_TFCARD_ACODEC_G711A){
        snprintf((char *)pFrameHead->a.enc, sizeof(pFrameHead->a.enc), "%s", "G711A");
        pFrameHead->a.samplerate = frameHead.sampleRate;
        pFrameHead->a.samplewidth = frameHead.sampleWidth;
//        pFrameHead->a.compress = frameHead.compressionRatio;
        pFrameHead->a.channels = 1;
        pFrameHead->frametype = 0;
    }else if(frameHead.codec == NK_TFCARD_ACODEC_AAC){
        snprintf((char *)pFrameHead->a.enc, sizeof(pFrameHead->a.enc), "%s", "AAC");
        pFrameHead->a.samplerate = frameHead.sampleRate;
        pFrameHead->a.samplewidth = frameHead.sampleWidth;
//        pFrameHead->a.compress = frameHead.compressionRatio;
        pFrameHead->a.channels = 1;
        pFrameHead->frametype = 0;
    }else if(frameHead.codec == NK_TFCARD_CODEC_DATA){
		pFrameHead->frametype = P2P_FRAMEHEAD_FRAMETYPE_OOB;
    }else{
        APP_TRACE("unknow frameHead.codec = %d!", frameHead.codec);
        dataSize = 0;
    }
    pFrameHead->rectype = P2P_REC_TYPE_ALL;//8;
    pFrameHead->recchn = 0;

//	APP_TRACE("on read frame done, frame size: %lu, type: %d (rec ctx: %p)",
//			  frameHead.dataSize, frameHead.codec, ctx);
    return dataSize;
#else
    APP_TRACE("firmware do not support tf card!");
    return -1;
#endif
}

ssize_t OnRemoteSetup(void *ctx, const char *setReq, size_t reqSz, void *setResp, size_t maxRespSz)
{
	ssize_t respSz = 0;
    if (!setResp) {
        printf("OnRemoteSetup err\n");
        return 0;
    }
    printf("OnRemoteSetup--->Req%s\n", (char*)setReq);
	if(NULL != setReq)
	{
		respSz = p2p_parse((void *)setReq, setResp);
	}
	if(0 == respSz)
	{
		sprintf((char *)setResp, "{\r\n\"option\" :\"failed\"\r\n}\r\n");
	}else if(-1 == respSz){
		sprintf((char *)setResp, "{\r\n\"option\" :\"Authorization failed\"\r\n}\r\n");
	}

	respSz = strlen((char *)setResp);
	printf("OnRemoteSetup--->Resp:%s, respSz=%d\n", (char*)setResp, respSz);

    return respSz;
}

void*  OnAuP2PCall(void *ctx, void *talkSession, int talkChn, int *errNo)
{
	void * ret = NULL;
#if defined(VOICE_TALK)
    printf("FUNC:OnAuP2PCall1......seesion:%p, talkChn:%d\n", talkSession, talkChn);
	if(GLOBAL_enter_twowaytalk() != 0) {
		printf("OnAuP2PCall busy!\n");
		*errNo = -1;
		return NULL;
	}

	ret = SOUND_QueueisInit();
	if(!ret){
		GLOBAL_leave_twowaytalk();
		*errNo = -2;
	}

	return ret;
#endif
	*errNo = -2;
	return NULL;
}

lp_bindWidth newNodeBindwidth(float bw, void *session)
{
    lp_bindWidth pnode = NULL;
    pnode = (lp_bindWidth)calloc(1, sizeof(st_bindWidth));
    if(pnode == NULL) {
        return NULL;
    }
    pnode->bandWidth = bw;
    pnode->session = session;
    pnode->next = NULL;

    //APP_TRACE("new pnode = 0x%x node->bandWidth = %lf node->session = 0x%x", pnode, pnode->bandWidth, pnode->session);

    return pnode;

}

int insertNewBindwidth(lp_bindWidth phead, float bw, void *session)
{
    int i = 0;
    lp_bindWidth cur_node = NULL, tmp_node = NULL;
    if(phead == NULL) {
        goto INSERT_FAIL;
    }
    else {
        cur_node = phead;
        tmp_node = phead;
        while(cur_node != NULL) {
            tmp_node = cur_node;
            cur_node = cur_node->next;
        }
        //APP_TRACE("insert cur_node = 0x%x", tmp_node);
        cur_node = newNodeBindwidth(bw, session);
        if(cur_node != NULL) {
            tmp_node->next = cur_node;
        }
        else {
            goto INSERT_FAIL;
        }
    }

    return 0;
INSERT_FAIL:
    return -1;

}

int deletNodeBindwidth(lp_bindWidth *phead, void *session)
{
    if(*phead == NULL) {
        return -1;
    }
    lp_bindWidth cur_node = *phead;
    lp_bindWidth tmp_node = NULL;
    if((*phead)->session == session) {
        *phead = cur_node->next;
        free(cur_node);
        cur_node = NULL;
    }
    else {
        cur_node = (*phead)->next;
        tmp_node = *phead;
        while(cur_node != NULL) {
            //APP_TRACE("del node 0x%x (*phead)->session = 0x%x session = 0x%x", *phead, (*phead)->session, session);
            if(cur_node->session == session) {
                tmp_node->next = cur_node->next;
                free(cur_node);
                cur_node = NULL;
            }
            else {
                tmp_node = cur_node;
                cur_node = cur_node->next;
            }
        }
    }

    return 0;

}

void printList(lp_bindWidth phead)
{
    lp_bindWidth cur = phead;
    while(cur != NULL) {
        //APP_TRACE("printList curnode = 0x%x phead->bandWidth = %lf phead->session = 0x%x", cur, cur->bandWidth, cur->session);
        cur = cur->next;
    }

}

float findMinBindwidth(lp_bindWidth phead)
{
    if(phead == NULL) {
        return -1;
    }
    lp_bindWidth cur_node = phead, next_node = NULL;
    float minBw = cur_node->bandWidth;
    //APP_TRACE("find minBw = %lf", minBw);
    while(cur_node->next != NULL) {
        next_node = cur_node->next;
        if(minBw > next_node->bandWidth) {
            minBw = next_node->bandWidth;
        }
        //APP_TRACE("find minBw = %lf next_node->bandWidth = %lf", minBw, next_node->bandWidth);
        cur_node = next_node;
    }

    return minBw;

}

int setNodeBindwidth(lp_bindWidth phead, void *session, float bw)
{
    if(phead == NULL) {
        return -1;
    }
    lp_bindWidth cur_node = phead;
    while(cur_node != NULL) {
        if(cur_node->session == session) {
            cur_node->bandWidth = bw;
            break;
        }
        cur_node = cur_node->next;
    }
    //printList(_headNodeBindWidth);

    return 0;
}

void OnConnected(void *ctx, void *session)
{
#if !defined(ADT)        // ç›®å‰åªç”¨äºŽadityaé¡¹ç›®
    return;
#endif

    if(_headNodeBindWidth == NULL) {
        _headNodeBindWidth = newNodeBindwidth(0xffffffff, session);
    }
    else {
        insertNewBindwidth(_headNodeBindWidth, 0xffffffff, session);
    }

    //printList(_headNodeBindWidth);

}

void OnClosed(void *ctx, void *session)
{
#if !defined(ADT)        // ç›®å‰åªç”¨äºŽadityaé¡¹ç›®
    return;
#endif

    //APP_TRACE("delete session = 0x%x", session);
    deletNodeBindwidth(&_headNodeBindWidth, session);
    //printList(_headNodeBindWidth);
}

#define P2P_BANDWIDTH_STEP (50)//bps
void OnBindwidthChanged(void *ctx, void *session, float c_recv_bw)
{
#if !defined(ADT)        // ç›®å‰åªç”¨äºŽadityaé¡¹ç›®
    return;
#endif

	struct StreamCtx *pCtx = (struct StreamCtx*)ctx;
	if (!pCtx) {
		printf("OnBindwidthChanged failed:nil ctx\n");
		return;
	}

	int bw = 0;
	float minBw = 0;
	//int vin = pCtx->chn;
	//int stream = pCtx->streamNo;
	int vin = 0;
    int stream = 0;
	int ret = 0;
	int *pts_bps = NULL;
	ST_SDK_ENC_STREAM_ATTR stream_attr;
	ST_NSDK_VENC_CH venc_ch;
	memset(&stream_attr,0,sizeof(ST_SDK_ENC_STREAM_ATTR));
    setNodeBindwidth(_headNodeBindWidth, session, c_recv_bw);
    minBw = findMinBindwidth(_headNodeBindWidth);
	bw = (int)minBw*8*85/100;//85%bandwidth

	ret = SDK_ENC_get_stream(vin, stream, &stream_attr);
	switch(stream_attr.enType){
		default:
		case kSDK_ENC_BUF_DATA_H264:
			pts_bps = &stream_attr.H264_attr.bps;
			break;
		case kSDK_ENC_BUF_DATA_H265:
			pts_bps = &stream_attr.H265_attr.bps;
			break;
	}
	
	printf("min bw = %d(%f) now:%d\n", bw, minBw*8, *pts_bps);
	
	if(bw < *pts_bps && bw > 128){
		//need to reduce bandwidth
		*pts_bps = bw;
		SDK_ENC_set_stream (vin, stream, &stream_attr);
	}else if(bw - P2P_BANDWIDTH_STEP > *pts_bps){
		//need to raise bandwidth
		memset(&venc_ch, 0, sizeof(ST_NSDK_VENC_CH));
		NETSDK_conf_venc_ch_get((vin+1)*100+stream+1, &venc_ch);
		
		bw = bw < venc_ch.constantBitRate ? bw : venc_ch.constantBitRate;
		*pts_bps = bw;
		SDK_ENC_set_stream (vin, stream, &stream_attr);
	}
		
}


ssize_t OnAuP2PTalkRecv(void *talkCtx, P2PFrameHead* pVoiceHead, void *voiceData, size_t voiceDataSz)
{
	int ret = -1;
#if defined(VOICE_TALK)
	if(talkCtx){
		ret = SOUND_writeQueue((unsigned char *)voiceData, voiceDataSz, emSOUND_DATA_TYPE_P2P_G711A, emSOUND_PRIORITY_ZERO);
	}
#endif
	return ret;
}

int OnAuP2PHangup(void *talkCtx)
{
#if defined(VOICE_TALK)
    printf("VoP2P Hangup......%p\n", talkCtx);
	if(talkCtx){
		GLOBAL_leave_twowaytalk();
	    SOUND_releasePriority();
	}
	return 0;
#endif
}

void *P2PDeviceStart(void *arg)
{
	if(NULL == arg)
	{
		printf("P2PStart create DeviceCtx failed\n");
		return NULL;
	}
	struct P2PDeviceDemo *pDemo = (struct P2PDeviceDemo *)arg;

	P2PSDKSetLogLevel(P2PSDK_EMERG);

	struct P2PSDKDevice device = { 0 };
    snprintf(device.info.sn, sizeof(device.info.sn), "%s", pDemo->serialNo);
    snprintf(device.info.version, sizeof(device.info.version), "%s", pDemo->version);
    snprintf(device.info.vendor, sizeof(device.info.vendor), "%s", pDemo->vendor);
    device.info.max_ch = pDemo->max_ch;
    device.ctx = pDemo;
    
    device.OnAuth = OnAuth;
    device.OnPtzCtrl = OnPtzCtrl;
    device.OnSettingsReadStreamInfo = OnSettingsReadStreamInfo;
    device.OnAttachStream = OnAttachStream;
    device.OnDetachStream = OnDetachStream;
    device.OnReadFrame = OnReadFrame;
	device.OnReadFrameEx = OnReadFrameEx;
	device.OnAfterReadFrame = OnreleaseMediaBufFrame;
	device.OnDevOnline = OnDevOnline;
	device.OnDevConnectReq = NULL;
    device.OnFetchRecList = OnFetchRecListNew;
    device.OnOpenRecFile = NULL;
	device.OnOpenRecFile2 = OnOpenRecFile2;
    device.OnCloseRecFile = OnCloseRecFilesNew;
    device.OnReadRecFrame = OnReadRecFrameNew;
    device.OnRemoteSetup = OnRemoteSetup;
    device.OnConnected = OnConnected;
    device.OnClosed = OnClosed;
	device.OnBindwidthChanged = OnBindwidthChanged;

    device.OnVoP2PCall = OnAuP2PCall;
    device.OnVoP2PTalkRecv = OnAuP2PTalkRecv;
    device.OnVoP2PHangup = OnAuP2PHangup;
    P2PSDKInit(&device);
//    P2PSDKSetEseeServerAddr("115.28.0.173", 60101);
//    P2PSDKSetEseeServerAddr("113.105.223.77", 60101);
	ST_MODEL_CONF model_conf;
	if(NULL != MODEL_CONF_get(&model_conf)){
		P2PSDKSetCamDes(0, model_conf.modelName);
	}

	ST_CUSTOM_SETTING custom;
	if((0 == CUSTOM_get(&custom)) && (true == CUSTOM_check_string_valid(custom.function.p2pServerIp))){
		P2PSDKSetEseeServerAddr(custom.function.p2pServerIp, 60101);
	}
	
	return 0;
}

int P2PAddVconApp(void *pP2PDevice, const char *service, int type, fAddrGenerator addr_generator)
{
	if (!pP2PDevice) {
		printf("P2PAddVconApp failed: nil pP2PDevice\n");
		return NULL;
	}

	return P2PSDKAddVconApp(service, type, addr_generator);
}


/* P2PDevice TEst!!! */
void OnDevOnline(const char *eseeid, void *ctx)
{
    struct P2PDeviceDemo *pDemo = (struct P2PDeviceDemo*)ctx;
    snprintf(pDemo->eseeId, sizeof(pDemo->eseeId), "%s", eseeid);
    printf("Device %s logined Server!!!\n", pDemo->eseeId);
}

int SpookAddrGenerator(struct sockaddr *app_service_addr)
{
    struct sockaddr_in* pAddr = (struct sockaddr_in*)app_service_addr;
    pAddr->sin_family = AF_INET;
    pAddr->sin_addr.s_addr = inet_addr("192.168.28.5");
    pAddr->sin_port = htons(80);
    return 0;
}

int P2P_sdkdestroy()
{
	P2PSDKDeinit();

}


int P2PStart(const char *sn, const char *version, const char *vend)
{
	// init p2pDevice
	static struct P2PDeviceDemo Pdevice;
	pthread_t P2Ptid;
	int result=-1;

#if defined(ADT)
	char tutk_uid[33];

	if (!SECURE_CHIP_is_init_success()) {
		printf("%s:%d Secure chip has not inited, P2P not start.\n",
			   __FUNCTION__, __LINE__);
		return -1;
	}

	if (0 != SECURE_CHIP_get_data(SECURE_CHIP_DATA_UID, tutk_uid, sizeof(tutk_uid))) {
		printf("%s:%d Get tutk uid from secure chip failed, P2P not start.\n",
			   __FUNCTION__, __LINE__);
		return -1;
	}

	snprintf(Pdevice.serialNo, sizeof(Pdevice.serialNo), "%s", tutk_uid);
#else
    snprintf(Pdevice.serialNo, sizeof(Pdevice.serialNo), "%s", sn);
#endif
	snprintf(Pdevice.version, sizeof(Pdevice.version), "%s", version);
	
    snprintf(Pdevice.vendor, sizeof(Pdevice.vendor), "%s", vend);
    Pdevice.max_ch = 1;
	Pdevice.OnDevOnline = OnDevOnline;
	Pdevice.ctx = &Pdevice;
    
    printf("P2PDeviceDemo version:%s\n", Pdevice.version);

	result = pthread_create(&P2Ptid, NULL, P2PDeviceStart, (void *)&Pdevice);
	if(0 == result)
	{
		printf("P2P pthread start sucess\n");
	}
	else
	{
		printf("P2P pthread create failed\n");
		return -1;	
	}
	
	return 0;
}
