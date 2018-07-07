#include "production_test.h"
#include "inifile.h"
#include "app_debug.h"

#define PRODUCT_TEST_INFO_FILE_PATH "/media/tf/production_test.ini"

static LP_PRODUCT_TEST_INFO product_info = NULL;

static void product_info_dump(LP_PRODUCT_TEST_INFO info)
{
	APP_TRACE("enter to SDcard info getting");
	APP_TRACE("ESSID:%s", info->staEssid);
	APP_TRACE("PASSWORD:%s", info->staPassword);
	APP_TRACE("IP:%s", info->staStaticIp);
	APP_TRACE("netmask:%s", info->staNetmask);
	APP_TRACE("gateway:%s", info->staGateway);
}

LP_PRODUCT_TEST_INFO PRODUCT_TEST_get_info(LP_PRODUCT_TEST_INFO info)
{
	if(product_info){
		memcpy((void *)info, (void *)product_info, sizeof(ST_PRODUCT_TEST_INFO));
		return product_info;
	}else{
		return NULL;
	}
}

int PRODUCT_TEST_init()
{
	lpINI_PARSER inf = NULL;
	product_info = (LP_PRODUCT_TEST_INFO)calloc(sizeof(ST_PRODUCT_TEST_INFO), 1);
	inf = OpenIniFile(PRODUCT_TEST_INFO_FILE_PATH);
	if(inf){
		if(product_info){
			inf->read_text(inf, "wifi", "ssid", "", product_info->staEssid, sizeof(product_info->staEssid));
			inf->read_text(inf, "wifi", "psk", "", product_info->staPassword, sizeof(product_info->staPassword));
			inf->read_text(inf, "wifi", "ip", "", product_info->staStaticIp, sizeof(product_info->staStaticIp));
			sprintf(product_info->staNetmask, "255.255.0.0");
			inf->read_text(inf, "wifi", "gw", "", product_info->staGateway, sizeof(product_info->staGateway));
			product_info_dump(product_info);
		}
		CloseIniFile(inf);
		inf = NULL;
		return 0;
	}else{
		if(product_info){
			free(product_info);
			product_info = NULL;
		}
		return -1;
	}
}

int PRODUCT_TEST_destroy()
{
	if(product_info){
		free(product_info);
		product_info = NULL;
		return 0;
	}
	return -1;
}