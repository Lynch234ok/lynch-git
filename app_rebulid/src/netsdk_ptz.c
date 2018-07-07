
#include "netsdk.h"
#include "netsdk_util.h"
#include "netsdk_private.h"
#include "generic.h"
#include "sdk/sdk_api.h"
#include "app_overlay.h"
#include "ticker.h"
#include "generic.h"
#include "app_debug.h"

/////////////////////////////////////////////////////////////////////////////////////////////////
// NetSDK general opertaions
/////////////////////////////////////////////////////////////////////////////////////////////////
static int ptz_sync_read_lock()
{
	return NETSDK_private_read_lock(&netsdk->ptz_sync);
}

static int ptz_sync_try_read_lock()
{
	return NETSDK_private_try_read_lock(&netsdk->ptz_sync);
}

static int ptz_sync_write_lock()
{
	return NETSDK_private_write_lock(&netsdk->ptz_sync);
}

static int ptz_sync_try_write_lock()
{
	return NETSDK_private_try_write_lock(&netsdk->ptz_sync);
}

static int ptz_sync_unlock()
{
	return NETSDK_private_unlock(&netsdk->ptz_sync);
}

void NETSDK_conf_ptz_save()
{
	ptz_sync_write_lock();
	APP_TRACE("NetSDK PTZ Conf Save!!");
	NETSDK_conf_save(netsdk->ptz_conf, "ptz");
	ptz_sync_unlock();
}

void NETSDK_conf_ptz_save2()
{
	if(netsdk->ptz_conf_save){
		netsdk->ptz_conf_save(eNSDK_CONF_SAVE_JUST_SAVE, 1);
	}else{
		NETSDK_conf_ptz_save();
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////////

/*
/NetSDK/PTZ/channels
/NetSDK/PTZ/channels/properties
/NetSDK/PTZ/channels/ID
/NetSDK/PTZ/channels/ID/properties
/NetSDK/PTZ/channels/ID/control
*/


static int ptz_channel(LP_HTTP_CONTEXT context, HTTP_CSTR_t sub_uri,
	LP_JSON_OBJECT ptzRef, LP_JSON_OBJECT ptzDup, int id, char *content, int content_max)
{
	int i = 0, ii = 0, ret = kNSDK_INS_RET_INVALID_OPERATION;
	char text[128] = {""};

	if(H_IS_GET(context->request_header->method)){
		LP_JSON_OBJECT channelList = NETSDK_json_get_child(ptzDup, "ptzChannel");
		int const n_channelList = json_object_array_length(channelList);
		
		if(0 == id){
			snprintf(content, content_max, "%s", json_object_to_json_string(channelList));
			ret = kNSDK_INS_RET_CONTENT_READY;
		}else if(id > 0){
			if(id - 1 < n_channelList){
				LP_JSON_OBJECT channel = json_object_array_get_idx(channelList, id - 1);
				if(NULL != channel){
					snprintf(content, content_max, "%s", json_object_to_json_string(channel));
					ret = kNSDK_INS_RET_CONTENT_READY;
				}
			}
		}
	}else if(H_IS_PUT(context->request_header->method)){
		if(!context->request_content || 0 == context->request_content_len){
			ret = kNSDK_INS_RET_INVALID_DOCUMENT;
		}else{
			LP_JSON_OBJECT document = NETSDK_json_parse(context->request_content);
			APP_TRACE("Content : %s", json_object_to_json_string(document));
			if(!document){
				ret = kNSDK_INS_RET_INVALID_DOCUMENT;
			}else{
				if(0 == ptz_sync_write_lock()){
					LP_JSON_OBJECT channelList = NETSDK_json_get_child(ptzRef, "ptzChannel");
					int const n_channelList = json_object_array_length(channelList);
					
					if(id > 0){
						if(id - 1 < n_channelList){
							LP_JSON_OBJECT channel = json_object_array_get_idx(channelList, id - 1);
							LP_JSON_OBJECT channel2 = document;
							LP_JSON_OBJECT channelExternalControl = NETSDK_json_get_child(channel, "externalControl");
							LP_JSON_OBJECT channelExternalControl2 = NETSDK_json_get_child(channel2, "externalControl");
							
							// content -> channel
							NETSDK_json_copy_child(channel2, channel, "id");
							NETSDK_json_copy_child(channel2, channel, "serialPortID");
							NETSDK_json_copy_child(channel2, channel, "videoInputID");
							NETSDK_json_copy_child(channel2, channel, "duplexMode");
							NETSDK_json_copy_child(channel2, channel, "pulseDuration");
							NETSDK_json_copy_child(channel2, channel, "outputState");
							NETSDK_json_copy_child(channel2, channel, "controlType");
							if(NULL != channelExternalControl && NULL != channelExternalControl2){
								NETSDK_json_copy_child(channelExternalControl2, channelExternalControl, "protocol");
								NETSDK_json_copy_child(channelExternalControl2, channelExternalControl, "address");
							}
							ret = kNSDK_INS_RET_OK;
						}
					}
					ptz_sync_unlock();
					NETSDK_conf_ptz_save();
				}	
				json_object_put(document);
				document = NULL;
			}
		}
	}
	APP_TRACE(content); 
	return ret;
}


static void ptz_channel_remove_properties(LP_JSON_OBJECT ptz)
{
	int i = 0, ii = 0;
	LP_JSON_OBJECT channelList = NETSDK_json_get_child(ptz, "ptzChannel");
	int const n_channelList = json_object_array_length(channelList);
	
	for(i = 0; i < n_channelList; ++i){
		LP_JSON_OBJECT channel = json_object_array_get_idx(channelList, i);
		LP_JSON_OBJECT channelExternalControl = NETSDK_json_get_child(channel, "externalControl");
		NETSDK_json_remove_properties(channelExternalControl);
		NETSDK_json_remove_properties(channel);
	}
}

static int ptz_instance(LP_HTTP_CONTEXT context, HTTP_CSTR_t sub_uri, char *content, int content_max)
{
	int id = 0;
	int ret = kNSDK_INS_RET_INVALID_OPERATION;
	HTTP_CSTR_t prefix = NULL;
	LP_JSON_OBJECT jsonPTZ = json_object_get(netsdk->ptz_conf);
	LP_JSON_OBJECT ptzRef = jsonPTZ;
	LP_JSON_OBJECT ptzDup = NETSDK_json_dup(ptzRef);

	if(H_IS_GET(context->request_header->method) && !NSDK_PROPERTIES(sub_uri)){
		ptz_channel_remove_properties(ptzDup);
	}

	if(prefix = "/CHANNELS", 0 == strncmp(sub_uri, prefix, strlen(prefix))){
		ret = ptz_channel(context, sub_uri, ptzRef, ptzDup, 0, content, content_max);
	}else if(prefix = "/CHANNEL/%d", 1 == sscanf(sub_uri, prefix, &id)){
		sub_uri += sprintf(content, prefix, id);
		if(id > 0){
			LP_JSON_OBJECT channelList = NETSDK_json_get_child(ptzDup, "ptzChannel");
			int const n_channelList = json_object_array_length(channelList);
			if(id - 1 < n_channelList){
				LP_JSON_OBJECT channel = json_object_array_get_idx(channelList, id - 1);
				LP_JSON_OBJECT channelExternalControl = NETSDK_json_get_child(channel, "externalControl");
				if(prefix = "/CONTROL", 0 == strncmp(sub_uri, prefix, strlen(prefix))){					
					if(H_IS_PUT(context->request_header->method) && !NSDK_PROPERTIES(sub_uri)){
						LP_HTTP_QUERY_PARA_LIST queryList = HTTP_UTIL_parse_query_as_para(context->request_header->query);
						if(NULL != queryList){
							const char *outputState = queryList->read(queryList, "outputState");
							if(NULL != outputState){
								APP_TRACE("outputState = %s", outputState);

								// FIXME: not completed
								ret = kNSDK_INS_RET_OK;
							}
							queryList->free(queryList);
							queryList = NULL;
							ret = kNSDK_INS_RET_OK;
						}
					}
				}else{
					ret = ptz_channel(context, sub_uri, ptzRef, ptzDup, id, content, content_max);
				}
			}
		}
	}

	// put referense
	json_object_put(ptzDup);
	ptzDup = NULL;
	json_object_put(jsonPTZ);
	jsonPTZ = NULL;
	return ret;
}

int NETSDK_ptz_instance(LP_HTTP_CONTEXT context, HTTP_CSTR_t sub_uri, char *content, int content_max)
{
	return ptz_instance(context, sub_uri, content, content_max);
}


/////////////////////////////////////////////////////////////////////////////////////////////////////
// Configuration
/////////////////////////////////////////////////////////////////////////////////////////////////////

/*
LP_NSDK_ALARM_IN_CH NETSDK_conf_ptz_ch_get(int id, LP_NSDK_ALARM_IN_CH in_ch)
{
	// FIXME:
	return NULL;
}

LP_NSDK_ALARM_IN_CH NETSDK_conf_ptz_ch_get(int id, LP_NSDK_ALARM_IN_CH in_ch)
{
	// FIXME:
	return NULL;
}


LP_NSDK_ALARM_OUT_CH NETSDK_conf_alarm_out_ch_get(int id, LP_NSDK_ALARM_OUT_CH out_ch)
{
	// FIXME:
	return NULL;
}

LP_NSDK_ALARM_OUT_CH NETSDK_conf_alarm_out_ch_set(int id, LP_NSDK_ALARM_OUT_CH out_ch)
{
	// FIXME:
	return NULL;
}
*/


