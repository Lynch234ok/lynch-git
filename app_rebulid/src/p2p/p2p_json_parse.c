#include "p2p_json_parse.h"
#include "netsdk.h"
#include "netsdk_util.h"
#include "sdk/sdk_api.h"

#include "app_debug.h"
#include "netsdk_private.h"
#include "app_msg_push.h"
#include "usrm.h"
#include "_base64.h"
#include "urlXxcode.h"
#include <sys/statvfs.h>
#include <dirent.h>
#include "generic.h"
#include <stdarg.h>
#include <base/ja_process.h>
#include <bsp/rtc.h>
#include "schedule_parse.h"
#include "tfcard.h"
#include "fisheye.h"
#include "custom.h"
#include "led_pwm.h"
#include "../netsdk.h"
#include "../netsdk_private.h"
#include "global_runtime.h"
#include "http_auth/_md5.h"
#include <secure_chip.h>
#include <NkEmbedded/mem_allocator.h>
#include "include/aes.h"
#include "capability_set.h"

typedef enum {
    TOTAL_SIZE,///ÎÄ¼þÏµÍ³µÄ´óÐ¡
    FREE_SIZE, ///×ÔÓÉ¿Õ¼ä
    USED_SIZE, ///ÒÑÓÃ¿Õ¼ä
    AVAIL_SIZE ///ÓÃ»§Êµ¼Ê¿ÉÒÔÊ¹ÓÃµÄ¿Õ¼ä
}VFsize;

bool p2p_setup_is_get(void *request)
{
	
	bool ret = true;
	char text[32];
	LP_JSON_OBJECT obj; 
	obj = NETSDK_json_parse(request);

	memset(text, 0, sizeof(text));
	if(NULL != obj){
		NETSDK_json_get_string(obj, "Method", text, sizeof(text));
		if(!strcmp(text, "get")){
			//set
			ret = true;
		}else{
			//get
			ret = false;
		}

		json_object_put(obj);
	}
	return ret;
}

static NK_SSize aesDecode(char* crypto, int len, NK_PByte aesKey, int aesKeyLen, char* origin)
{
	NK_Byte mem[1024] = "\0";

	NK_SSize deSize = 0;
	NK_AES *AES = NK_Nil;

	if((crypto == NK_Nil) || (aesKey == NK_Nil) || (origin == NK_Nil)) {
		APP_TRACE("aesDecode fail");
		return -1;
	}

	NK_Allocator *Alloctr = NK_Alloc_Create(mem, sizeof(mem));

	if(NK_Nil != Alloctr) {
		AES = NK_AES_Create(Alloctr, aesKey, aesKeyLen);
		if(NK_Nil != AES) {
			deSize = AES->decrypt(AES, (NK_PByte)crypto, len, (NK_PByte)origin);

			NK_AES_Free(&AES);
		}
		NK_Alloc_Free(&Alloctr, NK_Nil);
	}

	return deSize;

}

static int p2p_parse_userpwd_varify(char *verify_str, char *user_name, char *passwd)
{
	if(NULL == verify_str || (0 == strlen(verify_str))){
		return -1;
	}
	//base64 decode
	size_t verify_str_len = strlen(verify_str);
	char *base64_decbuf = (char*)calloc(2*verify_str_len, 1);	
	BASE64_decode(verify_str, strlen(verify_str), base64_decbuf, 2*verify_str_len);

	if(base64_decbuf)
	{
		char *tmp = NULL;
		tmp = strstr(base64_decbuf, "&&");
		if(tmp){
			sprintf(passwd, tmp+2);
			memset(tmp, 0, 1);
		}
		if(user_name){
			tmp = strstr(base64_decbuf, "&");
			if(tmp)
			{
				sprintf(user_name, tmp+1);
			}
		}
	}
	if(base64_decbuf)
	{
		free(base64_decbuf);
		base64_decbuf = NULL;
	}
	return 0;
}

static int p2p_parse_userpwd_varify2(char *verify_str, char *user_name, char *passwd)
{
	NK_Byte video_share_key[] = {0xFF, 0x9A, 0x12, 0x34, 0xC2, 0xAA, 0x55, 0x3D,0xB4, 0x5C,
	0x83, 0xD2, 0xA9, 0xFF, 0x07, 0x4F};

	char aes_outbuf[256] = "\0";
	NK_SSize size = 0;

	if(NULL == verify_str || (0 == strlen(verify_str))){
		return -1;
	}

	//base64 decode
	size_t verify_str_len = strlen(verify_str);
	char *base64_decbuf = (char*)calloc(2*verify_str_len, 1);
	int decode_len = BASE64_decode(verify_str, strlen(verify_str), base64_decbuf, 2*verify_str_len);
	size = aesDecode(base64_decbuf, decode_len, video_share_key, sizeof(video_share_key), aes_outbuf);
	printf("Decode Res::%s\n", aes_outbuf);

	if(size > 0)
	{
		char *save_str = NULL;
		char *tmp = NULL;

		tmp = strtok_r(aes_outbuf, "&", &save_str);
		tmp = strtok_r(NULL, "&", &save_str);
		sprintf(user_name, "%s", (tmp != NULL) ? tmp : "");
		tmp = strtok_r(NULL, "&", &save_str);
		sprintf(passwd, "%s", (tmp != NULL) ? tmp : "");

		APP_TRACE("user_name:%s passwd:%s\n", user_name , passwd);
	}
	else {
		APP_TRACE("Verify decode failed(%s)", verify_str);
	}

	if(base64_decbuf)
	{
		free(base64_decbuf);
		base64_decbuf = NULL;
	}
	
	return 0;
}

bool p2p_Auth_Verify_pass(void *request)
{
	char varify_text[255]={0};
	char username[25]={0};
	char password[25]={0};
	LP_JSON_OBJECT obj, Auth_user=NULL; 
	obj = NETSDK_json_parse(request);

	if(NULL != obj){		
		Auth_user = NETSDK_json_get_child(obj, "Authorization");
		if(Auth_user){
			NETSDK_json_get_string(Auth_user, "Verify", varify_text, sizeof(varify_text));

			if(strlen(varify_text) == 0){
				//verify
				NETSDK_json_get_string(Auth_user, "username", username, sizeof(username));
				NETSDK_json_get_string(Auth_user, "password", password, sizeof(password));			
			}else{
				//verify
				p2p_parse_userpwd_varify2(varify_text, username, password);
			}
		}
	}

	json_object_put(obj);
	if(username || (0 != strlen(username))){		
		if(USRM_GREAT == USRM_check_user(username, password)){
			return true;
		}
	}
	return false;
}

bool p2p_Auth_userpwd_set(char *username, char *oldpwd, char *newpasswd)
{
	USRM_I_KNOW_U_t* i_m = NULL;
	bool set_success = false;
	
	if(NULL == username || (0 == strlen(username))){
		return false;
	}
	//set
	// user check in
	i_m = USRM_login(username, oldpwd);
	if(i_m){
		USRM_HOW_ABOUT_t how_about = USRM_GREAT;		
		how_about = i_m->set_password(i_m, oldpwd, newpasswd);
		if(USRM_GREAT == how_about){
			APP_TRACE("Set user \"%s\" password success!", strdupa(username));
			USRM_store();
			set_success = true;
		}
		// check out
		USRM_logout(i_m);
		i_m = NULL;
		return set_success;
	}
	return false;
}


int mtion_setsensitivityLevel(ST_NSDK_MD_CH *md_ch, char *sensitivityLevel)
{
#ifdef P6
    if(0 == strcmp(sensitivityLevel, "lowest"))
    {
        md_ch->detectionGrid.sensitivityLevel = 60;
    }
    else if(0 == strcmp(sensitivityLevel, "low"))
    {
        md_ch->detectionGrid.sensitivityLevel = 70;
    }
    else if(0 == strcmp(sensitivityLevel, "normal"))
    {
        md_ch->detectionGrid.sensitivityLevel = 80;
    }
    else if(0 == strcmp(sensitivityLevel, "high"))
    {
        md_ch->detectionGrid.sensitivityLevel = 90;
    }
    else if(0 == strcmp(sensitivityLevel, "highest"))
    {
        md_ch->detectionGrid.sensitivityLevel = 95;
    }
#else
	if(0==strcmp(sensitivityLevel, "lowest"))
	{
		md_ch->detectionGrid.sensitivityLevel = 60;
	}
		else if(0==strcmp(sensitivityLevel, "low"))
	{
		md_ch->detectionGrid.sensitivityLevel = 80;
	}
	else if(0==strcmp(sensitivityLevel, "normal"))
	{
		md_ch->detectionGrid.sensitivityLevel = 90;
	}
	else if(0==strcmp(sensitivityLevel, "high"))
	{
		md_ch->detectionGrid.sensitivityLevel = 95;
	}
	else if(0==strcmp(sensitivityLevel, "highest"))
	{
		md_ch->detectionGrid.sensitivityLevel = 98;
	}
#endif
	 NETSDK_conf_md_ch_set(1, md_ch);
}

int set_motinfo(LP_JSON_OBJECT motinfo, int sensitivityLevel)
{
	 int ret = -1;
#ifdef P6
    if(sensitivityLevel>=0 && sensitivityLevel <= 60)
    {
        ret = NETSDK_json_set_string2(motinfo, "SensitivityLevel", "lowest");
    }
    else if(sensitivityLevel > 60 && sensitivityLevel <= 70)
    {
        ret = NETSDK_json_set_string2(motinfo, "SensitivityLevel", "low");
    }
    else if(sensitivityLevel > 70 && sensitivityLevel <= 80)
    {
        ret = NETSDK_json_set_string2(motinfo, "SensitivityLevel", "normal");
    }
    else if(sensitivityLevel > 80 && sensitivityLevel <= 90)
    {
        ret = NETSDK_json_set_string2(motinfo, "SensitivityLevel", "high");
    }
    else if(sensitivityLevel > 90 && sensitivityLevel <= 100)
    {
        ret = NETSDK_json_set_string2(motinfo, "SensitivityLevel", "highest");
    }
#else
	 if(sensitivityLevel>=0 && sensitivityLevel <= 60)
	 {
	 	ret = NETSDK_json_set_string2(motinfo, "SensitivityLevel", "lowest");
	 }
	 else if(sensitivityLevel > 60 && sensitivityLevel <= 80)
	 {
		 ret = NETSDK_json_set_string2(motinfo, "SensitivityLevel", "low");
	 }
	 else if(sensitivityLevel > 80 && sensitivityLevel <= 90)
	 {
		 ret = NETSDK_json_set_string2(motinfo, "SensitivityLevel", "normal");					 	
	 }
	 else if(sensitivityLevel > 90 && sensitivityLevel <= 95)
	 {
		 ret = NETSDK_json_set_string2(motinfo, "SensitivityLevel", "high");
	 }
	 else if(sensitivityLevel > 95 && sensitivityLevel <= 100)
	 {
		 ret = NETSDK_json_set_string2(motinfo, "SensitivityLevel", "highest");
	 }
#endif
	 return ret;
}

static LP_JSON_OBJECT p2p_video_find_channel(LP_JSON_OBJECT channels, int id)
{
	int i = 0;
	int const n_channels = json_object_array_length(channels);
	// one channel
	for(i = 0; i < n_channels; ++i){
		LP_JSON_OBJECT channel = json_object_array_get_idx(channels, i);
		if(json_object_get_int(json_object_object_get(channel, "id")) == id){
			return channel;
		}
	}
	return NULL;
}

static NK_Int int_to_string(NK_PChar buf, NK_Int num, NK_Size bufLen)
{
    snprintf(buf,bufLen,"%d",num);
}

static int p2pParseCapabilitysetGet(LP_JSON_OBJECT obj)
{
    stCAPABILITY_SET capabilitySet;
    EM_NSDK_IMAGE_FISHEYE_FIX_MODE fixMode = eNSDK_IMAGE_FISHEYE_FIX_MODE_NONE;
    if(NULL != obj)
    {
        if(0 == CAPABILITY_SET_get(&capabilitySet))
        {
            NETSDK_json_set_int2(obj, "version", capabilitySet.version);
            NETSDK_json_set_boolean2(obj, "audioInput", capabilitySet.audioInput);
            NETSDK_json_set_boolean2(obj, "audioOutput", capabilitySet.audioOutput);
            NETSDK_json_set_boolean2(obj, "ptz", false);

            // ä¸´æ—¶å¤„ç†ï¼Œä»¥ä¸‹é€»è¾‘åŽé¢éƒ½ä¼šåˆ é™¤
            if(true == network_check_interface())
            {
                NETSDK_json_set_boolean2(obj, "rj45", true);
            }
            else
            {
                NETSDK_json_set_boolean2(obj, "rj45", false);
            }

            fixMode = FISHEYE_get_fix_mode();
            if(eNSDK_IMAGE_FISHEYE_FIX_MODE_WALL == fixMode)
            {
                NETSDK_json_set_int2(obj, "fisheye", 180);
            }
            else if(eNSDK_IMAGE_FISHEYE_FIX_MODE_CELL == fixMode)
            {
                NETSDK_json_set_int2(obj, "fisheye", 360);
            }
            else if(eNSDK_IMAGE_FISHEYE_FIX_MODE_TABLE == fixMode)
            {
                NETSDK_json_set_int2(obj, "fisheye", 720);
            }
            else
            {
                NETSDK_json_set_int2(obj, "fisheye", 0);
            }

            return 0;
        }
        else
        {
            return -1;
        }

    }

    return -1;

}

ssize_t p2p_parse(void *request, void *response)
{
	const ST_NSDK_MAP_STR_DEC promptSoundType_map[] = {
		{"chinese", kNSDK_SYSTEM_PROMPT_SOUND_TYPE_CHINESE},
		{"english", kNSDK_SYSTEM_PROMPT_SOUND_TYPE_ENGLISH},
		{"german", kNSDK_SYSTEM_PROMPT_SOUND_TYPE_GERMAN},
		{"korean", kNSDK_SYSTEM_PROMPT_SOUND_TYPE_KOREAN},
		{"portuguese", kNSDK_SYSTEM_PROMPT_SOUND_TYPE_PORTUGUESE},
		{"russian", kNSDK_SYSTEM_PROMPT_SOUND_TYPE_RUSSIAN},
		{"spanish", kNSDK_SYSTEM_PROMPT_SOUND_TYPE_SPANISH}
	};

	LP_JSON_OBJECT obj; 
			
	obj = NETSDK_json_parse(request);
	if(NULL != obj){
		//userÐ£Ñé
		if(!p2p_Auth_Verify_pass(request))
		{
			json_object_put(obj);
			return -1;
		}
		
		if(p2p_setup_is_get(request)){
			//get
			LP_JSON_OBJECT ipcam=NULL, devInfo=NULL, modeSetinfo=NULL, fisheyeSetinfo = NULL, Psound = NULL, Tfcard = NULL, alarmSetinfo=NULL, motinfo=NULL, push_msg=NULL, SystemOper=NULL, timeS=NULL, upginfo=NULL;			
            LP_JSON_OBJECT ledPwm = NULL, wirelessManager = NULL;
            LP_JSON_OBJECT optTmp = NULL;
            LP_JSON_OBJECT tmpJson = NULL;
            LP_JSON_OBJECT capabilitySet = NULL;
            ST_NSDK_VIN_CH vin_ch;

            /* èƒ½åŠ›é›†èŽ·å– */
            capabilitySet = NETSDK_json_get_child(obj, "CapabilitySet");
            if(NULL != capabilitySet)
            {
                p2pParseCapabilitysetGet(capabilitySet);
            }

			ipcam = NETSDK_json_get_child(obj, "IPCam");
			if(NULL!=ipcam){
				//DeviceInfo
				devInfo = NETSDK_json_get_child(ipcam, "DeviceInfo");
				
				if(NULL!=devInfo){
					ST_NSDK_SYSTEM_DEVICE_INFO sysInfo;
					NETSDK_conf_system_get_device_info(&sysInfo);
					int m = 0, count=0;
					int n = strlen(sysInfo.firmwareVersion);
					//OEM number ----56112100
					char *oemstr = strrchr(sysInfo.firmwareVersion, '.');
					ST_CUSTOM_SETTING custom;
					if(0 == CUSTOM_get(&custom) && CUSTOM_check_string_valid(custom.model.oemNumber)){
						NETSDK_json_set_string2(devInfo, "OEMNumber", custom.model.oemNumber);
						APP_TRACE("SET OEM number:%s", custom.model.oemNumber);
					}else if(oemstr){
						char OEMNum[7];
						memset(OEMNum, 0, sizeof(OEMNum));
						memcpy(OEMNum, oemstr+1, 6);
						NETSDK_json_set_string2(devInfo, "OEMNumber", OEMNum);
					}

					while(m<n)
					{
						if(sysInfo.firmwareVersion[m] == '.')
						{
							count++;
							if(count == 3)
								break;
						}
						m++;
					}
					memset(sysInfo.firmwareVersion+m, 0, n-m);
					//1.4.2-----.56112100
					//memset(sysInfo.firmwareVersion+5, 0, sizeof(sysInfo.firmwareVersion)-5); 
					char version_tmp[64];
					snprintf(version_tmp, sizeof(version_tmp), "%s.0", sysInfo.firmwareVersion);
					NETSDK_json_set_string2(devInfo, "FWVersion", version_tmp);

					NETSDK_json_set_string2(devInfo, "Model", sysInfo.model);
					NETSDK_json_set_string2(devInfo, "ID", sysInfo.serialNumber);
					NETSDK_json_set_string2(devInfo, "FWMagic", "SlVBTiBJUENBTSBGSVJNV0FSRSBERVNJR05FRCBCWSBMQVc=");
				}

				//ModeSetting
				modeSetinfo = NETSDK_json_get_child(ipcam, "ModeSetting");
				if(modeSetinfo != NULL) {
                    ST_NSDK_AENC_CH aenc_ch;
                    ST_NSDK_IMAGE image;
                    char text[20]={0};

                    NETSDK_conf_aenc_ch_get(101, &aenc_ch);
                    NETSDK_json_set_boolean2(modeSetinfo,"AudioEnabled", aenc_ch.enabled);

                    if(NETSDK_conf_image_get(&image)){
                        if(0 ==  image.sceneMode)
                        {
                            NETSDK_json_set_string2(modeSetinfo, "SceneMode", "auto");
                        }
                        if(1 ==  image.sceneMode)
                        {
                            NETSDK_json_set_string2(modeSetinfo, "SceneMode", "indoor");
                        }
                        if(2 ==  image.sceneMode)
                        {
                            NETSDK_json_set_string2(modeSetinfo, "SceneMode", "outdoor");
                        }
                        
                        //irCutMode
                        // åŒºåˆ†Cxå•å“ è‡ªåŠ¨|ç™½å¤©|å¤œæ™š
                        // æ™®é€š çº¢å¤–æ¨¡å¼|ç…§æ˜Žæ¨¡å¼|æ™ºèƒ½æ¨¡å¼
                        if(true == GLOBAL_sn_front()) {
                            if(kNSDK_IMAGE_IRCUT_MODE_AUTO == image.irCutFilter.irCutMode)
                            {
                                NETSDK_json_set_string2(modeSetinfo, "IRCutFilterMode", "auto");
                            }
                            if(kNSDK_IMAGE_IRCUT_MODE_DAYLIGHT == image.irCutFilter.irCutMode)
                            {
                                NETSDK_json_set_string2(modeSetinfo, "IRCutFilterMode", "daylight");
                            }
                            if(kNSDK_IMAGE_IRCUT_MODE_NIGHT == image.irCutFilter.irCutMode)
                            {
                                NETSDK_json_set_string2(modeSetinfo, "IRCutFilterMode", "night");
                            }
                            optTmp = json_object_new_object();
                            tmpJson = NETSDK_json_parse("[ \"auto\", \"daylight\", \"night\"]");
                            json_object_object_add(optTmp, "opt", tmpJson);
                            json_object_object_add(modeSetinfo, "IRCutFilterModeProperty", optTmp);
                        }
                        else {
                            if(kNSDK_IMAGE_IRCUT_MODE_AUTO == image.irCutFilter.irCutMode)
                            {
                                NETSDK_json_set_string2(modeSetinfo, "IRCutFilterMode", "ir");
                            }
                            if(kNSDK_IMAGE_IRCUT_MODE_LIGHTMODE == image.irCutFilter.irCutMode)
                            {
                                NETSDK_json_set_string2(modeSetinfo, "IRCutFilterMode", "light");
                            }
                            if(kNSDK_IMAGE_IRCUT_MODE_SMARTMODE == image.irCutFilter.irCutMode)
                            {
                                NETSDK_json_set_string2(modeSetinfo, "IRCutFilterMode", "smart");
                            }
                            optTmp = json_object_new_object();
                            tmpJson = NETSDK_json_parse("[ \"ir\", \"light\", \"smart\"]");
                            json_object_object_add(optTmp, "opt", tmpJson);
                            json_object_object_add(modeSetinfo, "IRCutFilterModeProperty", optTmp);
                        }
                    
                        char multi_conf_mode[16];
                        NETSDK_get_multi_conf_mode(multi_conf_mode);
                        //·µ»ØÉèÖÃÔÚ¼Ò Àë¼ÒÄ£Ê½
                        NETSDK_json_set_string2(modeSetinfo, "ConvenientSetting", multi_conf_mode);
                    
                        //image style
                        if(1 == image.imageStyle){
                            NETSDK_json_set_string2(modeSetinfo, "imageStyle", "standard");
                        }else if(2 == image.imageStyle){
                            NETSDK_json_set_string2(modeSetinfo, "imageStyle", "bright");
                        }else if(3 == image.imageStyle){
                            NETSDK_json_set_string2(modeSetinfo, "imageStyle", "gorgeous");
                        }
                        
                    }
                    
                    //audio volume setting
                    LP_JSON_OBJECT audioVolumeSetting = NETSDK_json_get_child(modeSetinfo, "AudioVolume");
                    if(audioVolumeSetting){
                        ST_NSDK_AIN_CH ain;
                        NETSDK_conf_ain_ch_get(1, &ain);
                        NETSDK_json_set_int(audioVolumeSetting, "AudioInputVolume", ain.inputVolume);
                        NETSDK_json_set_int(audioVolumeSetting, "AudioOutputVolume", ain.outputVolume);
                    }
                    
                    //NETSDK_json_set_string2(modeSetinfo, "Definition", "auto");
                    //definition type
                    LP_JSON_OBJECT videoJSON = json_object_get(netsdk->video_conf);
                    LP_JSON_OBJECT channelListJSON = NETSDK_json_get_child(videoJSON, "videoEncode.videoEncodeChannel");
                    LP_JSON_OBJECT channel = p2p_video_find_channel(channelListJSON, 101);
                    char definitionType[16]={0};
                    
                    NETSDK_json_get_string(channel, "definitionType", definitionType, sizeof(definitionType));
                    //venc_ch->definitionType= NETSDK_MAP_STR2DEC(definition_type, definitionType, kNSDK_DEFINITION_AUTO);
                    NETSDK_json_set_string2(modeSetinfo, "Definition", definitionType);
                    if(NULL != videoJSON) {
                        json_object_put(videoJSON);
                        videoJSON = NULL;
                    }

                    // powerLineFrequencyMode
                    NETSDK_conf_vin_ch_get(1, &vin_ch);
                    NETSDK_json_set_int2(modeSetinfo, "powerLineFrequencyMode", vin_ch.powerLineFrequencyMode);
                    optTmp = json_object_new_array();
                    if(NULL != optTmp) {
                        tmpJson = json_object_new_int(50);
                        json_object_array_add(optTmp, tmpJson);
                        tmpJson = json_object_new_int(60);
                        json_object_array_add(optTmp, tmpJson);
                        tmpJson = json_object_new_object();
                        if(NULL != tmpJson) {
                            json_object_object_add(tmpJson, "opt", optTmp);
                            json_object_object_add(modeSetinfo, "powerLineFrequencyModeProperty", tmpJson);
                        }
                    }
                }
				
				//AlarmSetting
				alarmSetinfo = NETSDK_json_get_child(ipcam, "AlarmSetting");
                if(alarmSetinfo != NULL) {
                    //MotionDetection
                    motinfo = NETSDK_json_get_child(alarmSetinfo, "MotionDetection");
                    if(motinfo != NULL) {
                        ST_NSDK_MD_CH md_ch;
                        ST_NSDK_SYSTEM_SETTING sysInfo;
                        if(NETSDK_conf_md_ch_get(1, &md_ch)){
                            NETSDK_json_set_boolean2(motinfo, "Enabled", md_ch.enabled);
                            set_motinfo(motinfo, md_ch.detectionGrid.sensitivityLevel);
                        }
                        if(NETSDK_conf_system_get_setting_info(&sysInfo)) {
                            NETSDK_json_set_boolean2(motinfo, "MotionWarningTone", sysInfo.mdAlarm.MotionWarningTone);
                        }
                    }
                    
                    //MessagePushEnabled
                    ST_NSDK_SYSTEM_SETTING sinfo;
                    NETSDK_conf_system_get_setting_info(&sinfo);
                    NETSDK_json_set_boolean2(alarmSetinfo, "MessagePushEnabled", sinfo.messagePushEnabled);
                    //MessagePushSchedule
                    LP_JSON_OBJECT  Schedule= NETSDK_json_get_child(alarmSetinfo, "MessagePushSchedule");
                    
                    if(Schedule){
                        int i;
                        for (i = 0
                                ; i < sizeof(sinfo.AlarmNotification.Schedule)
                                    / sizeof(sinfo.AlarmNotification.Schedule[0]);
                                ++i)
                        {
                            LP_JSON_OBJECT Scheduletime = json_object_array_get_idx(Schedule, i);
                            if(sinfo.AlarmNotification.Schedule[i].enabled)
                            {
                                char beginTime[64]={0};
                                char endTime[64]={0};
                                char weekday[64]={0};
                                
                                schedule_time_to_string(sinfo.AlarmNotification.Schedule[i].BeginTime.hour
                                        , sinfo.AlarmNotification.Schedule[i].BeginTime.min
                                        , sinfo.AlarmNotification.Schedule[i].BeginTime.sec
                                        , beginTime, sizeof(beginTime));
                        
                                schedule_time_to_string(sinfo.AlarmNotification.Schedule[i].EndTime.hour
                                        , sinfo.AlarmNotification.Schedule[i].EndTime.min
                                        , sinfo.AlarmNotification.Schedule[i].EndTime.sec
                                        , endTime, sizeof(endTime));
                        
                                schedule_weekday_to_string(sinfo.AlarmNotification.Schedule[i].weekday
                                        , weekday
                                        , sizeof(weekday));
                                if(!Scheduletime){
                                    Scheduletime = json_object_new_object();
                                    NETSDK_json_set_string2(Scheduletime, "Weekday", weekday);
                                    NETSDK_json_set_string2(Scheduletime, "BeginTime", beginTime);
                                    NETSDK_json_set_string2(Scheduletime, "EndTime", endTime);
                                    //NETSDK_json_set_int(Scheduletime, "id", i);
                                    NETSDK_json_set_int2(Scheduletime, "id", i);
                                    json_object_array_put_idx(Schedule, i, Scheduletime);
                                }
                            }
                        }
                    }
                    NETSDK_json_set_boolean2(alarmSetinfo, "ScheduleSupport", true);
                }
				

				//SystemOperation
				SystemOper = NETSDK_json_get_child(ipcam, "SystemOperation");
                if(SystemOper != NULL) {
                    //TimeSync
                    time_t utc_time;
                    char utc_time_str[15] = {0};
					LP_JSON_OBJECT UTCTime = NULL, TimeZone = NULL;
					ST_NSDK_SYSTEM_TIME sys_time;

                    timeS = NETSDK_json_get_child(SystemOper, "TimeSync");
					if (NULL != timeS) {
						utc_time = time(NULL);
						snprintf(utc_time_str, sizeof(utc_time_str), "%ld", utc_time);
						NETSDK_json_set_string(timeS, "UTCTime", utc_time_str);

						NETSDK_conf_system_get_time(&sys_time);
						NETSDK_json_set_int(timeS, "TimeZone", sys_time.greenwichMeanTime);
					}
                    
                    //if(timeS)
                    //  NETSDK_json_set_string2(timeS, "LocalTime", mtime);
                    
                    //Upgrade
                    upginfo = NETSDK_json_get_child(SystemOper, "Upgrade");

					LP_JSON_OBJECT dstJSON = NETSDK_json_get_child(SystemOper, "DaylightSavingTime");
					if(dstJSON != NULL){
						int i, n;
						LP_JSON_OBJECT weekJSON = NULL, tmpWeekJSON = NULL;;
						ST_NSDK_SYSTEM_DST dstInfo = {0};

						NETSDK_conf_system_get_DST_info(&dstInfo);
						NETSDK_json_set_boolean2(dstJSON, "Enabled", dstInfo.enable);
                        NETSDK_json_set_string2(dstJSON, "Country", dstInfo.country);
						NETSDK_json_set_int2(dstJSON, "Offset", dstInfo.offset);
						if((weekJSON = NETSDK_json_get_child(dstJSON, "Week")) != NULL){
							n = json_object_array_length(weekJSON);
							for(i = 0; i < n && i < sizeof(dstInfo.week) / sizeof(dstInfo.week[0]); i++){
								if((tmpWeekJSON = json_object_array_get_idx(weekJSON, i)) != NULL){
									NETSDK_json_set_string2(tmpWeekJSON, "Type", dstInfo.week[i].type);
									NETSDK_json_set_int2(tmpWeekJSON, "Month", dstInfo.week[i].month);
									NETSDK_json_set_int2(tmpWeekJSON, "Week", dstInfo.week[i].week);
									NETSDK_json_set_int2(tmpWeekJSON, "Weekday", dstInfo.week[i].weekday);
									NETSDK_json_set_int2(tmpWeekJSON, "Hour", dstInfo.week[i].hour);
									NETSDK_json_set_int2(tmpWeekJSON, "Minute", dstInfo.week[i].minute);
								}
							}
						}
					}
                }

				//PromptSounds
				ST_NSDK_SYSTEM_SETTING pinfo;				
				Psound = NETSDK_json_get_child(ipcam, "PromptSounds");
				if(Psound){
					NETSDK_conf_system_get_setting_info(&pinfo);
					NETSDK_json_set_boolean2(Psound, "Enabled", pinfo.promptSound.enabled);
					/*if(pinfo.promptSound.soundType == 0){
						NETSDK_json_set_string2(Psound, "Type", "chinese");
					}
					else if(pinfo.promptSound.soundType == 1){
						NETSDK_json_set_string2(Psound, "Type", "english");
					}*/
					NETSDK_json_set_string2(Psound, "Type", pinfo.promptSound.soundTypeStr);
					LP_JSON_OBJECT optionFromJSON = NETSDK_json_parse(pinfo.promptSound.soundTypeOpt);
					json_object_object_add(Psound, "TypeOption", optionFromJSON);
				}

				Tfcard = NETSDK_json_get_child(ipcam, "TfcardManager");
				if(Tfcard){
					unsigned int spacesize=0;
					unsigned int totalsize=0;
					int flag = 0;
					unsigned char spacestr[10]={0};
					unsigned char totalspace[10]={0};

#if defined(TFCARD)
					char tfcard_status[32];
					int status;
					status = NK_TFCARD_get_status(tfcard_status);
					if(emTFCARD_STATUS_OK == status){
						totalsize = NK_TFCARD_get_capacity();
						snprintf(totalspace, sizeof(totalspace), "%d", totalsize);
						if(0 != strlen(totalspace)){
							NETSDK_json_set_string2(Tfcard, "TotalSpacesize", totalspace);
						}
						
						spacesize = NK_TFCARD_get_freespace();
						snprintf(spacestr, sizeof(spacestr), "%d", spacesize);
						if(0 != strlen(spacestr)){
							NETSDK_json_set_string2(Tfcard, "LeaveSpacesize", spacestr);
						}
					}
					NETSDK_json_set_string2(Tfcard, "Status", tfcard_status);
#endif

					ST_NSDK_SYSTEM_SETTING minfo = {0};
					NETSDK_conf_system_get_setting_info(&minfo);
                    NETSDK_json_set_boolean2(Tfcard, "TimeRecordEnabled", minfo.timeRecordEnabled);
					LP_JSON_OBJECT recordSchedule = NETSDK_json_get_child(Tfcard,"TFcard_recordSchedule");
					if(recordSchedule){
						int i;
						for(i = 0
								; i < sizeof(minfo.TFcard_Record.Schedule)
									/ sizeof(minfo.TFcard_Record.Schedule[0]);
								i++)
						{
							LP_JSON_OBJECT recordScheduleTime = json_object_array_get_idx(recordSchedule,i);
							if(minfo.TFcard_Record.Schedule[i].enabled)
							{
								char beginTime[64] = {0};
								char endTime[64] = {0};
								char weekday[64] = {0};

								schedule_time_to_string(minfo.TFcard_Record.Schedule[i].BeginTime.hour
									, minfo.TFcard_Record.Schedule[i].BeginTime.min
									, minfo.TFcard_Record.Schedule[i].BeginTime.sec
									, beginTime, sizeof(beginTime));
								schedule_time_to_string(minfo.TFcard_Record.Schedule[i].EndTime.hour
									, minfo.TFcard_Record.Schedule[i].EndTime.min
									, minfo.TFcard_Record.Schedule[i].EndTime.sec
									, endTime, sizeof(endTime));
								schedule_weekday_to_string(minfo.TFcard_Record.Schedule[i].weekday
									, weekday
									, sizeof(weekday));
								if(!recordScheduleTime){
									recordScheduleTime = json_object_new_object();
									NETSDK_json_set_string2(recordScheduleTime, "Weekday", weekday);
									NETSDK_json_set_string2(recordScheduleTime, "BeginTime", beginTime);
									NETSDK_json_set_string2(recordScheduleTime, "EndTime", endTime);
									NETSDK_json_set_int2(recordScheduleTime, "id", i);
									json_object_array_put_idx(recordSchedule, i, recordScheduleTime);
								}
							}
						}
					}
					NETSDK_json_set_boolean2(Tfcard, "ScheduleSupport", true);
				}

				//FisheyeSetting
				fisheyeSetinfo = NETSDK_json_get_child(ipcam, "FisheyeSetting");
				if(fisheyeSetinfo){
					stFISHEYE_config conf = {0};
					int tmpFixMode;
					FISHEYE_config_get(&conf);

                    NETSDK_json_set_string2(fisheyeSetinfo, "LensName", conf.lensName);

					tmpFixMode = FISHEYE_get_fix_mode();
					if(eNSDK_IMAGE_FISHEYE_FIX_MODE_WALL == tmpFixMode){
						NETSDK_json_set_string2(fisheyeSetinfo,"FixMode","wall");
					}else if(eNSDK_IMAGE_FISHEYE_FIX_MODE_CELL == tmpFixMode){
						NETSDK_json_set_string2(fisheyeSetinfo,"FixMode","cell");
					}else if(eNSDK_IMAGE_FISHEYE_FIX_MODE_TABLE == tmpFixMode){
						NETSDK_json_set_string2(fisheyeSetinfo,"FixMode","table");
					}else{
						NETSDK_json_set_string2(fisheyeSetinfo,"FixMode","none");
					}

					LP_JSON_OBJECT fixParam = NETSDK_json_get_child(fisheyeSetinfo,"FixParam");
					if(fixParam){
						int i = 0;
						for(i = 0 ; i < (sizeof(conf.param) / sizeof(conf.param[0])) ; i++)
						{
							LP_JSON_OBJECT fixParamData = json_object_array_get_idx(fixParam,i);

							if(!fixParamData){
								fixParamData = json_object_new_object();
                                if(conf.type) {
                                    NETSDK_json_set_int2(fixParamData, "CenterCoordinateX", conf.param[i].CenterCoordinateX);
                                    NETSDK_json_set_int2(fixParamData, "CenterCoordinateY", conf.param[i].CenterCoordinateY);
                                    NETSDK_json_set_int2(fixParamData, "Radius", conf.param[i].Radius);
                                    NETSDK_json_set_int2(fixParamData, "AngleX", conf.param[i].AngleX);
                                    NETSDK_json_set_int2(fixParamData, "AngleY", conf.param[i].AngleY);
                                    NETSDK_json_set_int2(fixParamData, "AngleZ", conf.param[i].AngleZ);
                                }
                                else {
                                    NETSDK_json_set_float2(fixParamData, "CenterCoordinateX", conf.param2[i].CenterCoordinateX);
                                    NETSDK_json_set_float2(fixParamData, "CenterCoordinateY", conf.param2[i].CenterCoordinateY);
                                    NETSDK_json_set_float2(fixParamData, "Radius", conf.param2[i].Radius);
                                    NETSDK_json_set_float2(fixParamData, "AngleX", conf.param2[i].AngleX);
                                    NETSDK_json_set_float2(fixParamData, "AngleY", conf.param2[i].AngleY);
                                    NETSDK_json_set_float2(fixParamData, "AngleZ", conf.param2[i].AngleZ);
                                }
								NETSDK_json_set_int2(fixParamData, "id", i);
								json_object_array_put_idx(fixParam, i, fixParamData);
							}
						}
					}
				}

#if defined (LED_PWM)
                ledPwm = NETSDK_json_get_child(ipcam, "ledPwm");
                if(ledPwm == NULL) {
                    if(LED_PWM_is_pwm() == 0) {
                        int i = 0;
                        stLED_PWM_config ledPwmConfig;
                        LP_JSON_OBJECT child_json = NULL;

                        LED_PWM_get(&ledPwmConfig);
                        ledPwm = json_object_new_object();
                        if(ledPwm) {
                            NETSDK_json_set_int2(ledPwm, "channelCount", ledPwmConfig.channelCount);
                            NETSDK_json_set_int2(ledPwm, "switch", ledPwmConfig.ledSwitch);
                            LP_JSON_OBJECT child_json = json_object_new_array();
                            if(child_json) {
                                for(i = 0; i < ledPwmConfig.channelCount; i++) {
                                    LP_JSON_OBJECT info = json_object_array_get_idx(child_json, i);
                                    if(info == NULL) {
                                        info = json_object_new_object();
                                        NETSDK_json_set_int2(info, "type", ledPwmConfig.array[i].type);
                                        NETSDK_json_set_int2(info, "num", ledPwmConfig.array[i].num);
                                        NETSDK_json_set_int2(info, "channel", ledPwmConfig.array[i].channel);
                                        json_object_array_put_idx(child_json, i, info);
                                    }
                                }
                                json_object_object_add(ledPwm, "channelInfo", child_json);
                                json_object_object_add(ipcam, "ledPwm", ledPwm);
                            }
                        }
                    }
                }
                else {
                    if(LED_PWM_is_pwm() == 0)
                    {
                        int i = 0;
                        stLED_PWM_config ledPwmConfig;
                        LP_JSON_OBJECT child_json = NULL;

                        LED_PWM_get(&ledPwmConfig);
                        NETSDK_json_set_int2(ledPwm, "channelCount", ledPwmConfig.channelCount);
                        NETSDK_json_set_int2(ledPwm, "switch", ledPwmConfig.ledSwitch);
                        child_json = NETSDK_json_get_child(ledPwm, "channelInfo");
                        if(child_json) {
                            for(i = 0; i < ledPwmConfig.channelCount; i++) {
                                LP_JSON_OBJECT info = json_object_array_get_idx(child_json, i);
                                if(info == NULL) {
                                    info = json_object_new_object();
                                    NETSDK_json_set_int2(info, "type", ledPwmConfig.array[i].type);
                                    NETSDK_json_set_int2(info, "num", ledPwmConfig.array[i].num);
                                    NETSDK_json_set_int2(info, "channel", ledPwmConfig.array[i].channel);
                                    json_object_array_put_idx(child_json, i, info);
                                }
                            }
                        }
                    }
                }
#endif
                wirelessManager = NETSDK_json_get_child(ipcam, "WirelessManager");
                if(wirelessManager != NULL) {
#if defined (CX)
                    if(false == GLOBAL_sn_front()) {
                        ST_NSDK_NETWORK_INTERFACE inter;
                        char base64ApPsk[128];
                        NETSDK_conf_interface_get(4, &inter);
                        if(inter.wireless.wirelessMode == NSDK_NETWORK_WIRELESS_MODE_ACCESSPOINT) {
                            memset(base64ApPsk, 0, sizeof(base64ApPsk));
                            BASE64_encode(inter.wireless.wirelessApMode.wirelessPsk, strlen(inter.wireless.wirelessApMode.wirelessPsk),
                                    base64ApPsk, sizeof(base64ApPsk));
                            NETSDK_json_set_string2(wirelessManager, "ApPsk", base64ApPsk);
                        }
                    }
#endif
                }
			}

            strcpy(response, json_object_to_json_string(obj));
            if(NULL != obj)
            {
                json_object_put(obj);
            }
            if(optTmp) {
                json_object_put(optTmp);
            }
            if(tmpJson) {
                json_object_put(tmpJson);
            }
            return strlen(response);
		}else{
				//set	
				LP_JSON_OBJECT ipcam=NULL, devInfo=NULL, modeSetinfo=NULL, fisheyeSetinfo = NULL, alarmSetinfo=NULL, Psound =NULL, Tfcard = NULL, motinfo=NULL, SystemOper=NULL, timeS=NULL, upginfo=NULL;
                LP_JSON_OBJECT ledPwm = NULL, wirelessManager = NULL;
                int ain = 0;
				ST_NSDK_AENC_CH aenc_ch;
				ST_NSDK_IMAGE image;
                ST_NSDK_VIN_CH vin_ch;
                int plFrequencyMode;

				ipcam = NETSDK_json_get_child(obj, "IPCam");
				if(NULL!=ipcam){
					//ModeSetting
					//AudioEnabled
					modeSetinfo = NETSDK_json_get_child(ipcam, "ModeSetting");

					if(NULL != modeSetinfo){
						char Convenientset[16];
                        ST_NSDK_SYSTEM_TIME sys_time;
                        bool got_time = true;
						if(NULL != NETSDK_json_get_string(modeSetinfo, "ConvenientSetting", Convenientset, sizeof(Convenientset))){
							if(0 == NETSDK_set_multi_conf_mode(Convenientset)){
                                if (NULL == NETSDK_conf_system_get_time(&sys_time)) {
                                    APP_TRACE("NETSDK_conf_system_get_time Failed!");
                                    got_time = false;
                                }

                                //setting mode sucess
								NETSDK_conf_load(true);
                                custom_conf_match();
                                 // sync time setting
                                if (got_time) {
                                    if (NULL == NETSDK_conf_system_set_time(&sys_time)) {
                                        APP_TRACE("NETSDK_conf_system_set_time Failed!");
                                    }
                                }
							}
							//json_object_put(obj);
							//sprintf((char *)response, "{\r\n\"option\" :\"success\"\r\n}\r\n");
							//return strlen((char *)response);
						}
					}

					if(NULL != modeSetinfo)
					{
						NETSDK_conf_aenc_ch_get(101, &aenc_ch);
                        if(NETSDK_json_check_child(modeSetinfo, "AudioEnabled")) {
    						if(NETSDK_json_get_boolean(modeSetinfo, "AudioEnabled"))
    						{
                                if(false == aenc_ch.enabled) {
                                    aenc_ch.enabled = true;

                                    NETSDK_conf_aenc_ch_set(101, &aenc_ch);

                                    if(sdk_enc)
                                    {
                                        sdk_enc->create_audio_stream(0, 0, aenc_ch.codecType);
                                    }
                                }
    						}
    						else
    						{
                                if(true == aenc_ch.enabled) {
                                    aenc_ch.enabled = false;
                                    NETSDK_conf_aenc_ch_set(101, &aenc_ch);
                                    if(sdk_enc){
                                        sdk_enc->release_stream_g711a(0);
                                    }
                                }
    						}
                        }
					}

					//ModeSetting
					ST_NSDK_IMAGE image;
					char *str = NULL;
					char s_mode[10]={0};
					bool image_setting_flag = false;
					char text[32] = {0};
					if(NETSDK_conf_image_get(&image))
					{
						if(NULL != modeSetinfo)
						{
							if(NULL != NETSDK_json_get_string(modeSetinfo, "SceneMode", s_mode, sizeof(s_mode))){

								if(0 == strcmp(s_mode, "auto"))
								{
									image.sceneMode = 0;
								}
								if(0 == strcmp(s_mode, "indoor"))
								{
									image.sceneMode = 1;
								}
								if(0 == strcmp(s_mode, "outdoor"))
								{
									image.sceneMode = 2;
								}
								image_setting_flag = true;
							}

							//irCutMode	
							if(NULL != NETSDK_json_get_string(modeSetinfo, "IRCutFilterMode", s_mode, sizeof(s_mode))){
                                // åŒºåˆ†Cxå•å“ è‡ªåŠ¨|ç™½å¤©|å¤œæ™š
                                // æ™®é€š çº¢å¤–æ¨¡å¼|ç…§æ˜Žæ¨¡å¼|æ™ºèƒ½æ¨¡å¼
                                if(true == GLOBAL_sn_front()) {
                                    if(0 == strcmp(s_mode, "auto"))
                                    {
                                        image.irCutFilter.irCutMode= kNSDK_IMAGE_IRCUT_MODE_AUTO;
                                    }
                                    else if(0 == strcmp(s_mode, "daylight"))
                                    {
                                        image.irCutFilter.irCutMode = kNSDK_IMAGE_IRCUT_MODE_DAYLIGHT;
                                    }
                                    else if(0 == strcmp(s_mode, "night"))
                                    {
                                        image.irCutFilter.irCutMode = kNSDK_IMAGE_IRCUT_MODE_NIGHT;
                                    }
                                }
                                else {
                                    if(0 == strcmp(s_mode, "ir"))
                                    {
                                        image.irCutFilter.irCutMode= kNSDK_IMAGE_IRCUT_MODE_AUTO;
                                    }
                                    else if(0 == strcmp(s_mode, "light"))
                                    {
                                        image.irCutFilter.irCutMode = kNSDK_IMAGE_IRCUT_MODE_LIGHTMODE;
                                    }
                                    else if(0 == strcmp(s_mode, "smart"))
                                    {
                                        image.irCutFilter.irCutMode = kNSDK_IMAGE_IRCUT_MODE_SMARTMODE;
                                    }
                                }
								image_setting_flag = true;
							}

							//imageStyle
							if(NULL != NETSDK_json_get_string(modeSetinfo, "imageStyle", text, sizeof(text))){
								if(!strcmp(text, "standard")){
									image.imageStyle = 1;
								}else if(!strcmp(text, "bright")){
									image.imageStyle = 2;
								}else if(!strcmp(text, "gorgeous")){
									image.imageStyle = 3;
								}
								image_setting_flag = true;
							}

							if(image_setting_flag){
								//need to be setting
								NETSDK_conf_image_set(&image);
								netsdk_image_changed(&image);
							}
						}
					}

					//audio volume setting
					bool audio_setting_flag = false;
					LP_JSON_OBJECT audioVolumeSetting = NETSDK_json_get_child(modeSetinfo, "AudioVolume");
					if(audioVolumeSetting){
						ST_NSDK_AIN_CH ain;
						NETSDK_conf_ain_ch_get(1, &ain);
						if(NETSDK_json_check_child(audioVolumeSetting, "AudioInputVolume")){
							ain.inputVolume = NETSDK_json_get_int(audioVolumeSetting, "AudioInputVolume");
							audio_setting_flag = true;
						}
						if(NETSDK_json_check_child(audioVolumeSetting, "AudioOutputVolume")){
							ain.outputVolume = NETSDK_json_get_int(audioVolumeSetting, "AudioOutputVolume");
							audio_setting_flag = true;
						}
						if(audio_setting_flag){
							NETSDK_conf_ain_ch_set(1, &ain);
							//BSP_Audio_set_volume_val(-1, ain.inputVolume, -1, ain.outputVolume);
						}
					}

					//DefinitionºöÂÔ
					//definition type
					if(modeSetinfo){
						const ST_NSDK_MAP_STR_DEC definition_type[] = {
							{"auto", kNSDK_DEFINITION_AUTO},
							{"fluency", kNSDK_DEFINITION_FLUENCY},
							{"BD", kNSDK_DEFINITION_BD},
							{"HD", kNSDK_DEFINITION_HD},
						};
						LP_JSON_OBJECT videoJSON = json_object_get(netsdk->video_conf);
						LP_JSON_OBJECT channelListJSON = NETSDK_json_get_child(videoJSON, "videoEncode.videoEncodeChannel");
						LP_JSON_OBJECT channel = p2p_video_find_channel(channelListJSON, 101);
						char definitionType[16];
						ST_NSDK_VENC_CH venc_ch;
						int tmpDefinitionType = 0;
						NETSDK_conf_venc_ch_get(101, &venc_ch);

						if(NULL != NETSDK_json_get_string(modeSetinfo, "Definition", definitionType, sizeof(definitionType))){
                            tmpDefinitionType = NETSDK_MAP_STR2DEC(definition_type, definitionType, 0);
                            if(tmpDefinitionType != venc_ch.definitionType) {
                                venc_ch.definitionType = tmpDefinitionType;
                                if(NULL != NETSDK_conf_venc_ch_set(101, &venc_ch)){
                                    netsdk_venc_ch_changed(101, &venc_ch);
                                }
                            }
						}
                        if(NULL != videoJSON) {
                            json_object_put(videoJSON);
                            videoJSON = NULL;
                        }

                        // powerLineFrequencyMode
                        if(NETSDK_json_check_child(modeSetinfo, "powerLineFrequencyMode")) {
                            ST_CUSTOM_SETTING custom;
                            NETSDK_conf_vin_ch_get(1, &vin_ch);
                            plFrequencyMode = NETSDK_json_get_int(modeSetinfo, "powerLineFrequencyMode");
                            vin_ch.powerLineFrequencyMode = plFrequencyMode;
                            if(0 == CUSTOM_get(&custom)) {
                                custom.function.powerLineFrequencyMode = plFrequencyMode;
                                CUSTOM_set(&custom);
                            }
                            if(netsdk->videoInputChannelChanged) {
                                netsdk->videoInputChannelChanged(1, &vin_ch);
                            }
                            NETSDK_conf_vin_ch_set(1, &vin_ch);
                        }
					}		

					//AlarmSetting
					alarmSetinfo = NETSDK_json_get_child(ipcam, "AlarmSetting");
                    char level[25];

					if(NULL != alarmSetinfo){						
						//MotionDetection
						motinfo = NETSDK_json_get_child(alarmSetinfo, "MotionDetection");						
						//MessagePushEnabled
						ST_NSDK_SYSTEM_SETTING sinfo;
						NETSDK_conf_system_get_setting_info(&sinfo);

                        if(NETSDK_json_check_child(alarmSetinfo, "MessagePushEnabled")) {
    						if(NETSDK_json_get_boolean(alarmSetinfo, "MessagePushEnabled"))
    						{				
    							sinfo.messagePushEnabled = true;
    							//ESEE_msg_push();
    						}
    						else
    						{
    							sinfo.messagePushEnabled = false;
    						}
                        }
						
						
						//MessagePushSchedule
						int i = 0;
						int arrN = 0;
						LP_JSON_OBJECT arr = NETSDK_json_get_child(alarmSetinfo, "MessagePushSchedule");
						if(arr){
							// Çå¿Õ¼ÇÂ¼
							for(i = 0
									; i < sizeof(sinfo.AlarmNotification.Schedule)
										/ sizeof(sinfo.AlarmNotification.Schedule[0])
									; ++i)
							{
								sinfo.AlarmNotification.Schedule[i].enabled = false;
							}

							arrN = json_object_array_length(arr);
							for(i = 0; i < arrN
										&& i < sizeof(sinfo.AlarmNotification.Schedule)
											/ sizeof(sinfo.AlarmNotification.Schedule[0])
									; i++)
							{					
								LP_JSON_OBJECT Scheduletime = json_object_array_get_idx(arr, i);
								if(Scheduletime)
								{
									char beginTime[12]={0};
									char endTime[12]={0};
									char weekday[18]={0};
									int beginSec, endSec;
									NETSDK_json_get_string(Scheduletime, "Weekday", weekday, sizeof(weekday));
									NETSDK_json_get_string(Scheduletime, "BeginTime", beginTime, sizeof(beginTime));
									NETSDK_json_get_string(Scheduletime, "EndTime", endTime, sizeof(endTime));
						
									if(NULL != beginTime && NULL != endTime && NULL != weekday)
									{
										if(0 != schedule_parse_time(beginTime
												, &sinfo.AlarmNotification.Schedule[i].BeginTime.hour
												, &sinfo.AlarmNotification.Schedule[i].BeginTime.min
												, &sinfo.AlarmNotification.Schedule[i].BeginTime.sec))
										{
											APP_TRACE("Parse Begin Time Failed");
											continue;
										}
						
										if(0 != schedule_parse_time(endTime
												, &sinfo.AlarmNotification.Schedule[i].EndTime.hour
												, &sinfo.AlarmNotification.Schedule[i].EndTime.min
												, &sinfo.AlarmNotification.Schedule[i].EndTime.sec))
										{
											APP_TRACE("Parse End Time Failed");
											continue;
										}
						
										if(0 != schedule_parse_weekday(weekday
												, &sinfo.AlarmNotification.Schedule[i].weekday))
										{
											APP_TRACE("Parse Weekday Failed");
											continue;
										}
						
										sinfo.AlarmNotification.Schedule[i].weekday &= 0x7f;
										beginSec = sinfo.AlarmNotification.Schedule[i].BeginTime.hour * 3600
												+ sinfo.AlarmNotification.Schedule[i].BeginTime.min * 60
												+ sinfo.AlarmNotification.Schedule[i].BeginTime.sec;
										endSec = sinfo.AlarmNotification.Schedule[i].EndTime.hour * 3600
												+ sinfo.AlarmNotification.Schedule[i].EndTime.min * 60
												+ sinfo.AlarmNotification.Schedule[i].EndTime.sec;
						
										// ÅÐ¶ÏÐÇÆÚÒÔ¼°Ê±¼äµÄºÏ·¨ÐÔ
										if(0 != sinfo.AlarmNotification.Schedule[i].weekday
												&& beginSec < endSec)
										{
											sinfo.AlarmNotification.Schedule[i].enabled = true;
										}
									}
									else
									{
										sinfo.AlarmNotification.Schedule[i].enabled = false;
									}
								}
								else{
									sinfo.AlarmNotification.Schedule[i].enabled = false;
								}
							}
						}
						//sinfo.AlarmNotification.Schedule.enabled = true;
						NETSDK_conf_system_set_setting_info(&sinfo);

                        ST_NSDK_MD_CH md_ch;

                        NETSDK_conf_md_ch_get(1, &md_ch);
                        if(NULL != motinfo)
                        {
                            md_ch.enabled = NETSDK_json_get_boolean(motinfo, "Enabled");
                            NETSDK_json_get_string(motinfo, "SensitivityLevel", level, sizeof(level));
                            mtion_setsensitivityLevel(&md_ch, level);
                            netsdk_md_ch_set(0, &md_ch);
                            if(NETSDK_json_check_child(motinfo, "MotionWarningTone")) {
                                NETSDK_conf_system_get_setting_info(&sinfo);
                                sinfo.mdAlarm.MotionWarningTone = NETSDK_json_get_boolean(motinfo, "MotionWarningTone");
                                NETSDK_conf_system_set_setting_info(&sinfo);
                            }
                        }
                    }

					//SystemOperation
					SystemOper = NETSDK_json_get_child(ipcam, "SystemOperation");

					if(SystemOper){
						char str_t[15]={0};
						struct timeval timval, cur_tv;
						time_t utc_time = 0;
                        int tz_num = 0;
						LP_JSON_OBJECT TimeZoneJson;
						ST_NSDK_SYSTEM_TIME sys_time;
						//TimeSync
						timeS = NETSDK_json_get_child(SystemOper, "TimeSync");
                        if(NULL != timeS) {

                            // ÏÈ»ñÈ¡ÏµÍ³ÅäÖÃµÄÊ±ÇøÐÅÏ¢; Èç¹ûappÉèÖÃÀ´µÄÐÅÏ¢ÖÐÓÐÊ±Çø×Ö¶Î£¬ÏÈÉèÖÃÊ±Çø
                            if (NULL != NETSDK_conf_system_get_time(&sys_time)) {
                                tz_num = sys_time.greenwichMeanTime;

                                TimeZoneJson = NETSDK_json_get_child(timeS, "TimeZone");
                                if (NULL != TimeZoneJson) {
                                    tz_num = json_object_get_int(TimeZoneJson);

                                    sys_time.greenwichMeanTime = tz_num;

                                    if (NULL != NETSDK_conf_system_set_time(&sys_time)) {
                                        if(netsdk->systemChanged){
                                            APP_TRACE("netsdk->systemChanged %x",netsdk->systemChanged);
                                            sys_time.ntpEnabled = false;  // è¿œç¨‹è®¾ç½®æ—¶é—´åŒæ­¥ä¸å¯åŠ¨ntp
                                            netsdk->systemChanged(&sys_time);
                                        }
                                    }
                                }
							}

                            if (NULL != NETSDK_json_get_string(timeS, "UTCTime", str_t, sizeof(str_t))) {
								utc_time = atol(str_t);
                            } else {
                            	str_t[0] = '\0';
                                // ±£Áô LocalTime ÊÇÎªÁËºÍÒÔÇ°±£³Ö¼æÈÝ
                               if (NULL != NETSDK_json_get_string(timeS, "LocalTime", str_t, sizeof(str_t))) {
									utc_time = atol(str_t);
                                }
                            }
                            if(utc_time > 0) {
                                gettimeofday(&cur_tv, NULL);
                                if(abs(utc_time - cur_tv.tv_sec) > 10) {
                                    timval.tv_sec = utc_time;
                                    timval.tv_usec = 0;
                                    settimeofday(&timval,NULL);
                                    RTC_settime(utc_time);
                                }
                            }
                        }

						//Upgrade
						upginfo = NETSDK_json_get_child(SystemOper, "Upgrade");

						LP_JSON_OBJECT dstJSON = NETSDK_json_get_child(SystemOper, "DaylightSavingTime");
						if(dstJSON != NULL){
							int i, j, n;
							LP_JSON_OBJECT weekJSON = NULL, tmpWeekJSON = NULL;;
							ST_NSDK_SYSTEM_DST dstInfo = {0};
							char tmpType[8];

							NETSDK_conf_system_get_DST_info(&dstInfo);
							dstInfo.enable = NETSDK_json_get_boolean(dstJSON, "Enabled");
                            NETSDK_json_get_string(dstJSON, "Country", dstInfo.country, sizeof(dstInfo.country));
							dstInfo.offset = NETSDK_json_get_int(dstJSON, "Offset");
							if((weekJSON = NETSDK_json_get_child(dstJSON, "Week")) != NULL){
								n = json_object_array_length(weekJSON);
								for(j = 0; j < n; j++){
									if((tmpWeekJSON = json_object_array_get_idx(weekJSON, j)) != NULL){
										memset(tmpType, 0, sizeof(tmpType));
										NETSDK_json_get_string(tmpWeekJSON, "Type", tmpType, sizeof(tmpType));
										for(i = 0; i < sizeof(dstInfo.week) / sizeof(dstInfo.week[0]); i++){
											if(strcmp(tmpType, dstInfo.week[i].type) == 0){
												dstInfo.week[i].month = NETSDK_json_get_int(tmpWeekJSON, "Month");
												dstInfo.week[i].week = NETSDK_json_get_int(tmpWeekJSON, "Week");
												dstInfo.week[i].weekday = NETSDK_json_get_int(tmpWeekJSON, "Weekday");
												dstInfo.week[i].hour = NETSDK_json_get_int(tmpWeekJSON, "Hour");
												dstInfo.week[i].minute = NETSDK_json_get_int(tmpWeekJSON, "Minute");
												break;
											}
										}
									}
								}
							}
							NETSDK_conf_system_set_DST_info(&dstInfo);
							if(netsdk->systemDSTChanged){
								netsdk->systemDSTChanged(&dstInfo);
							}
						}
					}
					if(upginfo)
					{
						NETSDK_json_get_string(upginfo, "Enabled", level, sizeof(level));
						if(NETSDK_json_get_boolean(upginfo, "Enabled"))
						{
							REMOTE_UPGRADE_start();
						}
					}
					//PromptSounds
					ST_NSDK_SYSTEM_SETTING pinfo;
					char ptype[20] = {0};
					Psound = NETSDK_json_get_child(ipcam, "PromptSounds");
					if(Psound)
					{
						NETSDK_conf_system_get_setting_info(&pinfo);
						NETSDK_json_get_string(Psound, "Type", ptype, sizeof(ptype));
                        if(NETSDK_json_check_child(Psound, "Enabled")) {
    						if(NETSDK_json_get_boolean(Psound, "Enabled"))
    						{
    							pinfo.promptSound.enabled = true;
    						}
    						else
    						{
    							pinfo.promptSound.enabled = false;
    						}
                        }
						pinfo.promptSound.soundType = NETSDK_MAP_STR2DEC(promptSoundType_map, ptype, pinfo.promptSound.soundType);
                        ST_CUSTOM_SETTING custom;
                        if(0 == CUSTOM_get(&custom)) {
                            custom.function.promptSoundType = pinfo.promptSound.soundType;
                            CUSTOM_set(&custom);
                        }
						NETSDK_conf_system_set_setting_info(&pinfo);
					}
				#if defined(TFCARD)
					Tfcard = NETSDK_json_get_child(ipcam, "TfcardManager");
					if(Tfcard){
						char formate[10]={0};
						NETSDK_json_get_string(Tfcard, "Operation", formate, sizeof(formate));
						if(0 == strcmp(formate, "format"))
						{
						    TFCARD_stop_record();
							NK_TFCARD_format();
						}
						//TFcard_recordSchedule
						ST_NSDK_SYSTEM_SETTING minfo = {0};
						NETSDK_conf_system_get_setting_info(&minfo);

                        if(NETSDK_json_check_child(Tfcard, "TimeRecordEnabled")) {
                            if(NETSDK_json_get_boolean(Tfcard, "TimeRecordEnabled"))
                            {
                                minfo.timeRecordEnabled = true;
                            }
                            else
                            {
                                minfo.timeRecordEnabled = false;
                            }
                        }

						int i = 0;
						int arrN = 0;
						LP_JSON_OBJECT arr = NETSDK_json_get_child(Tfcard, "TFcard_recordSchedule");

						if(arr){
							//clear setting before
							for(i = 0
									; i < sizeof(minfo.TFcard_Record.Schedule)
										/ sizeof(minfo.TFcard_Record.Schedule[0])
									; i++)
							{
								minfo.TFcard_Record.Schedule[i].enabled = false;
							}

							arrN = json_object_array_length(arr);
							//printf("\n******arrN = %d******\n",arrN);
							for(i = 0; i < arrN
										&& i < sizeof(minfo.TFcard_Record.Schedule)
											/ sizeof(minfo.TFcard_Record.Schedule[0])
									; i++)
							{
								LP_JSON_OBJECT recordScheduleTime = json_object_array_get_idx(arr, i);
								if(recordScheduleTime){
									char beginTime[12] = {0};
									char endTime[12] = {0};
									char weekday[18] = {0};
									int beginSec,endSec;

									NETSDK_json_get_string(recordScheduleTime, "Weekday", weekday, sizeof(weekday));
									NETSDK_json_get_string(recordScheduleTime, "BeginTime", beginTime, sizeof(beginTime));
									NETSDK_json_get_string(recordScheduleTime, "EndTime", endTime, sizeof(endTime));

									if(NULL != beginTime && NULL != endTime && NULL != weekday)
									{
										if(0 != schedule_parse_time(beginTime
												, &minfo.TFcard_Record.Schedule[i].BeginTime.hour
												, &minfo.TFcard_Record.Schedule[i].BeginTime.min
												, &minfo.TFcard_Record.Schedule[i].BeginTime.sec))
										{
											APP_TRACE("Parse Begin Time Failed");
											continue;
										}
										if(0 != schedule_parse_time(endTime
												, &minfo.TFcard_Record.Schedule[i].EndTime.hour
												, &minfo.TFcard_Record.Schedule[i].EndTime.min
												, &minfo.TFcard_Record.Schedule[i].EndTime.sec))
										{
											APP_TRACE("Parse End Time Failed");
											continue;
										}
										if(0 != schedule_parse_weekday(weekday
												, &minfo.TFcard_Record.Schedule[i].weekday))
										{
											APP_TRACE("Parse Weekday Failed");
											continue;
										}

										minfo.TFcard_Record.Schedule[i].weekday &= 0x7f;
										beginSec = minfo.TFcard_Record.Schedule[i].BeginTime.hour * 3600
												+ minfo.TFcard_Record.Schedule[i].BeginTime.min * 60
												+ minfo.TFcard_Record.Schedule[i].BeginTime.sec;
										endSec = minfo.TFcard_Record.Schedule[i].EndTime.hour * 3600
												+ minfo.TFcard_Record.Schedule[i].EndTime.min * 60
												+ minfo.TFcard_Record.Schedule[i].EndTime.sec;

										// verify time
										if(0 != minfo.TFcard_Record.Schedule[i].weekday
												&& beginSec < endSec)
										{
											minfo.TFcard_Record.Schedule[i].enabled = true;
										}
									}
									else{
										minfo.TFcard_Record.Schedule[i].enabled = false;
									}
								}
								else{
									minfo.TFcard_Record.Schedule[i].enabled = false;
								}
							}
						}else{
							printf("\n***not \"TFcard_recordSchedule\" field in setup_jsonp***\n");
						}
						NETSDK_conf_system_set_setting_info(&minfo);
					}else{
						printf("\n***not Tfcard_Manager field in setup_jsonp***\n");
					}
				#endif
					fisheyeSetinfo = NETSDK_json_get_child(ipcam, "FisheyeSetting");
					if(fisheyeSetinfo){
						ST_NSDK_IMAGE image2 = {0};
						NK_Char fixMode[32] = {0};
						int fixType;
						if(NETSDK_json_get_string(fisheyeSetinfo,"FixMode",fixMode,sizeof(fixMode)) != NULL) {
							if(0 == strcmp(fixMode, "wall")){
								fixType = eNSDK_IMAGE_FISHEYE_FIX_MODE_WALL;
							}else if(0 == strcmp(fixMode, "cell")){
								fixType = eNSDK_IMAGE_FISHEYE_FIX_MODE_CELL;
							}else if(0 == strcmp(fixMode, "table")){
								fixType = eNSDK_IMAGE_FISHEYE_FIX_MODE_TABLE;
							}else{
								fixType = eNSDK_IMAGE_FISHEYE_FIX_MODE_NONE;
							}
						}
						if(NETSDK_conf_image_get(&image2)){
							image2.videoMode.fixType = fixType;
							NETSDK_conf_image_set(&image2);
						}

						stFISHEYE_config conf = {0};
						FISHEYE_config_get(&conf);

						int i = 0;
						int arrN = 0;
						LP_JSON_OBJECT fixParam = NETSDK_json_get_child(fisheyeSetinfo, "FixParam");
                        bool type = false;
                        if(NETSDK_json_check_child(fisheyeSetinfo, "LensName")) {
                            NETSDK_json_get_string(fisheyeSetinfo, "LensName", conf.lensName, sizeof(conf.lensName));
                        }
                        else {
                            sprintf(conf.lensName, "m109");
                        }
						conf.fixMode = fixType;
						if(fixParam){
							arrN = json_object_array_length(fixParam);
							for(i = 0; ((i < arrN) && (i < (sizeof(conf.param) / sizeof(conf.param[0]))));i++){
								LP_JSON_OBJECT fixParamData = json_object_array_get_idx(fixParam,i);
								if(fixParamData){
                                    /* Ö»ÅÐ¶ÏµÚÒ»¸öÊý¾Ý£¬Èç¹ûÊÇÕûÐÎÔò°´int´æ´¢£¬Èç¹ûÊÇ¸¡µãÔò°´float´æ´¢ */
                                    if(json_type_int == json_object_get_type(json_object_object_get(fixParamData, "CenterCoordinateX"))) {
                                        conf.param[i].CenterCoordinateX = NETSDK_json_get_int(fixParamData, "CenterCoordinateX");
                                        conf.param[i].CenterCoordinateY = NETSDK_json_get_int(fixParamData, "CenterCoordinateY");
                                        conf.param[i].Radius = NETSDK_json_get_int(fixParamData, "Radius");
                                        conf.param[i].AngleX = NETSDK_json_get_int(fixParamData, "AngleX");
                                        conf.param[i].AngleY = NETSDK_json_get_int(fixParamData, "AngleY");
                                        conf.param[i].AngleZ = NETSDK_json_get_int(fixParamData, "AngleZ");
                                        type = true;
                                    }
                                    else {
                                        conf.param2[i].CenterCoordinateX = NETSDK_json_get_float(fixParamData, "CenterCoordinateX");
                                        conf.param2[i].CenterCoordinateY = NETSDK_json_get_float(fixParamData, "CenterCoordinateY");
                                        conf.param2[i].Radius = NETSDK_json_get_float(fixParamData, "Radius");
                                        conf.param2[i].AngleX = NETSDK_json_get_float(fixParamData, "AngleX");
                                        conf.param2[i].AngleY = NETSDK_json_get_float(fixParamData, "AngleY");
                                        conf.param2[i].AngleZ = NETSDK_json_get_float(fixParamData, "AngleZ");
                                        type = false;
                                    }
								}
							}
							FISHEYE_config_set(&conf, type);
						}
					}

#if defined (LED_PWM)
                    ledPwm = NETSDK_json_get_child(ipcam, "ledPwm");
                    if(ledPwm) {
                        int i = 0, j = 0;
                        stLED_PWM_config setLedPwmConfig;
                        LED_PWM_get(&setLedPwmConfig);  // ÎªÁË±ÜÃâchannelCountÉèÖÃÓÐÎó£¬Ê¹ÓÃÉè±¸±£´æµÄÖµ
                        setLedPwmConfig.ledSwitch = NETSDK_json_get_int(ledPwm, "switch");
                        LP_JSON_OBJECT child_json = NETSDK_json_get_child(ledPwm, "channelInfo");
                        if(child_json) {
                            for(i = 0; i < setLedPwmConfig.channelCount; i++) {
                                LP_JSON_OBJECT info = json_object_array_get_idx(child_json, i);
                                if(info) {
                                    for(j = 0; j < setLedPwmConfig.channelCount; j++) {
                                        if(setLedPwmConfig.array[j].channel == NETSDK_json_get_int(info, "channel")) {
                                            setLedPwmConfig.array[j].num = NETSDK_json_get_int(info, "num");
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                        LED_PWM_set(setLedPwmConfig);
                    }
#endif

					LP_JSON_OBJECT Userinfo=NULL, Authinfo=NULL;
					char method[10]={0};
					char p2p_Verify[255]={0};
					char username[25]={0};
					char oldpwd[25]={0};
					char newpasswd[25]={0};
					char recvarify[255]={0};
					Userinfo = NETSDK_json_get_child(obj, "UserManager");
					Authinfo = NETSDK_json_get_child(obj, "Authorization"); 

					if(Userinfo && Authinfo){

						NETSDK_json_get_string(Authinfo, "Verify", recvarify, sizeof(recvarify));
						NETSDK_json_get_string(Userinfo, "Method", method, sizeof(method));
						NETSDK_json_get_string(Userinfo, "Verify", p2p_Verify, sizeof(p2p_Verify));
						NETSDK_json_get_string(Userinfo, "username", username, sizeof(username));

						if(NULL == recvarify || (0 == strlen(recvarify)) )
						{
							NETSDK_json_get_string(Authinfo, "password", oldpwd, sizeof(oldpwd));
						}
						else
						{
							//parse varify
							p2p_parse_userpwd_varify2(recvarify, username, oldpwd);
						}
						if(!p2p_Verify || (0 == strlen(p2p_Verify)))
						{							
							NETSDK_json_get_string(Userinfo, "password", newpasswd, sizeof(newpasswd));
						}
						else
						{							
							p2p_parse_userpwd_varify(p2p_Verify, username, newpasswd);
						}
					}

					if(method && (0 == strcmp(method, "modify")))
					{
						if(!p2p_Auth_userpwd_set(username, oldpwd, newpasswd)){
							json_object_put(obj);
							return 0;
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      						}
					}

                    wirelessManager = NETSDK_json_get_child(ipcam, "WirelessManager");
                    if(wirelessManager != NULL) {
#if defined (CX)
                        if(false == GLOBAL_sn_front()) {
                            ST_NSDK_NETWORK_INTERFACE inter;
                            char base64ApPsk[128];
                            NETSDK_conf_interface_get(4, &inter);
                            if(inter.wireless.wirelessMode == NSDK_NETWORK_WIRELESS_MODE_ACCESSPOINT) {
                                memset(base64ApPsk, 0, sizeof(base64ApPsk));
                                if(NULL != NETSDK_json_get_string(wirelessManager, "ApPsk", base64ApPsk, sizeof(base64ApPsk))) {
                                    BASE64_decode(base64ApPsk, strlen(base64ApPsk),
                                    inter.wireless.wirelessApMode.wirelessPsk, sizeof(inter.wireless.wirelessApMode.wirelessPsk));
                                    NETSDK_conf_interface_set_by_delay(4, &inter, eNSDK_CONF_SAVE_RESTART, 5);
                                }
                            }
                        }
#endif
                    }

					json_object_put(obj);
					sprintf((char *)response, "{\r\n\"option\" :\"success\"\r\n}\r\n");
					return strlen((char *)response);
			}
			
		}
	}
	return 0;
}

