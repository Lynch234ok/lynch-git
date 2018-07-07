#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "sysconf.h"
#include "sensor.h"
#include "netsdk.h"
#include "ptz.h"

#include "env_common.h"
#include "generic.h"
#include "app_debug.h"
#include "ifconf.h"

static int _get_system_information(lpNVP_DEV_INFO info)
{
	ST_NSDK_SYSTEM_DEVICE_INFO info_n;
	
	memset(&info_n, 0, sizeof(info_n));
	NETSDK_conf_system_get_device_info(&info_n);
	
	strcpy(info->manufacturer, "GUANGZHOU");
	strcpy(info->devname, info_n.deviceName);
	strcpy(info->model, info_n.model);
	strcpy(info->sn, info_n.serialNumber);
	strcpy(info->firmware, info_n.firmwareVersion);
	strcpy(info->sw_version, info_n.firmwareVersion);
	strcpy(info->sw_builddate, info_n.firmwareReleaseDate);
	strcpy(info->hw_version, info_n.hardwareVersion);
	strcpy(info->hwid, "HW000");

	return 0;
}

static int _get_date_time(lpNVP_SYS_TIME systime)
{
	time_t t_now;
	struct tm *ptm;
	struct tm tm_local, tm_gm;

	ST_NSDK_SYSTEM_TIME time_n;
	
	memset(&time_n, 0, sizeof(time_n));
	NETSDK_conf_system_get_time(&time_n);

	systime->ntp_enable = time_n.ntpEnabled;
	strncpy(systime->ntp_server, time_n.ntpServerDomain, sizeof(systime->ntp_server) - 1);
	systime->tzone = time_n.greenwichMeanTime;

	time(&t_now);
	localtime_r(&t_now, &tm_local);
	gmtime_r(&t_now, &tm_gm);

	ptm = &tm_local;
	systime->local_time.date.year = ptm->tm_year;
	systime->local_time.date.month = ptm->tm_mon;
	systime->local_time.date.day = ptm->tm_mday;
	systime->local_time.time.hour = ptm->tm_hour;
	systime->local_time.time.minute = ptm->tm_min;
	systime->local_time.time.second = ptm->tm_sec;
		
	ptm = &tm_gm;
	systime->gm_time.date.year = ptm->tm_year;
	systime->gm_time.date.month = ptm->tm_mon;
	systime->gm_time.date.day = ptm->tm_mday;
	systime->gm_time.time.hour = ptm->tm_hour;
	systime->gm_time.time.minute = ptm->tm_min;
	systime->gm_time.time.second = ptm->tm_sec;
	
	return 0;
}

static int _set_date_time(lpNVP_SYS_TIME systime)
{
	time_t t_set;
	struct timeval tv_set;
	struct tm tm_set;
	ST_NSDK_SYSTEM_TIME time_n;
	
	memset(&time_n, 0, sizeof(time_n));
	NETSDK_conf_system_get_time(&time_n);

	time_n.ntpEnabled = systime->ntp_enable;
	strncpy(time_n.ntpServerDomain, systime->ntp_server, sizeof(time_n.ntpServerDomain) - 1);
	time_n.greenwichMeanTime = systime->tzone;

	NETSDK_conf_system_set_time(&time_n);
	
	tm_set.tm_year = systime->gm_time.date.year;
	tm_set.tm_mon = systime->gm_time.date.month;
	tm_set.tm_mday = systime->gm_time.date.day;
	tm_set.tm_hour = systime->gm_time.time.hour;
	tm_set.tm_min = systime->gm_time.time.minute;
	tm_set.tm_sec = systime->gm_time.time.second;
	
	GMT_SET(0);
	t_set = mktime(&tm_set);
	
	tv_set.tv_sec = t_set;
	tv_set.tv_usec = 0;
	settimeofday(&tv_set, NULL);
	GMT_SET(time_n.greenwichMeanTime);
	
	APP_TRACE("set datetime zone:%d", time_n.greenwichMeanTime);
	
	return 0;
}


static int _get_interface(lpNVP_ETHER_CONFIG ether)
{
	ST_NSDK_NETWORK_INTERFACE net_n;
	ST_NSDK_NETWORK_PORT port_n;
	ST_NSDK_SYSTEM_DEVICE_INFO sysinfo;
	ifconf_interface_t irf;
	char *ether_iface = getenv("DEF_ETHER");
	
	memset(&net_n, 0, sizeof(net_n));
	memset(&port_n, 0, sizeof(port_n));
	memset(&sysinfo, 0, sizeof(sysinfo));
	NETSDK_conf_interface_get(1, &net_n);
	NETSDK_conf_port_get(1, &port_n);
	NETSDK_conf_system_get_device_info(&sysinfo);

	ifconf_get_interface(getenv("DEF_ETHER"), &irf);
	if (net_n.lan.addressingType == kNSDK_NETWORK_LAN_ADDRESSINGTYPE_STATIC) {
		ether->dhcp = false;
	} else {
		ether->dhcp = true;
	}
	memcpy(ether->ip, irf.ipaddr.s_b, sizeof(ether->ip));
	memcpy(ether->netmask, irf.netmask.s_b, sizeof(ether->netmask));
	memcpy(ether->gateway, irf.gateway.s_b, sizeof(ether->gateway));
	memcpy(ether->mac, irf.hwaddr.s_b, sizeof(ether->mac));

	NVP_IP_INIT_FROM_STRING(ether->dns1, net_n.dns.staticPreferredDns);
	NVP_IP_INIT_FROM_STRING(ether->dns2, net_n.dns.staticAlternateDns);	

	ether->http_port = port_n.value;
	ether->rtsp_port = port_n.value;

	return 0;
}

static int _set_interface(lpNVP_ETHER_CONFIG ether)
{
	ST_NSDK_NETWORK_INTERFACE net_n;
	ST_NSDK_NETWORK_PORT port_n;
	char *ether_iface = getenv("DEF_ETHER");

	memset(&net_n, 0, sizeof(net_n));
	memset(&port_n, 0, sizeof(port_n));
	NETSDK_conf_interface_get(1, &net_n);
	NETSDK_conf_port_get(1, &port_n);

	if (ether->dhcp == true) {
		net_n.lan.addressingType = kNSDK_NETWORK_LAN_ADDRESSINGTYPE_DYNAMIC;
	} else {
		net_n.lan.addressingType = kNSDK_NETWORK_LAN_ADDRESSINGTYPE_STATIC;
		_ip_2string(ether->ip, net_n.lan.staticIP);
		_ip_2string(ether->netmask, net_n.lan.staticNetmask);
		_ip_2string(ether->gateway, net_n.lan.staticGateway);
	}
	
	if(MATCH_GATEWAY(net_n.lan.staticIP, net_n.lan.staticNetmask, net_n.lan.staticGateway) == 0){
		snprintf(net_n.lan.staticGateway, sizeof(net_n.lan.staticGateway), "%d.%d.%d.%d", ether->ip[0] & ether->netmask[0], ether->ip[1] & ether->netmask[1],
			ether->ip[2] & ether->netmask[2], (ether->ip[3] & ether->netmask[3]) | 0x1);
	}
	APP_TRACE("_set_interface %s %s %s %s", ether_iface, net_n.lan.staticIP, net_n.lan.staticNetmask, net_n.lan.staticGateway);

	_ip_2string(ether->dns1, net_n.dns.staticPreferredDns); 	
	_ip_2string(ether->dns2, net_n.dns.staticAlternateDns);
	if (port_n.value != ether->http_port) {
		APP_TRACE("spook port %d -> %d", port_n.value, ether->http_port);
		port_n.value = ether->http_port;
	}

	if (NETSDK_conf_interface_set_by_delay(1, &net_n, eNSDK_CONF_SAVE_RESTART, 3)
		&& NETSDK_conf_port_set_by_delay(1, &port_n, eNSDK_CONF_SAVE_REBOOT, 3))
		return 0;

	return -1;
}

static int _get_color(lpNVP_COLOR_CONFIG color, int chn)
{
	ST_NSDK_VIN_CH vin_n;

	memset(&vin_n, 0, sizeof(vin_n));
	NETSDK_conf_vin_ch_get(chn + 1, &vin_n);

	color->brightness = (float)vin_n.brightnessLevel;
	color->contrast = (float)vin_n.contrastLevel;
	color->hue = (float)vin_n.hueLevel;
	color->saturation = (float)vin_n.saturationLevel;
	color->sharpness = (float)vin_n.sharpnessLevel;
	
	APP_TRACE("color (%d, %d, %d, %d, %d)", vin_n.brightnessLevel, vin_n.contrastLevel, vin_n.hueLevel, vin_n.saturationLevel, vin_n.sharpnessLevel);
	APP_TRACE("color (%f, %f, %f, %f, %f)", color->brightness, color->contrast, color->hue, color->saturation, color->sharpness);
	
	return 0;
}


static int _set_color(lpNVP_COLOR_CONFIG color, int chn)
{
	ST_NSDK_VIN_CH vin_n;
	
	memset(&vin_n, 0, sizeof(vin_n));
	NETSDK_conf_vin_ch_get(chn + 1, &vin_n);

	vin_n.brightnessLevel = color->brightness;
	vin_n.contrastLevel = color->contrast;
	vin_n.hueLevel = color->hue;
	vin_n.saturationLevel = color->saturation;
	vin_n.sharpnessLevel = color->sharpness;

	if (NETSDK_conf_vin_ch_set(chn + 1, &vin_n) ){		
		netsdk_vin_ch_set(chn + 1, &vin_n);
		return 0;
	}else
		return -1;
}

static int _get_image_option(lpNVP_IMAGE_OPTIONS image, int chn)
{
	// FIX me
	image->brightness.min = 0;
	image->brightness.max = 100;
	image->saturation.min = 0;
	image->saturation.max = 100;
	image->contrast.min = 0;
	image->contrast.max = 100;
	image->hue.min = 0;
	image->hue.max = 100;
	image->sharpness.min = 0;
	image->sharpness.max = 255;

	image->ircut_mode.nr = 3;
	image->ircut_mode.list[0] = NVP_IRCUT_MODE_AUTO;
	image->ircut_mode.list[1] = NVP_IRCUT_MODE_DAYLIGHT;
	image->ircut_mode.list[2] = NVP_IRCUT_MODE_NIGHT;

	return 0;
}

static int _get_image(lpNVP_IMAGE_CONFIG image, int chn)
{
	ST_NSDK_IMAGE img_n;

	memset(&img_n, 0, sizeof(img_n));
	NETSDK_conf_image_get(&img_n);

	image->ircut.control_mode = img_n.irCutFilter.irCutControlMode;
	image->ircut.ircut_mode = img_n.irCutFilter.irCutMode;

	image->wdr.enabled = img_n.wdr.enabled;
	image->wdr.WDRStrength = img_n.wdr.WDRStrength;

	image->manual_sharp.enabled = img_n.manualSharpness.enabled;
	image->manual_sharp.sharpnessLevel = img_n.manualSharpness.sharpnessLevel;

	image->d3d.enabled = img_n.denoise3d.enabled;
	image->d3d.denoise3dStrength = img_n.denoise3d.denoise3dStrength;

	_get_color(&image->color, chn);

	_get_image_option(&image->option, chn);
	
	return 0;
}

static int _set_image(lpNVP_IMAGE_CONFIG image, int chn)
{
	ST_NSDK_IMAGE img_n;
	ST_NSDK_VIN_CH vin_n;

	memset(&img_n, 0, sizeof(img_n));
	memset(&vin_n, 0, sizeof(vin_n));
	NETSDK_conf_vin_ch_get(chn + 1, &vin_n);
	NETSDK_conf_image_get(&img_n);

	vin_n.brightnessLevel = image->color.brightness;
	vin_n.contrastLevel = image->color.contrast;
	vin_n.hueLevel = image->color.hue;
	vin_n.saturationLevel = image->color.saturation;
	vin_n.sharpnessLevel = image->color.sharpness;

	img_n.irCutFilter.irCutControlMode = image->ircut.control_mode;
	img_n.irCutFilter.irCutMode = image->ircut.ircut_mode;

	img_n.manualSharpness.enabled = image->manual_sharp.enabled;
	img_n.manualSharpness.sharpnessLevel = image->manual_sharp.sharpnessLevel;

	img_n.wdr.enabled = image->wdr.enabled;
	img_n.wdr.WDRStrength = image->wdr.WDRStrength;

	img_n.denoise3d.enabled = image->d3d.enabled;
	img_n.denoise3d.denoise3dStrength = image->d3d.denoise3dStrength;

	if (NETSDK_conf_vin_ch_set(chn + 1, &vin_n) ){
		netsdk_vin_ch_set(chn + 1, &vin_n);
		if (NETSDK_conf_image_set(&img_n)) {
			netsdk_image_changed(&img_n);
			return 0;
		}
	}
	return -1;
}

static int _get_video_source(lpNVP_V_SOURCE src, int chn)
{
	ST_NSDK_VIN_CH vin_n;

	memset(&vin_n, 0, sizeof(vin_n));
	NETSDK_conf_vin_ch_get(chn + 1, &vin_n);

	src->resolution.width = vin_n.captureWidth;
	src->resolution.height = vin_n.captureHeight;	

	src->fps = vin_n.captureFrameRate;

	_get_image(&src->image, chn);
	
	return 0;
}

static int _set_video_source(lpNVP_V_SOURCE src, int chn)
{	
	return _set_image(&src->image, chn);
}


static int _get_video_input_conf(lpNVP_VIN_CONFIG vin, int chn)
{
	ST_NSDK_VIN_CH vin_n;

	memset(&vin_n, 0, sizeof(vin_n));
	NETSDK_conf_vin_ch_get(chn + 1, &vin_n);

	vin->rect.nX = 0;
	vin->rect.nY = 0;
	vin->rect.width = vin_n.captureWidth;
	vin->rect.height = vin_n.captureHeight;	

	if (vin_n.flip || vin_n.mirror)
		vin->rotate.enabled = true;
	else
		vin->rotate.enabled = false;
	
	vin->rotate.degree = 0;
	if (vin_n.mirror)
		vin->rotate.degree += 90;
	if (vin_n.flip)
		vin->rotate.degree += 180;
	
	return 0;
}

static int _set_video_input_conf(lpNVP_VIN_CONFIG vin, int chn)
{	
	ST_NSDK_VIN_CH vin_n;

	memset(&vin_n, 0, sizeof(vin_n));
	NETSDK_conf_vin_ch_get(chn + 1, &vin_n);

	vin_n.captureWidth = vin->rect.width;
	vin_n.captureHeight = vin->rect.height;	

	if (vin->rotate.enabled) {
		if (vin->rotate.degree == NVP_ROTATE_MIRROR) {
			vin_n.mirror = true;
		} else if (vin->rotate.degree == NVP_ROTATE_FLIP) {
			vin_n.flip = true;
		} else if (vin->rotate.degree == NVP_ROTATE_FLIP_MIRROR) {
			vin_n.flip = true;
			vin_n.mirror = true;
		} else {
			APP_TRACE("unsupported rotate degree: %d", vin->rotate.degree);
			return -1;
		}
	}

	if (NETSDK_conf_vin_ch_set(chn + 1, &vin_n) ) {
		netsdk_vin_ch_set(chn + 1, &vin_n);
		return 0;
	}else
		return -1;
}

static int _get_video_encode_option(lpNVP_VENC_OPTIONS venc, int chn)
{
	ST_NSDK_VIN_CH vin_n;

	memset(&vin_n, 0, sizeof(vin_n));
	NETSDK_conf_vin_ch_get(chn + 1, &vin_n);
	// FIM Me
	venc->enc_fps.min = 3;
	venc->enc_fps.max = 30;

	venc->enc_gov.min = 1;
	venc->enc_gov.max = 30;

	venc->enc_interval.min = 1;
	venc->enc_interval.max = 1;
	
	venc->enc_quality.min = 0;
	venc->enc_quality.max = 4;

	if (venc->index == 0) {		
		venc->enc_bps.min  = 512;
		venc->enc_bps.max = 5000;

		venc->resolution_nr = 7;
		NVP_SET_SIZE(&venc->resolution[0], 640, 480);
		NVP_SET_SIZE(&venc->resolution[1], 720, 480);
		NVP_SET_SIZE(&venc->resolution[2], 720, 576);
		NVP_SET_SIZE(&venc->resolution[3], 1024, 768);
		NVP_SET_SIZE(&venc->resolution[4], 1280, 720);
		NVP_SET_SIZE(&venc->resolution[5], 1280, 960);
		NVP_SET_SIZE(&venc->resolution[6], 1920, 1080);
	} else if (venc->index == 1) {
		venc->enc_bps.min  = 32;
		venc->enc_bps.max = 1500;

		venc->resolution_nr = 5;
		NVP_SET_SIZE(&venc->resolution[0], 320, 240);
		NVP_SET_SIZE(&venc->resolution[1], 352, 240);
		NVP_SET_SIZE(&venc->resolution[2], 352, 288);
		NVP_SET_SIZE(&venc->resolution[3], 640, 360);
		NVP_SET_SIZE(&venc->resolution[4], 640, 480);
	} else {
		venc->enc_bps.min  = 32;
		venc->enc_bps.max = 512;

		venc->resolution_nr = 6;
		NVP_SET_SIZE(&venc->resolution[0], 160, 120);
		NVP_SET_SIZE(&venc->resolution[1], 176, 144);
		NVP_SET_SIZE(&venc->resolution[2], 320, 180);
		NVP_SET_SIZE(&venc->resolution[3], 320, 240);
		NVP_SET_SIZE(&venc->resolution[4], 352, 240);
		NVP_SET_SIZE(&venc->resolution[5], 352, 288);
	}

	venc->enc_profile_nr = 1;
	venc->enc_profile[0] = NVP_H264_PROFILE_MAIN;

	return 0;
}

static int _get_video_encode(lpNVP_VENC_CONFIG venc, int chn)
{
	ST_NSDK_VENC_CH venc_n;

	memset(&venc_n, 0, sizeof(venc_n));
	NETSDK_conf_venc_ch_get((chn + 1)*100+ venc->index + 1, &venc_n);

	if (venc_n.freeResolution == true) {
		venc->width = venc_n.resolutionWidth;
		venc->height =  venc_n.resolutionHeight;
	} else {
		venc->width = (venc_n.resolution >> 16) & 0xffff;
		venc->height =  venc_n.resolution & 0xffff;
	}
	venc->enc_bps = venc_n.constantBitRate;
	venc->enc_fps = venc_n.frameRate;
	venc->enc_gov = venc_n.keyFrameInterval;
	venc->enc_interval = 1;
	if (venc_n.bitRateControlType == kNSDK_BR_CONTROL_CBR) {
		venc->quant_mode = NVP_QUANT_CBR;
	} else {
		venc->quant_mode = NVP_QUANT_VBR;
	}

	if (venc_n.codecType == kNSDK_CODEC_TYPE_H264)
		venc->enc_type = NVP_VENC_H264;
	else {
		//APP_ASSERT(0, "err: unknown code type: %d  @id=%d,%d", venc_n.codecType, chn, venc->index);
	}
	// FIX me
	venc->enc_profile = venc_n.h264Profile - 1;//NVP_H264_PROFILE_MAIN;
	if (venc->index == 0)
		venc->user_count = 4;
	else
		venc->user_count = 6;
	
	venc->option.index = venc->index;
	strcpy(venc->option.token, venc->token);
	strcpy(venc->option.enc_token, venc->enc_token);
	_get_video_encode_option(&venc->option, chn);

	return 0;
}

static int _set_video_encode(lpNVP_VENC_CONFIG venc, int chn)
{
	ST_NSDK_VENC_CH venc_n;

	memset(&venc_n, 0, sizeof(venc_n));
	NETSDK_conf_venc_ch_get((chn + 1)*100+ venc->index + 1, &venc_n);
	
	if (venc_n.freeResolution == true) {
		 venc_n.resolutionWidth = venc->width;
		 venc_n.resolutionHeight = venc->height;
	} else {
		venc_n.resolution = ((venc->width & 0xffff) << 16) | (venc->height & 0xffff);
	}
	if (venc->quant_mode == NVP_QUANT_CBR) {
		venc_n.bitRateControlType = kNSDK_BR_CONTROL_CBR;
	} else {
		venc_n.bitRateControlType = kNSDK_BR_CONTROL_VBR;
	}
	venc_n.frameRate = venc->enc_fps;
	venc_n.constantBitRate = venc->enc_bps;
	venc_n.keyFrameInterval = venc->enc_gov;
	if (venc->enc_type == NVP_VENC_H264) {
		venc_n.codecType = kNSDK_CODEC_TYPE_H264;
	} else {		
		APP_TRACE("err: unsupported code type: %d", venc->enc_type);
		return -1;
	}
	
	if (NETSDK_conf_venc_ch_set((chn + 1)*100+ venc->index + 1, &venc_n)){
	//	NETSDK_venc_ch_delay_set((chn + 1)*100+ venc->index + 1, (void*)&venc_n);
		netsdk_venc_ch_changed((chn + 1)*100+ venc->index + 1, (LP_NSDK_VENC_CH)&venc_n);
		return 0;
	}else
		return -1;
}

static int _get_audio_input(lpNVP_AIN_CONFIG ain, int chn)
{
	return 0;
}

static int _set_audio_input(lpNVP_AIN_CONFIG ain, int chn)
{
	return -1;
}


static int _get_audio_encode(lpNVP_AENC_CONFIG aenc, int chn)
{
	//FIX me
	aenc->channel = 1;
	aenc->enc_type = NVP_AENC_G711;
	aenc->sample_size = 8;
	aenc->sample_rate = 8000;
	
	aenc->user_count = 2;
	return 0;
}

static int _set_audio_encode(lpNVP_AENC_CONFIG aenc, int chn)
{
	return -1;
}

static int _get_motion_detection(lpNVP_MD_CONFIG md, int chn)
{
	ST_NSDK_MD_CH md_n;
	
	memset(&md_n, 0, sizeof(md_n));
	NETSDK_conf_md_ch_get(chn + 1, &md_n);

	if (md_n.detectionType == kNSDK_MD_TYPE_GRID) {
		md->type = NVP_MD_TYPE_GRID;

		md->grid.columnGranularity = md_n.detectionGrid.columnGranularity;
		md->grid.rowGranularity = md_n.detectionGrid.rowGranularity;
		md->grid.sensitivity = md_n.detectionGrid.sensitivityLevel;
		memcpy(md->grid.granularity, md_n.detectionGrid.granularity, sizeof(md->grid.granularity));
		//FIM me
		md->grid.threshold = 5;
	}else {
		md->type = NVP_MD_TYPE_REGION;

		// FIX me
		APP_TRACE("unexpected md region mode!!!!");
	}

	// FIX me
	md->delay_off_alarm = 300;
	md->delay_on_alarm = 200;
	
	return 0;
}

static int _set_motion_detection(lpNVP_MD_CONFIG md, int chn)
{	
	ST_NSDK_MD_CH md_n;
	
	memset(&md_n, 0, sizeof(md_n));
	NETSDK_conf_md_ch_get(1, &md_n);

	md_n.enabled = true;	
	//if (md->type == NVP_MD_TYPE_GRID) {
		md_n.detectionType = kNSDK_MD_TYPE_GRID;
		md_n.detectionGrid.columnGranularity = md->grid.columnGranularity;
		md_n.detectionGrid.rowGranularity = md->grid.rowGranularity;
		md_n.detectionGrid.sensitivityLevel = md->grid.sensitivity;
		memcpy(md_n.detectionGrid.granularity, md->grid.granularity, sizeof(md_n.detectionGrid.granularity));
	//} else {
	//	md_n.detectionType = kNSDK_MD_TYPE_REGION;
	//	// FIX me
	//	APP_TRACE("unexpected md region mode!!!!");
	//}

	NETSDK_conf_md_ch_set(1, &md_n);
	
	return 0;
}

static int _get_video_analytic(lpNVP_VAN_CONFIG van, int chn)
{
	return 0;
}

static int _set_video_analytic(lpNVP_VAN_CONFIG van, int chn)
{
	return 0;
}


static int _get_ptz(lpNVP_PTZ_CONFIG ptz, int chn)
{
	return 0;
}

static int _set_ptz(lpNVP_PTZ_CONFIG ptz, int chn)
{
	return 0;
}

static int _get_profile(lpNVP_PROFILE_CHN profile)
{
	int i;
	// FIX me
	profile->profile_nr = 2;
	profile->venc_nr = 2;
	profile->aenc_nr = 1;
	//
	for (i = 0; i < profile->venc_nr; i++) {
		_get_video_encode(&profile->venc[i], profile->index);
	}
	for (i = 0; i < profile->aenc_nr; i++) {
		_get_audio_encode(&profile->aenc[i], profile->index);
	}
	_get_video_source(&profile->v_source, profile->index);
	for (i = 0; i < profile->vin_conf_nr; i++) {
		_get_video_input_conf(&profile->vin[i], profile->index);
	}
	_get_audio_input(&profile->ain, profile->index);
	_get_ptz(&profile->ptz, profile->index);
	_get_video_analytic(&profile->van, profile->index);
	_get_motion_detection(&profile->md, profile->index);

	return 0;
}

static int _set_profile(lpNVP_PROFILE_CHN profile)
{
	int i;
	int ret = 0;
	//
	for (i = 0; i < profile->venc_nr; i++) {
		if (_set_video_encode(&profile->venc[i], profile->index) < 0)
			ret = -1;
	}
	for (i = 0; i < profile->aenc_nr; i++) {
		if (_set_audio_encode(&profile->aenc[i], profile->index) < 0)
			ret = -1;
	}
	if (_set_video_source(&profile->v_source, profile->index) < 0)
		ret = -1;
	for (i = 0; i < profile->vin_conf_nr; i++) {
		if (_set_video_input_conf(&profile->vin[i], profile->index) < 0)
			ret = -1;
	}
	if (_set_audio_input(&profile->ain, profile->index) < 0)
		ret = -1;
	if (_set_ptz(&profile->ptz, profile->index) < 0)
		ret = -1;
	if (_set_video_analytic(&profile->van, profile->index) < 0)
		ret = -1;
	if (_set_motion_detection(&profile->md, profile->index) < 0)
		ret = -1;

	return ret;
}

static int _get_profiles(lpNVP_PROFILE profiles)
{
	int i;

	profiles->chn = NVP_MAX_CH;
	//
	for ( i = 0; i < profiles->chn; i++) {
		_get_profile(&profiles->profile[i]);
	}
	return 0;
}

static int _set_profiles(lpNVP_PROFILE profiles)
{
	int i;
	int ret = 0;
	//
	for ( i = 0; i < profiles->chn; i++) {
		if(_set_profile(&profiles->profile[i]) < 0)
			ret = -1;
	}
	return ret;
}


static int _get_all(lpNVP_ENV env)
{
	_get_system_information(&env->devinfo);
	_get_date_time(&env->systime);
	_get_interface(&env->ether);
	_get_profiles(&env->profiles);

	return 0;
}

static int _set_all(lpNVP_ENV env)
{
	int ret = 0;

	if (_get_system_information(&env->devinfo) < 0)
		ret = -1;
	if (_get_date_time(&env->systime) < 0)
		ret = -1;
	if (_get_interface(&env->ether) < 0)
		ret = -1;
	if (_get_profiles(&env->profiles) < 0)
		ret = -1;

	return ret;	
}

/********************************************************************************
* system command interfaces
*********************************************************************************/
static void _cmd_system_boot(long l, void *r)
{
	TICKER_del_task(_cmd_system_boot);
	APP_TRACE("system reboot now...");
	exit(0);
}

static int _cmd_ptz(lpNVP_CMD cmd, const char *module, int keyid)
{
	const char *ptz_cmd_name[] = 
	{
		"PTZ_CMD_UP",
		"PTZ_CMD_DOWN",
		"PTZ_CMD_LEFT",
		"PTZ_CMD_RIGHT",
		"PTZ_CMD_LEFT_UP",
		"PTZ_CMD_RIGHT_UP",
		"PTZ_CMD_LEFT_DOWN",
		"PTZ_CMD_RIGHT_DOWN",
		"PTZ_CMD_AUTOPAN",
		"PTZ_CMD_IRIS_OPEN",
		"PTZ_CMD_IRIS_CLOSE",
		"PTZ_CMD_ZOOM_IN",
		"PTZ_CMD_ZOOM_OUT",
		"PTZ_CMD_FOCUS_FAR",
		"PTZ_CMD_FOCUS_NEAR",
		"PTZ_CMD_STOP",
		"PTZ_CMD_WIPPER_ON",
		"PTZ_CMD_WIPPER_OFF",
		"PTZ_CMD_LIGHT_ON",
		"PTZ_CMD_LIGHT_OFF",
		"PTZ_CMD_POWER_ON",
		"PTZ_CMD_POWER_OFF",
		"PTZ_CMD_GOTO_PRESET",
		"PTZ_CMD_SET_PRESET",
		"PTZ_CMD_CLEAR_PRESET",
		"PTZ_CMD_TOUR",
	};
	int speed = (int)cmd->ptz.speed * 64;
	int ret = -1;

	APP_TRACE("%s(%d)", ptz_cmd_name[cmd->ptz.cmd], cmd->ptz.cmd);

	switch(cmd->ptz.cmd) 
	{
		case NVP_PTZ_CMD_LEFT:			
			ret = PTZ_Send(0, PTZ_CMD_LEFT, speed);
			break;
		case NVP_PTZ_CMD_RIGHT:
			ret = PTZ_Send(0, PTZ_CMD_RIGHT, speed);
			break;
		case NVP_PTZ_CMD_UP:
			ret = PTZ_Send(0, PTZ_CMD_UP, speed);
			break;
		case NVP_PTZ_CMD_DOWN:
			ret = PTZ_Send(0, PTZ_CMD_DOWN, speed);
			break;
		case NVP_PTZ_CMD_ZOOM_IN:
			ret = PTZ_Send(0, PTZ_CMD_ZOOM_IN, speed);
			break;
		case NVP_PTZ_CMD_ZOOM_OUT:
			ret = PTZ_Send(0, PTZ_CMD_ZOOM_OUT, speed);
			break;
		case NVP_PTZ_CMD_SET_PRESET:			
			ret = PTZ_Send(0, PTZ_CMD_SET_PRESET, cmd->ptz.index);
			break;
		case NVP_PTZ_CMD_GOTO_PRESET:
			ret = PTZ_Send(0, PTZ_CMD_GOTO_PRESET, cmd->ptz.index);
			break;
		case NVP_PTZ_CMD_CLEAR_PRESET:
			ret = PTZ_Send(0, PTZ_CMD_CLEAR_PRESET, cmd->ptz.index);
			break;
		case NVP_PTZ_CMD_STOP:
			ret = PTZ_Send(0, PTZ_CMD_STOP, 0);
			break;
		default:
			break;
	}

	return ret;
}


/********************************************************************************
* external interfaces
*********************************************************************************/


int NVP_env_load(lpNVP_ENV env, const char *module, int keyid)
{
	char temp[512];
	char *ptr = NULL, *pbuf;
	char *saveptr = NULL;
	int ret;
	int chn, id;

	chn = keyid/100;
	id = keyid%100;
	strncpy(temp, module, 512);
	pbuf = temp;
	while((ptr = strtok_r(pbuf, OM_AND, &saveptr)) != NULL)
	{
		if (OM_MATCH(ptr, OM_ALL)) {
			ret = _get_all(env);
			break;
		}else if (OM_MATCH(ptr, OM_PROFILE)) {
			ret = _get_profile(&env->profiles.profile[chn]);
		}else if (OM_MATCH(ptr, OM_PROFILES)) {
			ret = _get_profiles(&env->profiles);
		}else if (OM_MATCH(ptr, OM_INFO)) {
			ret = _get_system_information(&env->devinfo);
		}else if (OM_MATCH(ptr, OM_DTIME)) {
			ret = _get_date_time(&env->systime);
		}else if (OM_MATCH(ptr, OM_NET)) {
			ret = _get_interface(&env->ether);
		}  else if (OM_MATCH(ptr, OM_VENC)) {
			ret = _get_video_encode(&env->profiles.profile[chn].venc[id], chn);
		}  else if (OM_MATCH(ptr, OM_VSRC)) {
			ret = _get_video_source(&env->profiles.profile[chn].v_source, chn);
		}  else if (OM_MATCH(ptr, OM_VINC)) {
			ret = _get_video_input_conf(&env->profiles.profile[chn].vin[id], chn);
		}  else if (OM_MATCH(ptr, OM_AENC)) {
			ret = _get_audio_encode(&env->profiles.profile[chn].aenc[id], chn);
		}  else if (OM_MATCH(ptr, OM_AIN)) {
			ret = _get_audio_input(&env->profiles.profile[chn].ain, chn);
		}  else if (OM_MATCH(ptr, OM_COLOR)) {
			ret = _get_color(&env->profiles.profile[chn].v_source.image.color, chn);			
		}  else if (OM_MATCH(ptr, OM_IMG)) {
			ret = _get_image(&env->profiles.profile[chn].v_source.image, chn);
		}  else if (OM_MATCH(ptr, OM_MD)) {
			ret = _get_motion_detection(&env->profiles.profile[chn].md, chn);
		}  else if (OM_MATCH(ptr, OM_PTZ)) {
			ret = _get_ptz(&env->profiles.profile[chn].ptz, chn);
		} else {
			APP_TRACE("unknown env module: %s", ptr);
		}
		//
		pbuf = NULL;
	}

	return 0;
}

int NVP_env_save(lpNVP_ENV env, const char *module, int keyid)
{
	char temp[512];
	char *ptr = NULL, *pbuf;
	char *saveptr = NULL;
	int ret;
	int f_ret = 0;
	int chn, id;

	chn = keyid / 100;
	id = keyid % 100;
	strncpy(temp, module, 512);
	pbuf = temp;
	while((ptr = strtok_r(pbuf, OM_AND, &saveptr)) != NULL)
	{
		if (OM_MATCH(ptr, OM_ALL)) {
			ret = _set_all(env);
			break;
		}else if (OM_MATCH(ptr, OM_PROFILE)) {
			ret = _set_profile(&env->profiles.profile[chn]);
		}else if (OM_MATCH(ptr, OM_PROFILES)) {
			ret = _set_profiles(&env->profiles);
		}else if (OM_MATCH(ptr, OM_INFO)) {
			//
		}else if (OM_MATCH(ptr, OM_DTIME)) {
			ret = _set_date_time(&env->systime);
		}else if (OM_MATCH(ptr, OM_NET)) {
			ret = _set_interface(&env->ether);
		}  else if (OM_MATCH(ptr, OM_VENC)) {
			ret = _set_video_encode(&env->profiles.profile[chn].venc[id], chn);
		}  else if (OM_MATCH(ptr, OM_VSRC)) {
			ret = _set_video_source(&env->profiles.profile[chn].v_source, chn);
		}  else if (OM_MATCH(ptr, OM_VINC)) {
			ret = _set_video_input_conf(&env->profiles.profile[chn].vin[id], chn);
		}  else if (OM_MATCH(ptr, OM_AENC)) {
			ret = _set_audio_encode(&env->profiles.profile[chn].aenc[id], chn);
		}  else if (OM_MATCH(ptr, OM_AIN)) {
			ret = _set_audio_input(&env->profiles.profile[chn].ain, chn);
		}  else if (OM_MATCH(ptr, OM_COLOR)) {
			ret = _set_color(&env->profiles.profile[chn].v_source.image.color, chn);			
		}  else if (OM_MATCH(ptr, OM_IMG)) {
			ret = _set_image(&env->profiles.profile[chn].v_source.image, chn);
		}  else if (OM_MATCH(ptr, OM_MD)) {
			ret = _set_motion_detection(&env->profiles.profile[chn].md, chn);
		}  else if (OM_MATCH(ptr, OM_PTZ)) {
			ret = _set_ptz(&env->profiles.profile[chn].ptz, chn);
		} else {
			APP_TRACE("unknown env module: %s", ptr);
			f_ret = -1;
		}

		if (ret < 0)
			f_ret = -1;

		//
		pbuf = NULL;
	}

	return f_ret;
}

int NVP_env_cmd(lpNVP_CMD cmd, const char *module, int keyid)
{
	char temp[512];
	char *ptr = NULL, *pbuf;
	char *saveptr = NULL;
	int ret;
	int f_ret = 0;
	int chn, id;

	chn = keyid / 100;
	id = keyid % 100;
	strncpy(temp, module, 512);
	pbuf = temp;
	while((ptr = strtok_r(pbuf, OM_AND, &saveptr)) != NULL)
	{
		if (OM_MATCH(ptr, OM_REBOOT)) {
			ret = TICKER_add_task(_cmd_system_boot, 1, false);
		}else if (OM_MATCH(ptr, OM_SYS_RESET)) {
			APP_TRACE("unknown env module: %s", ptr);
		}else if (OM_MATCH(ptr, OM_PTZ)) {
			ret = _cmd_ptz(cmd, module, keyid);
		} else {
			APP_TRACE("unknown env module: %s", ptr);
			f_ret = -1;
		}

		if (ret < 0)
			f_ret = -1;

		//
		pbuf = NULL;
	}

	return f_ret;
}

