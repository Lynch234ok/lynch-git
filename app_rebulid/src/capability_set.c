#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <json/json.h>
#include "capability_set.h"



#define CAPABILITY_SET_JSON            "/media/conf/capability_set.json"
#define CAPABILITY_SET_CUSTOM_JSON     "/media/tf/CapabilitySet.json"

typedef struct CAPABILITY_SET_ATTR
{
    stCAPABILITY_SET stCapabilitySet;
    bool stCapabilitySetInited;
}stCAPABILITY_SET_ATTR, *pstCAPABILITY_SET_ATTR;

static stCAPABILITY_SET_ATTR stCapabilitySetAttr;


static void capabilitySet_structInit(pstCAPABILITY_SET config)
{
    if(NULL != config)
    {
        config->version = 1;
        config->maxChannel = 1;
        config->lightControl = 0;
        config->bulbControl = 0;
        config->fisheye = emCAPABILITY_SET_FISHEYE_TYPE_NONE;
        config->ptz = true;
        config->sdCard = true;
        config->lte = false;
        config->wifi = true;
        config->rtc = false;
        config->rj45 = false;
        config->powerBattery = false;
        config->audioInput = true;
        config->audioOutput = true;
        config->bluetooth = false;
    }

}

static int capabilitySet_jsonSetInt(struct json_object *json, const char *key, int val)
{
    struct json_object *tmpJson = NULL;

    if((NULL != key) && (NULL != json)) {
        tmpJson = json_object_new_int(val);
        if(NULL != tmpJson) {
            json_object_object_add(json, key, tmpJson);
            return 0;
        }
    }

    return -1;

}

static int capabilitySet_jsonSetBoolean(struct json_object *json, const char *key, bool val)
{
    struct json_object *tmpJson = NULL;

    if((NULL != key) && (NULL != json)) {
        tmpJson = json_object_new_boolean(val);
        if(NULL != tmpJson) {
            json_object_object_add(json, key, tmpJson);
            return 0;
        }
    }

    return -1;

}

static int capabilitySet_structSaveFile(char *fileName, const stCAPABILITY_SET setConfig)
{
    struct json_object *json = json_object_new_object();
    struct json_object *tmpJson = json_object_new_object();

    int ret = 0;

    if((NULL != json) && (NULL != tmpJson)) {
        capabilitySet_jsonSetInt(tmpJson, "version", setConfig.version);
        capabilitySet_jsonSetBoolean(tmpJson, "audioInput", setConfig.audioInput);
        capabilitySet_jsonSetBoolean(tmpJson, "audioOutput", setConfig.audioOutput);
        json_object_object_add(json, "CapabilitySet", tmpJson);
        if(NULL != fileName) {
            ret = json_object_to_file(fileName, json);
        }

    }
    else {
        ret = -1;
    }

    if(NULL != json) {
		json_object_put(json);
        json = NULL;
	}
    if(NULL != tmpJson) {
		json_object_put(tmpJson);
        tmpJson = NULL;
	}

    return ret;

}

static struct json_object *capabilitySet_jsonObjectGet(struct json_object *json, const char *key, json_type type)
{
    struct json_object *tmpJson = NULL;

    if((NULL != json) && (NULL != key)) {
        tmpJson = json_object_object_get(json, key);
        if(NULL != tmpJson) {
            if(json_object_is_type(tmpJson, type)) {
                return tmpJson;
            }
        }
    }

    return NULL;

}

static int capabilitySet_jsonGetInt(struct json_object *json, const char *key)
{
    struct json_object *tmpJson = NULL;
    int ret = 0;

    tmpJson = capabilitySet_jsonObjectGet(json, key, json_type_int);
    if(NULL != tmpJson) {
        ret = json_object_get_int(tmpJson);
    }

    return ret;

}

static bool capabilitySet_jsonGetBoolean(struct json_object *json, const char *key)
{
    struct json_object *tmpJson = NULL;
    int ret = 0;

    tmpJson = capabilitySet_jsonObjectGet(json, key, json_type_boolean);
    if(NULL != tmpJson) {
        ret = json_object_get_boolean(tmpJson);
    }

    return ret;

}

static int capabilitySet_jsonToStruct(struct json_object *json, pstCAPABILITY_SET config)
{
    struct json_object *tmpJson = NULL;

	if((NULL != json) && (NULL != config))
	{
	    tmpJson = json_object_object_get(json, "CapabilitySet");
	    if(NULL != tmpJson)
	    {
            config->audioInput = capabilitySet_jsonGetBoolean(tmpJson, "audioInput");
            config->audioOutput = capabilitySet_jsonGetBoolean(tmpJson, "audioOutput");

            if(NULL != tmpJson)
            {
                json_object_put(tmpJson);
                tmpJson = NULL;
            }
            return 0;
	    }

    }

    return -1;

}

static int capabilitySet_structMatch(stCAPABILITY_SET srcConfig, pstCAPABILITY_SET dstConfig)
{
    int i = 0;
    int ret = 0;

    if(NULL != dstConfig) {
        dstConfig->version = srcConfig.version;
        dstConfig->audioInput = srcConfig.audioInput;
        dstConfig->audioOutput = srcConfig.audioOutput;
        ret = 0;
    }
    else {
        ret = -1;
    }

    return ret;

}

/**
 * [capabilitySet_jsonFileToStruct description]
 * @param  json_file 能力集json配置文件解析为结构体
 * @return           0成功/-1失败
 */
static int capabilitySet_jsonFileToStruct(char *json_file, pstCAPABILITY_SET config)
{
    int ret = -1;
    struct json_object *json = NULL;

    if((NULL != json_file) && (NULL != config)) {
        capabilitySet_structInit(config);
        json = json_object_from_file(json_file);
        if(NULL != json) {
            ret = capabilitySet_jsonToStruct(json, config);
            if(0 != ret) {
                printf("[%s:%d] capabilitySet parse json file %s to struct failed\n", __FUNCTION__, __LINE__, json_file);
            }
            else {
                stCapabilitySetAttr.stCapabilitySetInited = true;
            }
            json_object_put(json);
            json = NULL;
        }
        else {
            printf("[%s:%d] parse json file %s failed\n", __FUNCTION__, __LINE__, json_file);
        }
    }

    return ret;

}

/**
 * 查找和确认当前使用的配置文件
 * 此函数主要用来查找flash中的配置文件
 * @param  customFile 返回能力集json配置文件路径
 * @param  size      获取大小
 * @return           0成功|-1没有配置文件
 */
static bool capabilitySet_isTfCustom(char *customFile, unsigned int size)
{
    bool ret = false;

    if(NULL != customFile) {
        if(-1 != access(CAPABILITY_SET_CUSTOM_JSON, F_OK)) {
            snprintf(customFile, size, "%s", CAPABILITY_SET_CUSTOM_JSON);
            ret = true;
        }
        else {
            ret = false;
        }
    }
    else {
        ret = false;
    }

    return ret;

}

/**
 * 查找和确认当前使用的配置文件
 * 此函数主要用来查找flash中的配置文件
 * @param  file_path 返回能力集json配置文件路径
 * @param  size      获取大小
 * @return           0成功|-1没有配置文件
 */
static int capabilitySet_filePathGet(char *file_path, unsigned int size)
{
    int ret = 0;

    if(NULL == file_path) {
        return -1;
    }

	if(-1 != access(CAPABILITY_SET_JSON, F_OK)) {
		snprintf(file_path, size, "%s", CAPABILITY_SET_JSON);
	}
    else {
        ret = -1;
    }
    return ret;

}

/**
 * 能力集配置文件查找及解析到内存结构体
 * @return 0成功|-1失败
 */
int CAPABILITY_SET_init()
{
    int ret = -1;
    char filePath[64] = {0};

    stCapabilitySetAttr.stCapabilitySetInited = false;
    memset(filePath, 0, sizeof(filePath));

    /*
        先判断是否有tf卡定制文件,
        有则直接把tf卡配置文件读取到内存,
        然后再由内存保存到flash
    */
    if(true == capabilitySet_isTfCustom(filePath, sizeof(filePath))) {
        ret = capabilitySet_jsonFileToStruct(filePath, &stCapabilitySetAttr.stCapabilitySet);
        capabilitySet_structSaveFile(CAPABILITY_SET_JSON, stCapabilitySetAttr.stCapabilitySet);
    }

    // 获取flash中配置文件路径
    else if(0 == capabilitySet_filePathGet(filePath, sizeof(filePath))) {
        ret = capabilitySet_jsonFileToStruct(filePath, &stCapabilitySetAttr.stCapabilitySet);
    }
    else {
        printf("[%s:%d] capabilitySet config file not found!!!\n", __FUNCTION__, __LINE__);
    }

    return ret;

}

bool CAPABILITY_SET_is_inited()
{
    return stCapabilitySetAttr.stCapabilitySetInited;

}

/**
 * 能力集数据结构体获取
 * @param  capabilitySet 能力集数据结构体
 * @return        0成功|-1失败
 */
int CAPABILITY_SET_get(pstCAPABILITY_SET capabilitySet)
{
    if((NULL != capabilitySet) && (true == stCapabilitySetAttr.stCapabilitySetInited)) {
        return capabilitySet_structMatch(stCapabilitySetAttr.stCapabilitySet, capabilitySet);
    }

    return -1;

}

