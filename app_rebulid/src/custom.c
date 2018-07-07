#include "app_debug.h"
#include "netsdk_json.h"
#include "netsdk_private.h"
#include "custom.h"
#include "generic.h"

static const ST_NSDK_MAP_STR_DEC fixMode_map[] = {
	{"wall", eNSDK_IMAGE_FISHEYE_FIX_MODE_WALL},
	{"cell", eNSDK_IMAGE_FISHEYE_FIX_MODE_CELL},
	{"table", eNSDK_IMAGE_FISHEYE_FIX_MODE_TABLE},
	{"none", eNSDK_IMAGE_FISHEYE_FIX_MODE_NONE},
};	

static const ST_NSDK_MAP_STR_DEC promptSoundType_map[] = {
	{"chinese", kNSDK_SYSTEM_PROMPT_SOUND_TYPE_CHINESE},
	{"english", kNSDK_SYSTEM_PROMPT_SOUND_TYPE_ENGLISH},
	{"german", kNSDK_SYSTEM_PROMPT_SOUND_TYPE_GERMAN},
	{"korean", kNSDK_SYSTEM_PROMPT_SOUND_TYPE_KOREAN},
	{"portuguese", kNSDK_SYSTEM_PROMPT_SOUND_TYPE_PORTUGUESE},
	{"russian", kNSDK_SYSTEM_PROMPT_SOUND_TYPE_RUSSIAN},
	{"spanish", kNSDK_SYSTEM_PROMPT_SOUND_TYPE_SPANISH}
};

static const ST_NSDK_MAP_STR_DEC irCutControlMode_map[] = {
	{"hardware", kNSDK_IMAGE_IRCUT_CONTROL_MODE_HARDWARE},
	{"software", kNSDK_IMAGE_IRCUT_CONTROL_MODE_SOFTWARE},
};

static const ST_NSDK_MAP_STR_DEC productType_map[] = {
	{"PX", eNSDK_PRODUCT_TYPE_PX},
	{"CX", eNSDK_PRODUCT_TYPE_CX},
};

#define CUSTOM_SETTING_FILE_PATH "/media/conf/custom.conf"
#define CUSTOM_SETTING_DEFAULT_FILE_PATH "/media/custom/custom.conf"
//#define CUSTOM_SETTING_DEFAULT_FILE_PATH "/root/nfs/git_ipc/server_ipc/git_ipc/app_rebulid/bin/custom.conf"

typedef struct _custom
{
	ST_CUSTOM_SETTING custom;
	pthread_mutex_t mutex;
}ST_CUSTOM,*LP_CUSTOM;

static LP_CUSTOM custom_attr = NULL;

static void custom_setting_dump(LP_CUSTOM_SETTING custom_conf)
{
	APP_TRACE("function.audioInputGain:%d", custom_conf->function.audioInputGain);
	APP_TRACE("function.audioInputVolume:%d", custom_conf->function.audioInputVolume);
	APP_TRACE("function.audioOutputGain:%d", custom_conf->function.audioOutputGain);
	APP_TRACE("function.fixMode:%d", custom_conf->function.fixMode);
	APP_TRACE("function.ipAdapted:%d", custom_conf->function.ipAdapted);
	APP_TRACE("function.promptSoundType:%d", custom_conf->function.promptSoundType);
	APP_TRACE("function.irCutControlMode:%d", custom_conf->function.irCutControlMode);
	APP_TRACE("function.p2pServerIp:%s", custom_conf->function.p2pServerIp);
	APP_TRACE("function.irCutControlThresh:%d,%d,%d,%d,%d,%d,%d,%d", 
		custom_conf->function.irCutControlThresh[0],
		custom_conf->function.irCutControlThresh[1],
		custom_conf->function.irCutControlThresh[2],
		custom_conf->function.irCutControlThresh[3],
		custom_conf->function.irCutControlThresh[4],
		custom_conf->function.irCutControlThresh[5],
		custom_conf->function.irCutControlThresh[6],
		custom_conf->function.irCutControlThresh[7]);
	APP_TRACE("function.imageStyle:%d", custom_conf->function.imageStyle);
    APP_TRACE("function.audioHwSpec:%d", custom_conf->function.audioHwSpec);
	APP_TRACE("function.ledPwmEnabled:%d", custom_conf->function.ledPwmEnabled);
	APP_TRACE("function.ledPwmChannelCount:%d", custom_conf->function.ledPwmChannelCount);
    APP_TRACE("function.powerLineFrequencyMode:%d", custom_conf->function.powerLineFrequencyMode);
	APP_TRACE("protocol.hikvision:%s", (custom_conf->protocol.hikvision == 1)?"True":"False");
	APP_TRACE("model.oemNumber:%s", custom_conf->model.oemNumber);
    APP_TRACE("model.productType:%d", custom_conf->model.productType);
}

static int custom_setting_save(LP_JSON_OBJECT json)
{
	json_object_to_file(CUSTOM_SETTING_FILE_PATH, json);
	return 0;
}

static void custom_setting_parse_thresh_array8(char *text, int *array)
{
	if(text && array){
		int _array[8];
		memset(_array, 0, sizeof(_array));
		if(8 == sscanf(text, "%d,%d,%d,%d,%d,%d,%d,%d",
			&_array[0], &_array[1], &_array[2], &_array[3], &_array[4], &_array[5], &_array[6], &_array[7])){
			memcpy(array, _array, sizeof(_array));
		}else{
			APP_TRACE("Wrong array format");
		}
	}
}

static void custom_setting_array2str(char *text, int *array)
{
	if(array){
		int _array[8];
		memcpy(_array, array, sizeof(_array));
		sprintf(text, "%d,%d,%d,%d,%d,%d,%d,%d",
			_array[0], _array[1], _array[2], _array[3], _array[4], _array[5], _array[6], _array[7]);
	}
}

static void custom_setting_init_struct(LP_CUSTOM_SETTING custom_conf)
{
	if(custom_conf){
		//function
		custom_conf->function.audioInputGain = -1;
		custom_conf->function.audioInputVolume = -1;
		custom_conf->function.audioOutputGain = -1;
		custom_conf->function.fixMode = -1;
		custom_conf->function.imageStyle = -1;
		custom_conf->function.ipAdapted = -1;
		custom_conf->function.promptSoundType = -1;
		custom_conf->function.irCutControlMode = -1;
		memset(custom_conf->function.irCutControlThresh, 0, sizeof(custom_conf->function.irCutControlThresh));
		memset(custom_conf->function.p2pServerIp, 0, sizeof(custom_conf->function.p2pServerIp));
        custom_conf->function.audioHwSpec = -1;
		custom_conf->function.ledPwmEnabled = -1;
		custom_conf->function.ledPwmChannelCount = -1;
        custom_conf->function.powerLineFrequencyMode = -1;

		//module

		//protocol
		custom_conf->protocol.hikvision = -1;

		//model
		memset(custom_conf->model.oemNumber, 0, sizeof(custom_conf->model.oemNumber));
        custom_conf->model.productType = -1;
	}
}

static int custom_conf_json_parse(LP_JSON_OBJECT json, LP_CUSTOM_SETTING custom_conf)
{
	char text[64];
	LP_JSON_OBJECT child_json = NULL;
	
	if(json && custom_conf){
		//Function
		child_json = NETSDK_json_get_child(json, "Function");
		if(child_json){
			if(NETSDK_json_check_child(child_json, "audioInputGain")){
				custom_conf->function.audioInputGain = NETSDK_json_get_int(child_json, "audioInputGain");
			}
			if(NETSDK_json_check_child(child_json, "audioInputVolume")){
				custom_conf->function.audioInputVolume = NETSDK_json_get_int(child_json, "audioInputVolume");
			}
			if(NETSDK_json_check_child(child_json, "audioOutputGain")){
				custom_conf->function.audioOutputGain= NETSDK_json_get_int(child_json, "audioOutputGain");
			}
			if(NETSDK_json_check_child(child_json, "ipAdapted")){
				custom_conf->function.ipAdapted = (int)NETSDK_json_get_boolean(child_json, "ipAdapted");
			}
			if(NETSDK_json_check_child(child_json, "fixMode")){
				if(NULL != NETSDK_json_get_string(child_json, "fixMode", text, sizeof(text))){
					custom_conf->function.fixMode = NETSDK_MAP_STR2DEC(fixMode_map,text,eNSDK_IMAGE_FISHEYE_FIX_MODE_CELL);
				}
			}
			if(NETSDK_json_check_child(child_json, "promptSoundType")){
				if(NULL != NETSDK_json_get_string(child_json, "promptSoundType", text, sizeof(text))){
					custom_conf->function.promptSoundType = NETSDK_MAP_STR2DEC(promptSoundType_map,text,kNSDK_SYSTEM_PROMPT_SOUND_TYPE_CHINESE);
				}
			}
			if(NETSDK_json_check_child(child_json, "p2pServerIp")){
				NETSDK_json_get_string(child_json, "p2pServerIp", custom_conf->function.p2pServerIp, sizeof(custom_conf->function.p2pServerIp));
				//FIX ME:  防止OEM配置文件人为出错，写入局域网IP地址
				if(NULL != strstr(custom_conf->function.p2pServerIp, "192.168")){
					//avoid wrong serverIP
					APP_TRACE("avoid wrong serverIP:%s", custom_conf->function.p2pServerIp);
					memset(custom_conf->function.p2pServerIp, 0, sizeof(custom_conf->function.p2pServerIp));
				}
			}
			if(NETSDK_json_check_child(child_json, "irCutControlMode")){
				if(NULL != NETSDK_json_get_string(child_json, "irCutControlMode", text, sizeof(text))){
					custom_conf->function.irCutControlMode = NETSDK_MAP_STR2DEC(irCutControlMode_map,text,kNSDK_IMAGE_IRCUT_CONTROL_MODE_HARDWARE);
				}
			}
			if(NETSDK_json_check_child(child_json, "irCutControlThresh")){
				if(NULL != NETSDK_json_get_string(child_json, "irCutControlThresh", text, sizeof(text))){
					custom_setting_parse_thresh_array8(text, custom_conf->function.irCutControlThresh);
				}
			}
			if(NETSDK_json_check_child(child_json, "imageStyle")){
				custom_conf->function.imageStyle = NETSDK_json_get_int(child_json, "imageStyle");
			}
            if(NETSDK_json_check_child(child_json, "audioHwSpec")){
				custom_conf->function.audioHwSpec = NETSDK_json_get_int(child_json, "audioHwSpec");
			}

			if(NETSDK_json_check_child(child_json, "ledPwmEnabled")){
				custom_conf->function.ledPwmEnabled = (int)NETSDK_json_get_boolean(child_json, "ledPwmEnabled");
			}
			if(NETSDK_json_check_child(child_json, "ledPwmChannelCount")){
				custom_conf->function.ledPwmChannelCount = NETSDK_json_get_int(child_json, "ledPwmChannelCount");
			}
            if(NETSDK_json_check_child(child_json, "powerLineFrequencyMode")){
                custom_conf->function.powerLineFrequencyMode = NETSDK_json_get_int(child_json, "powerLineFrequencyMode");
            }
		}

		//Module
		child_json = NETSDK_json_get_child(json, "Module");		
		if(child_json){
			
		}

		//Protocol
		child_json = NETSDK_json_get_child(json, "Protocol");
		if(child_json){

		}

		//Model
		child_json = NETSDK_json_get_child(json, "Model");
		if(child_json){
            if(NETSDK_json_check_child(child_json, "productType")){
                if(NULL != NETSDK_json_get_string(child_json, "productType", text, sizeof(text))){
                    custom_conf->model.productType = NETSDK_MAP_STR2DEC(productType_map,text,eNSDK_PRODUCT_TYPE_PX);
                }
            }
			if(NETSDK_json_check_child(child_json, "oemNumber")){
				NETSDK_json_get_string(child_json, "oemNumber", custom_conf->model.oemNumber, sizeof(custom_conf->model.oemNumber));
			}else{
				return -1;
			}
		}else{
			return -1;
		}
	}
	return 0;
}

static int custom_setting_file_to_string(char *str)
{
	LP_JSON_OBJECT json_conf =  NETSDK_json_load(CUSTOM_SETTING_FILE_PATH);
	if(NULL != json_conf){
		snprintf(str, 1024, "%s", json_object_to_json_string(json_conf));
		json_object_put(json_conf);
		return 0;
	}else{
	
	}
	return -1;
}

static int custom_setting_load(const char *fileName, LP_CUSTOM_SETTING custom_conf)
{
	int ret = -1;
	LP_JSON_OBJECT json_conf =  NETSDK_json_load(fileName);
	custom_setting_init_struct(custom_conf);
	if(NULL != json_conf){
		ret = custom_conf_json_parse(json_conf, custom_conf);
		json_object_put(json_conf);
		if(-1 == ret){
			APP_TRACE("NO OEM number");
			return -1;
		}
	}else{
		//APP_TRACE("NO OEM file in media conf");
		return -1;
	}
	return 0;
}

static int custom_match_struct(LP_CUSTOM_SETTING attr)
{
	ST_CUSTOM_SETTING custom_conf;
	custom_setting_load(CUSTOM_SETTING_FILE_PATH, &custom_conf);
	//model
	if(CUSTOM_check_string_valid(custom_conf.model.oemNumber)){
		sprintf(attr->model.oemNumber, "%s", custom_conf.model.oemNumber);
	}
	if(CUSTOM_check_int_valid(custom_conf.model.productType)){
		attr->model.productType = custom_conf.model.productType;
	}

	//function
	if(CUSTOM_check_int_valid(custom_conf.function.audioInputGain)){
		attr->function.audioInputGain  = custom_conf.function.audioInputGain;
	}
	if(CUSTOM_check_int_valid(custom_conf.function.audioInputVolume)){
		attr->function.audioInputVolume = custom_conf.function.audioInputVolume;
	}
	if(CUSTOM_check_int_valid(custom_conf.function.audioOutputGain)){
		attr->function.audioOutputGain  = custom_conf.function.audioOutputGain;
	}
	if(CUSTOM_check_int_valid(custom_conf.function.fixMode)){
		attr->function.fixMode = custom_conf.function.fixMode;
	}
	if(CUSTOM_check_int_valid(custom_conf.function.imageStyle)){
		attr->function.imageStyle = custom_conf.function.imageStyle;
	}
	if(CUSTOM_check_int_valid(custom_conf.function.promptSoundType)){
		attr->function.promptSoundType = custom_conf.function.promptSoundType;
	}
	if(CUSTOM_check_int_valid(custom_conf.function.ipAdapted)){
		attr->function.ipAdapted = custom_conf.function.ipAdapted;
	}
	if(CUSTOM_check_int_valid(custom_conf.function.irCutControlMode)){
		attr->function.irCutControlMode = custom_conf.function.irCutControlMode;
	}
	if(CUSTOM_check_string_valid(custom_conf.function.p2pServerIp)){
		sprintf(attr->function.p2pServerIp, "%s", custom_conf.function.p2pServerIp);
	}
	if(CUSTOM_check_int_valid(custom_conf.function.audioHwSpec)){
		attr->function.audioHwSpec  = custom_conf.function.audioHwSpec;
	}
	if(CUSTOM_check_int_valid(custom_conf.function.ledPwmEnabled)){
		attr->function.ledPwmEnabled  = custom_conf.function.ledPwmEnabled;
	}
	if(CUSTOM_check_int_valid(custom_conf.function.ledPwmChannelCount)){
		attr->function.ledPwmChannelCount  = custom_conf.function.ledPwmChannelCount;
	}
    if(CUSTOM_check_int_valid(custom_conf.function.powerLineFrequencyMode)){
        attr->function.powerLineFrequencyMode  = custom_conf.function.powerLineFrequencyMode;
    }

	//protocal
	if(CUSTOM_check_int_valid(custom_conf.protocol.hikvision)){
		attr->protocol.hikvision = custom_conf.protocol.hikvision;
	}

	return 0;
}

static LP_JSON_OBJECT custom_create_object(LP_CUSTOM_SETTING attr)
{
	LP_JSON_OBJECT obj = json_object_new_object();
	LP_JSON_OBJECT param =NULL;
	int model_flag = 0, function_flag = 0, protocol_flag = 0, module_flag = 0;;
	char *text;

	//model
	param = json_object_new_object();
	if(CUSTOM_check_string_valid(attr->model.oemNumber)){
		NETSDK_json_set_string2(param, "oemNumber", attr->model.oemNumber);
		model_flag = 1;
	}
    if(CUSTOM_check_int_valid(attr->model.productType)){
		text = NETSDK_MAP_DEC2STR(productType_map, attr->model.productType, eNSDK_PRODUCT_TYPE_PX);
		NETSDK_json_set_string(param, "productType", text);
		model_flag = 1;
	}
	if(model_flag){
		json_object_object_add(obj, "Model", param);
	}else{
		//needn't to add to json file
		json_object_put(param);
		param = NULL;
	}

	//function
	param = json_object_new_object();
	if(CUSTOM_check_int_valid(attr->function.audioInputGain)){
		NETSDK_json_set_int(param, "audioInputGain", attr->function.audioInputGain);
		function_flag = 1;
	}
	if(CUSTOM_check_int_valid(attr->function.audioInputVolume)){
		NETSDK_json_set_int(param, "audioInputVolume", attr->function.audioInputVolume);
		function_flag = 1;
	}
	if(CUSTOM_check_int_valid(attr->function.audioOutputGain)){
		NETSDK_json_set_int(param, "audioOutputGain", attr->function.audioOutputGain);
		function_flag = 1;
	}
	if(CUSTOM_check_int_valid(attr->function.fixMode)){
		text = NETSDK_MAP_DEC2STR(fixMode_map, attr->function.fixMode,eNSDK_IMAGE_FISHEYE_FIX_MODE_CELL);
		NETSDK_json_set_string(param, "fixMode", text);
		function_flag = 1;
	}
	if(CUSTOM_check_int_valid(attr->function.imageStyle)){
		NETSDK_json_set_int(param, "imageStyle", attr->function.imageStyle);
		function_flag = 1;
	}
	if(CUSTOM_check_int_valid(attr->function.promptSoundType)){
		text = NETSDK_MAP_DEC2STR(promptSoundType_map, attr->function.promptSoundType,kNSDK_SYSTEM_PROMPT_SOUND_TYPE_CHINESE);
		NETSDK_json_set_string(param, "promptSoundType", text);
		function_flag = 1;
	}
	if(CUSTOM_check_int_valid(attr->function.ipAdapted)){
		NETSDK_json_set_int(param, "ipAdapted", attr->function.ipAdapted);
		function_flag = 1;
	}
	if(CUSTOM_check_int_valid(attr->function.irCutControlMode)){
		text = NETSDK_MAP_DEC2STR(irCutControlMode_map, attr->function.irCutControlMode,kNSDK_IMAGE_IRCUT_CONTROL_MODE_HARDWARE);
		NETSDK_json_set_string(param, "irCutControlMode", text);
		function_flag = 1;
	}
	if(CUSTOM_check_array_valid(attr->function.irCutControlThresh, 8)){
		char array_str[32];
		custom_setting_array2str(array_str, attr->function.irCutControlThresh);
		NETSDK_json_set_string(param, "irCutControlThresh", array_str);
		function_flag = 1;
	}
	if(CUSTOM_check_string_valid(attr->function.p2pServerIp)){
		NETSDK_json_set_string(param, "p2pServerIp", attr->function.p2pServerIp);
		function_flag = 1;
	}
    if(CUSTOM_check_int_valid(attr->function.audioHwSpec)){
		NETSDK_json_set_int(param, "audioHwSpec", attr->function.audioHwSpec);
		function_flag = 1;
	}
	if(CUSTOM_check_int_valid(attr->function.ledPwmEnabled)){
		NETSDK_json_set_boolean(param, "ledPwmEnabled", attr->function.ledPwmEnabled);
		function_flag = 1;
	}
	if(CUSTOM_check_int_valid(attr->function.ledPwmChannelCount)){
		NETSDK_json_set_int(param, "ledPwmChannelCount", attr->function.ledPwmChannelCount);
		function_flag = 1;
	}
    if(CUSTOM_check_int_valid(attr->function.powerLineFrequencyMode)){
        NETSDK_json_set_int(param, "powerLineFrequencyMode", attr->function.powerLineFrequencyMode);
        function_flag = 1;
    }
	if(function_flag){
		json_object_object_add(obj, "Function", param);
	}else{
		//needn't to add to json file
		json_object_put(param);
		param = NULL;
	}

	//protocol
	param = json_object_new_object();
	if(CUSTOM_check_int_valid(attr->protocol.hikvision)){
		NETSDK_json_set_boolean(param, "hikvision", attr->protocol.hikvision);
		protocol_flag = 1;
	}
	if(protocol_flag){
		json_object_object_add(obj, "Protocol", param);
	}else{
		//needn't to add to json file
		json_object_put(param);
		param = NULL;
	}

	//module

	if((0 == model_flag) &&
		(0 == function_flag) &&
		(0 == protocol_flag) &&
		(0 == module_flag)){
		json_object_put(obj);
		obj = NULL;
	}
	
	return obj;
}

int CUSTOM_set_json_string(char * json_str)
{
	int ret = -1;
	LP_JSON_OBJECT json = NULL;

	if(custom_attr){
		pthread_mutex_lock(&custom_attr->mutex);
		if(NULL != (json = NETSDK_json_parse(json_str))){
			custom_setting_save(json);
			custom_setting_load(CUSTOM_SETTING_FILE_PATH, &custom_attr->custom);
			json_object_put(json);
			ret = 0;
		}else{
			ret = -1;
		}
	}

	pthread_mutex_unlock(&custom_attr->mutex);
	return ret;
}

bool CUSTOM_check_int_valid(int input)
{
	return (-1 == input)? false:true;
}

bool CUSTOM_check_string_valid(char *input)
{	
	return (strlen(input) > 0)? true:false;
}

bool CUSTOM_check_array_valid(int *input, int length)
{
	int i,sum = 0;;
	for(i = 0; i < length; i++){
		sum += *input;
		input++;
	}
	return (sum > 0)? true:false;
}

int CUSTOM_set(LP_CUSTOM_SETTING attr)
{
	LP_JSON_OBJECT obj = NULL;
	obj = custom_create_object(attr);
    if(obj){
		custom_setting_save(obj);
        custom_setting_load(CUSTOM_SETTING_FILE_PATH, &custom_attr->custom);
		json_object_put(obj);
	}
	obj = NULL;
	return 0;
}

int CUSTOM_get_json_string(char * json_str)
{
	custom_setting_file_to_string(json_str);
	return 0;
}

int CUSTOM_get(LP_CUSTOM_SETTING attr)
{
	if(attr && custom_attr){
		memcpy(attr, &custom_attr->custom, sizeof(ST_CUSTOM_SETTING));
		return 0;
	}
	return -1;
}

int CUSTOM_init()
{
	int ret = 0;
	if(NULL == custom_attr){
		custom_attr = (LP_CUSTOM)calloc(sizeof(ST_CUSTOM), 1);
		ret = custom_setting_load(CUSTOM_SETTING_FILE_PATH, &custom_attr->custom);
		if((0 != ret)){
			//use default custom.conf in /media/custom
			ST_CUSTOM_SETTING custom_def;
			custom_setting_load(CUSTOM_SETTING_DEFAULT_FILE_PATH, &custom_def);
			custom_match_struct(&custom_def);
			CUSTOM_set(&custom_def);
			custom_setting_load(CUSTOM_SETTING_FILE_PATH, &custom_attr->custom);
		}else{
		
		}
		//custom_setting_dump(&custom_attr->custom);
		pthread_mutex_init(&custom_attr->mutex, NULL);
	}
	
	return 0;
}

void CUSTOM_destory()
{
	if(custom_attr){
		free(custom_attr);
		custom_attr = NULL;
	}
}
