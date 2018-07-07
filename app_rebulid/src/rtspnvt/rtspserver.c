#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "sock.h"
#include "rtspdef.h"
#include "vlog.h"
#include "rtsplib.h"
#include "rtspserver.h"
#include "gnu_win.h"
#include "netsdk.h"
#include "global_runtime.h"
#include "base/ja_process.h"

static SOCK_t server_fd=-1;
static int m_toggle=true;

static void signal_handle(int sign_no) 
{
    m_toggle=false;
    VLOG(VLOG_DEBUG,"rtsp server kill!");
    SOCK_close(server_fd);
	RTSPS_stop();
	MSLEEP(3000);
}

#if defined(_WIN32) || defined(_WIN64) || defined(NOCROSS)
void* RTSPS_proc(void *para) 
{
	int ret;
	Rtsp_t *r=NULL;
	RtspStream_t *stream=NULL;
	char media_name[16];
	uint32_t base_ts=0xffffffff,last_ts=0xffffffff;
	int out_success= false;
	int bStreamInit=false,avc_flag=false;
	MillisecondTimer_t t_tmp,base_t_v,base_t_a;
	MilliSecond_t m_lasttime;
	int i_count_v=0,real_fps_v=25,send_fps_v=0;
	int i_count_a=0,real_fps_a=25,send_fps_a=0;
	int i_fix_to = 0;
	int max_sock=0;
	//int i_count=0,static_fps[10];
	ThreadArgs_t *args=(ThreadArgs_t *)para;
	int sock=args->RParam;
	int *trigger=(int *)args->LParam;
	fd_set read_set;
	//fd_set write_set;
	struct timeval timeout;
	
#if defined(_WIN32) || defined(_WIN64)
#else	
	pthread_detach(pthread_self());
#endif
	r=RTSP_SERVER_init(sock,RTSP_ENABLE_AUDIO);
	if(r == NULL) return NULL;
	stream = (RtspStream_t *)&r->s;

	timeout.tv_sec = 0;
	timeout.tv_usec = 1000;
	
	VLOG(VLOG_CRIT,"rtsp server proc enter...");
	while(r->toggle && (*trigger)){
		timeout.tv_sec = 0;
		timeout.tv_usec = 1000;
		FD_ZERO(&read_set);
		FD_SET(r->sock,&read_set);
		max_sock = r->sock;
#if defined(RTCP_SENDER)
		if(r->rtcp_video){
			FD_SET(r->rtcp_video->sock,&read_set);
			max_sock = MAXAB(max_sock,r->rtcp_video->sock);
		}		
		if(r->rtcp_audio){
			FD_SET(r->rtcp_audio->sock,&read_set);
			max_sock = MAXAB(max_sock,r->rtcp_audio->sock);
		}
#endif
		ret=select(max_sock+1,&read_set,NULL,NULL,&timeout);
		if(ret < 0){
			VLOG(VLOG_ERROR,"select failed");
			break;
		}else if(ret == 0){
			//timeout
		}else{
			if(FD_ISSET(r->sock,&read_set)){
				if(RTSP_read_message(r)==RTSP_RET_FAIL) break;
				if(RTSP_parse_message(r,NULL,RTCP_HANDLE1) == RTSP_RET_FAIL) break;
			}
#if defined(RTCP_SENDER)
			if(r->rtcp_audio){
				if(FD_ISSET(r->rtcp_audio->sock,&read_set)){
					RTCP_handle_packet(r->rtcp_audio,NULL,0);
				}
			}
			if(r->rtcp_video){
				if(FD_ISSET(r->rtcp_video->sock,&read_set)){
					RTCP_handle_packet(r->rtcp_video,NULL,0);
				}
			}
#endif

		}// end select
		if(r->state == RTSP_STATE_PLAYING){
			out_success = false;
			if(bStreamInit == false){
				RTSP_find_stream(r->stream,media_name);
				if(RTSP_STREAM_init(stream,media_name)<0)
					break;
				bStreamInit = true;
				MilliTimerStart(base_t_v);
				MilliTimerStart(base_t_a);
			}
			if(RTSP_STREAM_next(stream)==RTSP_RET_OK){
				//printf("1  !!!!!!!!!!!!!!!!!!!!!!! streamtype:%x %d\n",r->stream_type,stream->type);
				VLOG(VLOG_DEBUG,"get stream type:%d success,size:%d",stream->type,stream->size);
				if(send_fps_v == 0) send_fps_v = stream->fps;
				if(avc_flag == false){
					if(stream->type == RTSP_STREAM_TYPE_VIDEO && stream->isKeyFrame == true){
						avc_flag=true;
						base_ts=stream->timestamp;
					}else
						continue;
				}
				if((stream->type == RTSP_STREAM_TYPE_VIDEO) && (r->stream_type & RTSP_STREAM_VIDEO)){
					if(RTP_send_packet(r->rtp_video,stream->data,stream->size,
						SDP_MEDIA_H264_FREQ/1000*(stream->timestamp-base_ts),
						RTP_DEFAULT_VIDEO_TYPE)==RTSP_RET_FAIL){
						break;
					}else{
						i_count_v++;
						MilliTimerStop(base_t_v,t_tmp,m_lasttime);
						if(m_lasttime > 1000){
							MilliTimerStart(base_t_v);
							real_fps_v = i_count_v;
							i_count_v = 0;
							if(real_fps_v < send_fps_v){
								i_fix_to += (send_fps_v - real_fps_v)*1500;
							}else{
								i_fix_to -= (real_fps_v - send_fps_v)*1500;
							}
							if(i_fix_to < 0) i_fix_to = 0;
							if(i_fix_to > stream->inspeed) i_fix_to = stream->inspeed;
							// caculate the send fps
							/*
							if(i_count < 10){
								static_fps[i_count]=real_fps_v;
								i_fix_to = stream->inspeed;
							}else if(i_count==10){
								i_count;
								send_fps_v=caculate_average(static_fps,10);
								i_fix_to = 5000;
							}
							i_count++;*/
							VLOG(VLOG_DEBUG,"**** real video fps:%d(%d,%d) fix_to:%d ****",
								real_fps_v,stream->fps,send_fps_v,i_fix_to);
						}
						out_success = true;
						if((stream->timestamp - last_ts) > 40){
							//printf("timestamp dev:%d\n",stream->timestamp -  last_ts);
						}
						last_ts = stream->timestamp;
					}
				}
				else if((stream->type == RTSP_STREAM_TYPE_AUDIO) && (r->stream_type & RTSP_STREAM_AUDIO)){
					if(RTP_send_packet(r->rtp_audio,stream->data,stream->size,
						SDP_MEDIA_G711_FREQ/1000*(stream->timestamp-base_ts),
						RTP_TYPE_PCMA)==RTSP_RET_FAIL){
						break;
					}else{
						i_count_a++;
						MilliTimerStop(base_t_a,t_tmp,m_lasttime);
						if(m_lasttime > 1000){
							MilliTimerStart(base_t_a);
							real_fps_a = i_count_a;
							i_count_a = 0;
							VLOG(VLOG_INFO,"**** real audio fps:%d ****",real_fps_a);
						}
					}
				}else{
					//VLOG(VLOG_DEBUG,"unsupport stream type:%d",stream->type);
					//break;
				}
				avc_flag =true;
			}
			if(out_success){
				if((stream->inspeed > i_fix_to) && (stream->isKeyFrame == false)){
					MSLEEP((stream->inspeed - i_fix_to)/1000);
				}
				out_success = false;
			}
		}
		// 	process rtcp
#if defined(RTCP_SENDER)
		if(r->rtcp_audio){
			RTCP_process(r->rtcp_audio);
		}
		if(r->rtcp_video){
			RTCP_process(r->rtcp_video);
		}
#endif
	}// end of while
	*trigger = false;
	RTSP_destroy(r);
	
	VLOG(VLOG_CRIT,"rtsp server proc exit");
	
	*trigger = false;
	return NULL;
}


#else	

#include "media_buf.h"
#include "sdk/sdk_enc.h"

int rtsp_get_avc_hook(char *stream , void *data)
{
	int media_buf_id = MEDIABUF_lookup_byname(stream);
	VLOG(VLOG_CRIT, "rtsp get avc hook , mediabuf %d(%s)", media_buf_id, stream);
	return 0;
	//return MEDIABUF_out_additional_data(media_buf_id, data, sizeof(H264AVC_t));
}

int proc_MINIRTSP_SERVER_GET_VENC(char *stream_name,int* channal_venc){

	if((NULL == stream_name)||(NULL == channal_venc))
		return -1;
	ST_NSDK_VENC_CH venc_ch;
	char* ptr_name = NULL;
	if(ptr_name ==NULL){
		ptr_name = strstr(stream_name,"ch0_0.264");	
	}
	if(ptr_name ==NULL){
		ptr_name = strstr(stream_name,"ch1/main/av_stream");
	}
	if(ptr_name ==NULL){
		ptr_name = strstr(stream_name,"ch0_1.264");
	}
	if(ptr_name ==NULL){
		ptr_name = strstr(stream_name,"ch1/sub/av_stream");
	}
	if(ptr_name ==NULL){
		strcpy(stream_name,"ch0_0.264");
		ptr_name = stream_name;
	}
	
	if(strncmp(ptr_name,"ch0_1.264",sizeof("ch0_1.264")) ==0 || strncmp(ptr_name,"ch1/sub/av_stream",sizeof("ch1/sub/av_stream")) ==0 ){
		NETSDK_conf_venc_ch_get(102,&venc_ch);
	}else{
		NETSDK_conf_venc_ch_get(101,&venc_ch)	;
	}	
	*channal_venc = (kNSDK_CODEC_TYPE_H265 == venc_ch.codecType) ? RTP_TYPE_DYNAMIC_H265 : RTP_TYPE_DYNAMIC;
	return *channal_venc;
}

int proc_get_venc_code(int sock){
	ST_NSDK_VENC_CH venc_ch;
//	printf("liry rtspserver.c 258 r->stream %s",r->stream);
	int ret;
	char stream_name[256]= {0};
	char* ptr_name = NULL;
	ret = recv(sock,stream_name,sizeof(stream_name),MSG_PEEK);
	if(ret > 0 ){
		if(ptr_name ==NULL){
			ptr_name = strstr(stream_name,"ch0_0.264");	
		}
		if(ptr_name ==NULL){
			ptr_name = strstr(stream_name,"ch1/main/av_stream");
		}
		if(ptr_name ==NULL){
			ptr_name = strstr(stream_name,"ch0_1.264");
		}
		if(ptr_name ==NULL){
			ptr_name = strstr(stream_name,"ch1/sub/av_stream");
		}
		if(ptr_name ==NULL){
			strcpy(stream_name,"ch0_0.264");
			ptr_name = stream_name;
		}
	}else{
		strcpy(stream_name,"ch0_0.264");
		ptr_name = stream_name;
	}
	if(strncmp(ptr_name,"ch0_1.264",strlen("ch0_1.264")) ==0 || strncmp(ptr_name,"ch1/sub/av_stream",strlen("ch1/sub/av_stream")) ==0 ){
		NETSDK_conf_venc_ch_get(102,&venc_ch);
	}else{
		NETSDK_conf_venc_ch_get(101,&venc_ch)	;
	}	
	int codecType = (kNSDK_CODEC_TYPE_H265 == venc_ch.codecType) ? RTP_TYPE_DYNAMIC_H265 : RTP_TYPE_DYNAMIC;
	return codecType;
}

void* RTSPS_proc(void *para) 
{
	int ret;
	Rtsp_t *r=NULL;
//	RtspStream_t *stream=NULL;
	char media_name[16];
	uint32_t base_ts_v = 0xffffffff, base_ts_a = 0xffffffff, last_ts=0xffffffff;
	int out_success= false;
	int bStreamInit=false;
	MillisecondTimer_t t_tmp,base_t_v,base_t_a;
	MilliSecond_t m_lasttime;	
	time_t t_now;
	int i_count_v=0,real_fps_v=25, fps_v = 0;
	int i_count_a=0,real_fps_a=25, fps_a = 0;
	int max_sock=0;
	ThreadArgs_t *args=(ThreadArgs_t *)para;
	int sock=args->RParam;
	int *trigger=(int *)args->LParam;
	lpMEDIABUF_USER media_user=NULL;
	int media_id;
	uint32_t media_speed;
	int is_first_i_frame = 1;
	int i_frame_cnt = 0;
	fd_set read_set;
	//fd_set write_set;
	struct timeval timeout;
	signal(SIGPIPE, SIG_IGN);
	
	pthread_detach(pthread_self());
//	r=RTSP_SERVER_init(sock,RTSP_ENABLE_AUDIO);
    // 无线单品打开码流启用nvr兼容模式,关闭跳帧参考,gop改为2秒
#if defined(CX)
    if(false == GLOBAL_sn_front()) {
        ST_NSDK_VENC_CH venc_ch;
        NETSDK_conf_venc_ch_get(101, &venc_ch);
        if(venc_ch.ImageTransmissionModel != eNSDK_COMPATIBILITY_MODE) {
            venc_ch.ImageTransmissionModel = eNSDK_COMPATIBILITY_MODE;
            NETSDK_conf_venc_ch_set(101, &venc_ch);
        }
        NETSDK_conf_venc_ch_get(102, &venc_ch);
        if(venc_ch.ImageTransmissionModel != eNSDK_COMPATIBILITY_MODE) {
            venc_ch.ImageTransmissionModel = eNSDK_COMPATIBILITY_MODE;
            NETSDK_conf_venc_ch_set(102, &venc_ch);
        }
    }
#endif

    /* 打开码流关闭有线和无线dhcp */
    ST_NSDK_NETWORK_INTERFACE lan0, wlan0;

    NETSDK_conf_interface_get(1, &lan0);
    NETSDK_conf_interface_get(4, &wlan0);

    if(lan0.lan.addressingType != kNSDK_NETWORK_LAN_ADDRESSINGTYPE_STATIC) {
        lan0.lan.addressingType = kNSDK_NETWORK_LAN_ADDRESSINGTYPE_STATIC;
        NETSDK_conf_interface_set(1, &lan0, eNSDK_CONF_SAVE_JUST_SAVE);
        NK_SYSTEM("kill -9 `pidof udhcpc`");
    }
    if(wlan0.wireless.dhcpServer.dhcpAutoSettingEnabled == true) {
        wlan0.wireless.dhcpServer.dhcpAutoSettingEnabled = false;
        NETSDK_conf_interface_set(4, &wlan0, eNSDK_CONF_SAVE_JUST_SAVE);
        NK_SYSTEM("kill -9 `pidof udhcpc`");
    }

	int codec = proc_get_venc_code( sock);
	if(RTP_TYPE_DYNAMIC_H265 == codec){
		r = RTSP_SERVER_init2(sock,RTSP_ENABLE_AUDIO,RTP_TYPE_DYNAMIC_H265);
	}else{
		r = RTSP_SERVER_init2(sock,RTSP_ENABLE_AUDIO,RTP_TYPE_DYNAMIC);
	}
	if(r == NULL) return NULL;
//	stream = (RtspStream_t *)&r->s;

	timeout.tv_sec = 0;
	timeout.tv_usec = 1000;
	r->get_avc = NULL;//rtsp_get_avc_hook;
	
	VLOG(VLOG_CRIT,"rtsp server :%s proc enter... r%p", r->ip_me,r);
	while(r->toggle && (*trigger)){
		timeout.tv_sec = 0;
		timeout.tv_usec = 1000;
		FD_ZERO(&read_set);
		FD_SET(r->sock,&read_set);
		
		max_sock = r->sock;
#if defined(RTCP_SENDER)
		if(r->rtcp_video){
			FD_SET(r->rtcp_video->sock,&read_set);
			max_sock = MAXAB(max_sock,r->rtcp_video->sock);
		}		
		if(r->rtcp_audio){
			FD_SET(r->rtcp_audio->sock,&read_set);
			max_sock = MAXAB(max_sock,r->rtcp_audio->sock);
		}
#endif
		
		ret=select(max_sock+1,&read_set, NULL, NULL,&timeout);
		if(ret < 0){
			VLOG(VLOG_ERROR,"select failed");
			break;
		}else if(ret == 0){
			MSLEEP(1);
			//timeout
		}else{
//			printf("liry RTSPS_proc r %p       read_set %p \n",r,read_set);
			if(FD_ISSET(r->sock,&read_set)){
				time(&r->m_sync);
				if(RTSP_read_message(r)==RTSP_RET_FAIL) break;
				if(RTSP_parse_message(r,NULL,RTCP_HANDLE1) == RTSP_RET_FAIL) break;
			}
#if defined(RTCP_SENDER)
			if(r->b_interleavedMode == false){
				if(r->rtcp_audio){
					if(FD_ISSET(r->rtcp_audio->sock,&read_set)){
						time(&r->m_sync);
						RTCP_handle_packet(r->rtcp_audio,NULL,0);
					}
				}
				if(r->rtcp_video){
					if(FD_ISSET(r->rtcp_video->sock,&read_set)){
						time(&r->m_sync);
						RTCP_handle_packet(r->rtcp_video,NULL,0);
					}
				}
			}
#endif

		}// end select
		//
		// keep liveness
		if (r->b_interleavedMode == false) {
			time(&t_now);
			if (((uint64_t)t_now - (uint64_t)r->m_sync) > 200) {
				VLOG(VLOG_WARNING, "long time no see keep liveness, teardown!");
				break;
			}
		}
		//
		//if(r->state == RTSP_STATE_PLAYING){
		while(r->state == RTSP_STATE_PLAYING && r->rtp_video){
			
			out_success = false;
			if(bStreamInit == false){
				RTSP_find_stream(r->stream,media_name);
				media_id = MEDIABUF_lookup_byname(media_name);
				if(media_id >= 0){
					media_user = MEDIABUF_attach(media_id);
					if(media_user == NULL){
						printf("media attach falied!\n");
						goto _RTSPS_EXIT;
					}
					media_speed = MEDIABUF_in_speed(media_id);
					MEDIABUF_sync(media_user);
					is_first_i_frame = 1;
					i_frame_cnt = 0;
				}else{
					printf("lookup name failed falied!\n");
					goto _RTSPS_EXIT;
				}
				printf("rtsp stream:%s init done!\n",media_name);
				bStreamInit = true;
				MilliTimerStart(base_t_v);
				MilliTimerStart(base_t_a);
			}
			if(0 == MEDIABUF_out_lock(media_user)){
				const lpSDK_ENC_BUF_ATTR attr = NULL;
				size_t out_size = 0;
				
				if(0 == MEDIABUF_out(media_user, (void **)&attr, NULL, &out_size)){
					const void* const raw_ptr = (void*)(attr + 1);
					size_t const raw_size = attr->data_sz;					
					uint64_t mb_timestamp = attr->timestamp_us;
					VLOG(VLOG_DEBUG,"get stream type:0x%08x success,size:%d",attr->type,raw_size);
					/*
					if(avc_flag == false){
						if(attr->type == kSDK_ENC_BUF_DATA_H264 && attr->h264.keyframe== true){
							avc_flag=true;
							base_ts=(uint32_t)(attr->timestamp_us/1000);
						}else{
							MEDIABUF_out_unlock(media_user);
							continue;
						}
					}*/
					/*
					if(is_first_i_frame && attr->h264.keyframe && attr->type == kSDK_ENC_BUF_DATA_H264){
						if((++i_frame_cnt) >= 2){
							is_first_i_frame = 0;
							base_ts_v=(uint32_t)(attr->timestamp_us/1000);
							//printf("timeus: %llu %u\n", attr->timestamp_us, base_ts);
						}
					}
					if(i_frame_cnt < 2){
						MEDIABUF_out_unlock(media_user);
						continue;
					}*/
					if(((attr->type == kSDK_ENC_BUF_DATA_H264) ||(attr->type == kSDK_ENC_BUF_DATA_H265)) && (r->stream_type & RTSP_STREAM_VIDEO)){
						MillisecondTimer_t m_lastt;
						MilliSecond_t m_dura;
						MilliTimerStart(m_lastt);
						fps_v = attr->h264.fps;
						if(base_ts_v == 0xffffffff) base_ts_v = (uint32_t)(attr->timestamp_us/1000);

						uint32_t ts_ut = SDP_MEDIA_H264_FREQ/1000*((uint32_t)(attr->timestamp_us/1000)-base_ts_v);
						
//						extern int RTP_proce_hiconn_md(Rtp_t *rtp,char *src,uint32_t len,uint32_t ts,int payload_type);
//						RTP_proce_hiconn_md(r->rtp_video,raw_ptr,raw_size,ts,RTP_DEFAULT_VIDEO_TYPE);
						if(attr->type == kSDK_ENC_BUF_DATA_H264){
							ret = RTP_pack(r->rtp_video, (char *)raw_ptr, raw_size,
							ts_ut,
							RTP_DEFAULT_VIDEO_TYPE);
						}else if(attr->type == kSDK_ENC_BUF_DATA_H265){
//							printf("liry attr->type == kSDK_ENC_BUF_DATA_H265 \n ");
							ret = RTP_pack(r->rtp_video, (char *)raw_ptr, raw_size,
							ts_ut,
							RTP_TYPE_DYNAMIC_H265);
						}
						MilliTimerStop(m_lastt, t_tmp, m_dura);
						//if(0 == strcmp(media_name, "720p.264"))
						//	printf("pack size:%8u(%d) time:%8ums\t",raw_size, attr->h264.keyframe, m_dura);
						MEDIABUF_out_unlock(media_user);
						if(ret == RTSP_RET_FAIL){
							VLOG(VLOG_ERROR, "pack video failed!");
							goto _RTSPS_EXIT;
						}else{
							MilliTimerStart(m_lastt);
							if(RTP_send(r->rtp_video) == RTSP_RET_FAIL){
								VLOG(VLOG_ERROR, "send video failed!");
								goto _RTSPS_EXIT;
							}	
							MilliTimerStop(m_lastt, t_tmp, m_dura);
							//if(0 == strcmp(media_name, "720p.264"))
							//	printf("send time:%8ums\r\n", m_dura);
							i_count_v++;
							MilliTimerStop(base_t_v,t_tmp,m_lasttime);
							if(m_lasttime > 1000){
								MilliTimerStart(base_t_v);
								real_fps_v = i_count_v;
								i_count_v = 0;
								VLOG(VLOG_DEBUG,"**** real video fps:%d(%d)****",
									real_fps_v, fps_v);
							}
							out_success = true;
							if(abs((uint32_t)(mb_timestamp/1000)-last_ts) > 72){
								//printf("timestamp dev:%u %u->%u\n",(uint32_t)(mb_timestamp/1000) -  last_ts,
								//	last_ts, (uint32_t)(mb_timestamp/1000));
							}
							last_ts = (uint32_t)(mb_timestamp/1000);
						}
					}
					else if((attr->type == kSDK_ENC_BUF_DATA_G711A) && (r->stream_type & RTSP_STREAM_AUDIO)){						
						if(r->rtp_audio != NULL){
							if(base_ts_a == 0xffffffff) base_ts_a = (uint32_t)(attr->timestamp_us/1000);
							
							ret = RTP_pack(r->rtp_audio, (char *)raw_ptr, raw_size,
								SDP_MEDIA_G711_FREQ/1000*((uint32_t)(attr->timestamp_us/1000)-base_ts_a),
								RTP_TYPE_PCMA);
							MEDIABUF_out_unlock(media_user);
							if(ret == RTSP_RET_FAIL){
								VLOG(VLOG_ERROR, "pack audio failed!");
								goto _RTSPS_EXIT;
							}else{
								if(RTP_send(r->rtp_audio) == RTSP_RET_FAIL){
									VLOG(VLOG_ERROR, "send audio failed!");
									goto _RTSPS_EXIT;
								}	
								i_count_a++;
								MilliTimerStop(base_t_a,t_tmp,m_lasttime);
								if(m_lasttime > 1000){
									MilliTimerStart(base_t_a);
									real_fps_a = i_count_a;
									i_count_a = 0;
									VLOG(VLOG_DEBUG,"**** real audio fps:%d ****",real_fps_a);
								}
							}
						}else{
							MEDIABUF_out_unlock(media_user);
						}
					}else{
						MEDIABUF_out_unlock(media_user);
					}
					
				}else{
					MEDIABUF_out_unlock(media_user);
					//usleep(3000);
					break;
				}
			}else{
				usleep(5000);
				VLOG(VLOG_WARNING, "media outlock failed!!!");
			}
			//
		}
		// 	process rtcp
#if defined(RTCP_SENDER)
		if(r->rtcp_audio){
			RTCP_process(r->rtcp_audio);
		}
		if(r->rtcp_video){
			RTCP_process(r->rtcp_video);
		}
#endif
	}// end of while

_RTSPS_EXIT:	
	if(media_user){
		MEDIABUF_detach(media_user);
		media_user = NULL;
	}
	*trigger = false;
	r->sock = -1; //spook would close this socket
	RTSP_destroy(r);
	
	VLOG(VLOG_CRIT,"rtsp server proc exit");
	return NULL;
}



#endif


int RTSPS_start() 
{
    SOCK_t client_fd;
	SOCKADDR_t client;
	SOCKADDR_IN_t sin;
	SOCKLEN_t size;	
	ThreadId_t tid;
	ThreadArgs_t args;
	args.data = NULL;
	args.LParam = (void *)&m_toggle;

    signal(SIGINT,signal_handle);            
	
    server_fd=SOCK_tcp_listen(RTSP_DEFAULT_PORT);
    do {
        size=sizeof(struct sockaddr);
        if ((client_fd = accept(server_fd,(SOCKADDR_t *)&client,&size)) == -1) {
            VLOG(VLOG_ERROR,"accept failed, errno=%d!!!",errno);
            return -1;
        } else {
            sin=*((SOCKADDR_IN_t *)&client);
            VLOG(VLOG_INFO,"accept connect from:%s sockfd:%d",inet_ntoa(sin.sin_addr),client_fd);
			m_toggle = true;
			args.RParam = client_fd;
            THREAD_create(tid,RTSPS_proc,((void *)&args));
        }
    } while (m_toggle);

    return 0;
}

void RTSPS_stop() 
{
	m_toggle = false;
}

int RTSPS_test(int argc,char *argv[])
{
	RTSP_add_stream("test.h264","test.h264");
	RTSP_add_stream("720p.h264","test.h264.720p");
	RTSP_add_stream("360p.h264","test.h264.360p");
	RTSPS_start();
	return 0;
}


