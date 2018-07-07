
#include <device_binding/device_binding.h>
#include <base64.h>
#include "ipcam_network.h"

//#include "sysenv.h"
#include "sysconf.h"
#include "ifconf.h"

#include "spook/spook.h"
#include "spook/owsp.h"
//#include "spook/bubble.h"
#include "bubble.h"
//#include "spook/rtspd.h"
//#include "spook/httpd.h"
#include "spook/minirtsp.h"
#include "spook/rtmps.h"
#include "spook/regRW.h"

#include "httpd.h"
#include "rtmpd.h"
#include "hls.h"
#include "onvif.h"
#include "spook/onvif_spook.h"
#include "env_common.h"

#include "gw.h"
#include "ntp.h"
#include "upnp.h"
#include "ddns.h"
#include "esee_client.h"
#include "hichip.h"
#include "smtp.h"
#include "securedat.h"
#include "live555.h"
#include "overlay.h"
#include "rudpa.h"
#include "p2p.h"
#include "cgi.h"
#include "generic.h"
#include "lonse.h"
#include "cgi_user.h"

#include "gb28181.h"
#include "web_server.h"

#include "cgi_bin.h"
#include "cgi_user_1.h"

#include "netsdk.h"
#include "hichipv10.h"
#include "hikvisionv10.h"
#include "piecake.h"
#include "wechat.h"
#include "app_debug.h"

#include "app_ftp.h"
#include "wechat_client.h"

#include "wifi/wifi.h"
#include "dana_lib.h"
#include "net_dhcp.h"

#include "usrm.h"
#include "http_auth/authentication.h"

#include "global_runtime.h"
#include "app_msg_push.h"
#include "ticker.h"
#include "n1/corsee.h"
#include "remote_upgrade.h"
#include "version.h"
#include "sound.h"
#include "NkUtils/types.h"
#include "base/ja_type.h"
#include "base/ja_process.h"
#include "bsp/keytime.h"
#include "network_interface.h"
#include "custom.h"
#include "production_test.h"
#include "app_video_ctrl.h"
#include "ipcam_timer.h"
#include "tfcard.h"
#include "wifi/ja_wifi_seek.h"
#include "app_wifi.h"
#include "p2p/p2pdevice.h"
#include <secure_chip.h>

#if defined(TFCARD)
#include "app_tfcard.h"
	extern TF_RL_Pack_t gRL;
#endif
#include "wifi/ja_wifi_seek.h"

#include "base64.h"
//#include "nk_aes.h"
#include "app_wifi.h"
#include "nk_ip_adapt.h"

/// Two Way Talk Temporary, Frank
#include "n1_device_2waytalk.h"

//network motion detection 
void IPCAM_network_md_handle(int vin, const char *rect_name)
{
	int const videoInputID = vin + 1;
	ST_NSDK_MD_CH sinfo={0};

	NETSDK_conf_md_ch_get(1,&sinfo);

	if(sinfo.enabled){
		APP_NETSDK_mark_motion_detection(videoInputID);
#if defined(ONVIFNVT)
		ONVIF_notify_event(JA_EVENT_MD);
		ONVIF_notify_event(JA_EVENT_MD_EX);
#endif//defined(ONVIFNVT)

#if defined(ANTS)
		// ants md notify
		ANTSLIB_notify_md(vin);
#endif//defined(ANTS)

		HICHIP_set_md_status(true);
		//APP_TRACE("VIN(%d) motion occurs", vin);

#if defined(DANALE)
		//dana protocol MD event
		Dana_notify_event();
#endif

#if defined(TFCARD)
		if(TFcard_motion_record_isenabled()){
			gRL.st_TF_record_parameter.motion_check = 1;
		}
#endif
	//put the image to wechat's fans!!
	//WECHAT_CLIENT_put_image();
	}

#if defined(MSG)
	ST_NSDK_SYSTEM_SETTING info;
	NETSDK_conf_system_get_setting_info(&info);
	if(info.messagePushEnabled && schedule_messagePush(info)){
		ESEE_msg_push();
	}
#endif
	
#if defined(VIDEO_CTRL)
		VIDEO_CTRL_turnup_event(VIDEO_CTRL_TURNUP_EVENT_MOTION);
#endif	
}


//onvif discovery callback
void get_ip_callback(char *origin_ip, char *origin_netmask, char *szip, char *szmac)
{
	char def_eth[128],shell_cmd[64];
	ifconf_interface_t intrface;
	ST_NSDK_NETWORK_INTERFACE _interface;

	if(NULL == getenv("DEF_ETHER")){
		snprintf(def_eth, sizeof(def_eth), "eth0");
	}else{
		snprintf(def_eth, sizeof(def_eth), "%s", getenv("DEF_ETHER"));
	}

	snprintf(shell_cmd, sizeof(shell_cmd), "ifconfig %s %s netmask 255.255.255.0", def_eth, szip);
	NK_SYSTEM(shell_cmd);

	nk_net_adapt_ip_set_pause_flag(time(NULL), 60);

	if (strcmp("eth0", def_eth) == 0) {
		NETSDK_conf_interface_get(1, &_interface);
		sprintf(_interface.lan.staticIP, "%s", szip);
		//fix me:stream connection disconnected while ip changed
		//sprintf(shell_cmd, "ifconfig eth0 %s", szip);
		//NK_SYSTEM(shell_cmd);
		//usleep(200*1000);
		intrface.ipaddr.s_addr = inet_addr(szip);
		intrface.ipaddr.s_b4 = 1;
		sprintf(_interface.lan.staticGateway,"%s", ifconf_ipv4_ntoa(intrface.ipaddr));
		NETSDK_conf_interface_set(1, &_interface, eNSDK_CONF_SAVE_RESTART);
		//usleep(500*1000);
		//NETSDK_conf_interface_set(1, &eth0, true);
	} else {
		NETSDK_conf_interface_get(4, &_interface);
		sprintf(_interface.lan.staticIP, "%s", szip);
		intrface.ipaddr.s_addr = inet_addr(szip);
		intrface.ipaddr.s_b4 = 1;
		sprintf(_interface.lan.staticGateway,"%s", ifconf_ipv4_ntoa(intrface.ipaddr));
		NETSDK_conf_interface_set(4, &_interface, eNSDK_CONF_SAVE_RESTART);
	}
}

static int network_onvif_wsdd_event_hook(char *type, char *xaddr, char *scopes ,int event, void *customCtx)
{
#if !defined(DHCP)
	char ipstr[64]={0};

	if (event == WSDD_EVENT_PROBE) {
		printf("got wsdd probe from:%s type:%s\n", xaddr, type);
		if (nk_net_adapt_ip_is_enable()) {
			ifconf_interface_t ifconf_ifr;
			char def_eth[128];

			if(NULL == getenv("DEF_ETHER")){
				snprintf(def_eth, sizeof(def_eth), "eth0");
			}else{
				snprintf(def_eth, sizeof(def_eth), "%s", getenv("DEF_ETHER"));
			}

			ifconf_get_interface(def_eth, &ifconf_ifr);

			strncpy(ipstr, ifconf_ipv4_ntoa(ifconf_ifr.ipaddr), 16);

			if (nk_net_adapt_ip_should_adapt(ipstr, xaddr)) {
				NET_find_avai_ip(xaddr, "255.255.255.0", NULL, NULL, 1, 5, 2, get_ip_callback);
			}
			else{
				nk_net_adapt_ip_set_pause_flag(time(NULL), 60);
			}
		}
		return 1;  // probe response
		//return 0; // no probe response
	} else {
		//printf("wsdd hook %s, %s event:%d\n\t\tscope:%s\n", type ? type : "", xaddr, event, scopes ? scopes : "");
		return 0;
	}
#endif //!define(DHCP)
}

#if defined(N1)
NK_Int corsee_GetDeviceUID(NK_PVoid ctx, NK_PChar uid, NK_Size *len)
{
	char tmp[64];

	if((SECURE_CHIP_get_data(SECURE_CHIP_DATA_UID, tmp, sizeof(tmp)) == 0) && NULL != uid && NULL != len){
		*len = strlen(tmp);
		memcpy(uid, tmp, *len + 1);
		return 0;
	}

	APP_TRACE("corsee_GetDeviceUID fail");
	return -1;
}

NK_Int corsee_GetDeviceKey(NK_PVoid ctx, NK_PChar devkey, NK_Size *len)
{
    return 0;
}

NK_Int corsee_trap (NK_PVoid ctx, NK_PChar device_id, NK_Size *len)
{
	char sn[32];
	memset(sn, 0, sizeof(sn));

	if((0 == UC_SNumberGet(sn)) && NULL != device_id && NULL != len){
		memcpy(device_id, sn, strlen(sn)+1);
		*len = strlen(sn);
		printf("!!!!!!!succes:%s-%d: %s %d\n", __FUNCTION__, __LINE__, device_id, *len);
	}else{
		printf("!!!!!!!failed:%s-%d\n", __FUNCTION__, __LINE__);
	}
	return 0;
}

static NK_Int corsee_RecvPacket(NK_PVoid ctx, NK_PChar fromIP, NK_UInt16 fromPort, NK_PChar packet)
{
	unsigned int eth0_ip = 0, eth0_1_ip = 0;
	uint8_t cmd[128];
	struct in_addr addr;
	ifconf_interface_t intrface;
	ST_NSDK_NETWORK_INTERFACE wlan;
	NETSDK_conf_interface_get(4, &wlan);
	if(wlan.wireless.wirelessMode != NSDK_NETWORK_WIRELESS_MODE_ACCESSPOINT){
		return 0;
	}

	eth0_ip = inet_addr(fromIP);
	if(0xA8C0 != (eth0_ip & 0xFFFF)){
		ifconf_get_interface("wlan0", &intrface);
		if((intrface.ipaddr.s_addr & intrface.netmask.s_addr) != (eth0_ip & intrface.netmask.s_addr)){
			eth0_1_ip = (eth0_ip & 0xFFFFFF) | 0xFD000000;
			memcpy(&addr, &eth0_1_ip, sizeof(unsigned int));
			sprintf(cmd, "ifconfig wlan0 %s", inet_ntoa(addr));
			NK_SYSTEM(cmd);
			printf("%s-%d:%s\n", __FUNCTION__, __LINE__, cmd);
			//route
			/*eth0_1_ip = (eth0_ip & 0xFFFFFF) | 0x1000000;
			memcpy(&addr, &eth0_1_ip, sizeof(unsigned int));
			sprintf(cmd, "route add default gw %s dev wlan0", inet_ntoa(addr));
			NK_SYSTEM(cmd);*/
		}
	}
	return 0;
}

static NK_Int corseeOnSetup(NK_PVoid ctx, NK_CorseeSetup *Setup)
{
	ST_NSDK_NETWORK_INTERFACE wlan;
	//static bool playSound = true;
	if(Setup && Setup->Ether && Setup->Ether->WiFi){
		NETSDK_conf_interface_get(4, &wlan);
		if(strcmp(Setup->Ether->WiFi->ssid, wlan.wireless.wirelessStaMode.wirelessApEssId) == 0 && 
			strcmp(Setup->Ether->WiFi->psk, wlan.wireless.wirelessStaMode.wirelessApPsk) == 0 &&
			wlan.wireless.wirelessMode == NSDK_NETWORK_WIRELESS_MODE_STATIONMODE){
			APP_TRACE("same essid (%s) (%s) <-> (%s) (%s)", Setup->Ether->WiFi->ssid, Setup->Ether->WiFi->psk,
				wlan.wireless.wirelessStaMode.wirelessApEssId, wlan.wireless.wirelessStaMode.wirelessApPsk);
			return 0;
		}
		APP_TRACE("setup essid(%s) psk(%s)", Setup->Ether->WiFi->ssid, Setup->Ether->WiFi->psk);
		snprintf(wlan.wireless.wirelessStaMode.wirelessApEssId,
			sizeof(wlan.wireless.wirelessStaMode.wirelessApEssId),"%s",
			Setup->Ether->WiFi->ssid);
		snprintf(wlan.wireless.wirelessStaMode.wirelessApPsk, 
			sizeof(wlan.wireless.wirelessStaMode.wirelessApPsk), "%s",
			Setup->Ether->WiFi->psk);
		wlan.wireless.wirelessMode = NSDK_NETWORK_WIRELESS_MODE_STATIONMODE;
		wlan.wireless.dhcpServer.dhcpAutoSettingEnabled = true;
		wlan.lan.addressingType = kNSDK_NETWORK_LAN_ADDRESSINGTYPE_DYNAMIC;
        SearchFileAndPlay(SOUND_WiFi_setting, NK_False);
        SearchFileAndPlay(SOUND_Please_wait, NK_False);
        NETSDK_conf_interface_set_by_delay(4, &wlan, eNSDK_CONF_SAVE_RESTART_WIRELESS, 2);
#if defined(SOUND_WAVE)
//		SW_destroy();
#endif
		return 0;
	}
	return -1;
}

static void get_near_ap(void *lpAPs, unsigned int *nAPs)
{
        int i = 0, count = 0;
        NK_CorseeWiFiAP * ret_ap = (NK_CorseeWiFiAP *)lpAPs;

        if(lpAPs && nAPs) {
            static stNK_WIFI_HotSpot essid_list[64];
            static time_t tms;
            static int essid_num;
            static unsigned char dBm[64];
            int j, *dBmList = NULL;

            if(time(NULL) - tms >= 8 || essid_num <= 0) {
                essid_num = APP_WIFI_Wifi_Scan("wlan0", essid_list, sizeof(essid_list) / sizeof(essid_list[0]));
                tms = time(NULL);
                if(essid_num > 0) {
                    memset(dBm, 0, sizeof(dBm));
                    if((dBmList = alloca(sizeof(int) * essid_num)) == NULL) {
                        essid_num = 0;
                    }
                    for(i = 0; i < essid_num; i++) {
                        if(essid_list[i].essid[0] == '\0' || strlen(essid_list[i].essid) > sizeof(ret_ap->ssid) - 1 || essid_list[i].essid[0] == ' ' 
                            ){
                            essid_list[i].essid[0] = '\0';
                            dBmList[i] = -100;
                        }
                        else{
                            dBmList[i] = essid_list[i].dBm;
                        }
                        dBm[i] = i;
                    }
                    for(j = 1; j < essid_num; j++) {
                        for(i = 0; i < essid_num - j; i++) {
                            if(dBmList[i] < dBmList[i + 1]) {
                                count = dBmList[i + 1];
                                dBmList[i + 1] = dBmList[i];
                                dBmList[i] = count;

                                count = dBm[i + 1];
                                dBm[i + 1] = dBm[i];
                                dBm[i] = count;
                            }
                        }
                    }
                    for(count = essid_num, i = essid_num; i > 0; i--) {
                        if(dBmList[i - 1] <= -99) {
                            count--;
                        }
                        else {
                            break;
                        }
                    }
                    essid_num = count;
                    for(i = 0; i < essid_num - 1; i++) {
                        for(j = i + 1; j < essid_num; j++) {
                            if(essid_list[dBm[j]].essid[0] == '\0'){
                                continue;
                            }
                            if(strcmp(essid_list[dBm[i]].essid, essid_list[dBm[j]].essid) == 0) {
                                APP_TRACE("Repeat SSID[%d] (%s)", dBm[j], essid_list[dBm[j]].essid);
                                essid_list[dBm[j]].essid[0] = '\0';
                            }
                        }
                    }
                }
            }
            if(essid_num > 0) {
                for(i = 0, count = 0; i < essid_num && i < *nAPs; i++) {
                    if(essid_list[dBm[i]].essid[0] == '\0') {
                        continue;
                    }
                    snprintf(ret_ap->ssid, sizeof(ret_ap->ssid), "%s", essid_list[dBm[i]].essid);
                    snprintf(ret_ap->bssid, sizeof(ret_ap->bssid), "%s", essid_list[dBm[i]].bssid);
                    snprintf(ret_ap->encrytype, sizeof(ret_ap->encrytype), "%s", essid_list[dBm[i]].encrypt);
                    ret_ap->rssi = essid_list[dBm[i]].dBm;
                    //APP_TRACE("SSID:%s(%d) BSSID:%s(%d) ENCRY:%s(%d) RSSI:%d", ret_ap->ssid, strlen(ret_ap->ssid), 
                    //    ret_ap->bssid, strlen(ret_ap->bssid), ret_ap->encrytype, strlen(ret_ap->encrytype), ret_ap->rssi);
                    ret_ap++;
                    count++;
                }
                APP_TRACE("Got %d , return %d", essid_num, count);
                *nAPs = count;
            }
        }

}

static void corsee_get_near_ap(NK_PVoid ctx, NK_CorseeWiFiAP *APs, NK_Size *nAPs)
{
    if(APs && nAPs) {
        get_near_ap(APs, nAPs);
    }
}

static int corsee_discovery_init()
{
	//NK_Thread_GlobalStartup(NK_MemAlloc_OS(), 128);
	static NK_CorseeDiscoveryEventSet EventSet;
	EventSet.onGetDeviceID = corsee_trap;
	EventSet.onRecvPacket = corsee_RecvPacket;
	EventSet.onSetup = corseeOnSetup;
    EventSet.onGetDeviceUID = corsee_GetDeviceUID;
    EventSet.onGetDeviceKey = corsee_GetDeviceKey;
    EventSet.onGetNearbyAPs = corsee_get_near_ap;
	NK_Corsee_DiscoveryListener(NK_True, &EventSet, NULL);	
	//TICKER_add_task((fTICKER_HANDLER)APP_WIFI_search_ap, 120, true);
    return 0;
}
#endif

static void ipcam_network_dev_bind_get_near_ap(lpDEV_NEAR_AP lpAPs, unsigned int *nAPs)
{
    get_near_ap((void *)lpAPs, nAPs);

}

static int esee_client_update_env(ESEE_CLIENT_ENV_t* ret_env)
{
#ifdef UPNP
	if(ret_env){
		// ip mapping info
		ST_NSDK_NETWORK_INTERFACE lan;
		ST_NSDK_NETWORK_PORT port;
		NETSDK_conf_interface_get(1, &lan);
		NETSDK_conf_port_get(1, &port);
		strncpy(ret_env->ip.lan, lan.lan.staticIP, sizeof(ret_env->ip.lan) - 1);
		ret_env->web_port.lan = ret_env->web_port.upnp = port.value;
		if(UPNP_done()){
			printf("upnp_done\r\n");
	    	ret_env->web_port.upnp = UPNP_external_port(ret_env->web_port.upnp, 0); //TCP PORT
		}
		// data
		ret_env->data_port = ret_env->web_port;

		printf("esee:\r\n");
		printf("ip: %s\r\n", ret_env->ip.lan);
		printf("port: %d/%d\r\n", ret_env->web_port.lan, ret_env->web_port.upnp);
		return 0;
	}
#endif
	return -1;
}

static void *refresh_arp_proc(char *gateway)
{
	char cmd_str[128];
	pthread_detach(pthread_self());
	memset(cmd_str , 0, sizeof(cmd_str));
	sprintf(cmd_str, "ping %s -c 2", gateway);
	printf("reflesh arp:%s\r\n", cmd_str);
	NK_SYSTEM(cmd_str);
	pthread_exit(NULL);
}

static int ipcam_network_refresh_arp(char *gateway)
{
	int ret = 0;
	pthread_t tid;
	ret = pthread_create(&tid, NULL, refresh_arp_proc, gateway);
	//assert(0 == ret);

	/*struct sockaddr_in s_addr;
	struct sockaddr_in c_addr;
	int sock;
	int status;
	int addr_len;
	int len;
	char buff[64];
	int yes =1;
	
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (-1 == sock) 
	{
	printf("socket error./n/r");
	return -1;
	}
	
	setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &yes, sizeof(yes));

	//?Œπ?
	gateway.s4 = 255;
	s_addr.sin_family = AF_INET;
	s_addr.sin_port = htons(1125);
	s_addr.sin_addr.s_addr = gateway.s_addr;
	printf("broadcast address:%s\r\n", inet_ntoa(gateway.in_addr));
	addr_len = sizeof(s_addr);
	sprintf(buff, "reflesh arp");
	len = sendto(sock, buff, strlen(buff), 0, 
	(struct sockaddr*) &s_addr, addr_len);
	
	printf("send ret:%d-%d\r\n", len, strlen(buff));
	
	close(sock);*/
	return 0;
}

//for BUBBLE callback
static BUBBLE_STREAM_TYPE bubble_venc_type(int stream)//stream:1/2/3
{
	int type = 0;
	ST_NSDK_VENC_CH venc_ch;
	NETSDK_conf_venc_ch_get(100+stream, &venc_ch);
	switch(venc_ch.codecType){
		default:
		case kNSDK_CODEC_TYPE_H264:
			type = BUBBLE_STREAM_TYPE_H264;
			break;
		case kNSDK_CODEC_TYPE_H265:
			type = BUBBLE_STREAM_TYPE_H265;
			break;
	}
	return type;
}

static int bubble_venc_resolution(int stream, int *ret_width, int *ret_height)
{
	ST_NSDK_VENC_CH venc_ch;
	NETSDK_conf_venc_ch_get(100+stream, &venc_ch);
	if(venc_ch.freeResolution){
		*ret_width = venc_ch.resolutionWidth;
		*ret_height = venc_ch.resolutionHeight;
	}else{
		*ret_width = (venc_ch.resolution >> 16) & 0xffff;
		*ret_height = (venc_ch.resolution >> 0) & 0xffff;
	}
	return 0;
}

static int network_rtsp_init()
{
#if  !defined(HI3516E_V1)
	int stream_cnt = NETSDK_venc_get_channels();
	int i;
	char stream_name[64];

	RTSP_session_init();
	PORT_MANAGE_init(56000,60000);
	SPOOK_add_service("minirtsp", MINIRTSP_probe, MINIRTSP_loop);

	for(i = 0; i < stream_cnt; i++){
		if(0 == MEDIABUF_get_username_by_id(i, stream_name)){
			RTSP_add_stream(stream_name, stream_name);
		}
	}
#endif
}

static int ipcam_network_bt_on_recved_data(char *ssid,
										   char *pass,
										   char *wifi_mode,
										   char *token)
{
	int ret;
	char SSID_Buffer[256];
	char PASS_Buffer[256];
	ST_NSDK_NETWORK_INTERFACE wlan;

	APP_TRACE("ssid: %s", ssid);
	APP_TRACE("pass: %s", pass);
	APP_TRACE("wifi_mode: %s", wifi_mode);
	APP_TRACE("token: %s", token);

	ret = base64_decode(ssid, SSID_Buffer, strlen(ssid));
	if (ret < 0) {
		APP_TRACE("base64_decode failed for: %s !", ssid);
		return -1;
	}
	ret = base64_decode(pass, PASS_Buffer, strlen(pass));
	if (ret < 0) {
		APP_TRACE("base64_decode failed for: %s !", pass);
		return -1;
	}

	APP_TRACE("ssid decode: %s", SSID_Buffer);
	APP_TRACE("pass decode: %s", PASS_Buffer);


	NETSDK_conf_interface_get(4, &wlan);
	snprintf(wlan.wireless.wirelessStaMode.wirelessApEssId,
			 sizeof(wlan.wireless.wirelessStaMode.wirelessApEssId),"%s",
			 SSID_Buffer);
	snprintf(wlan.wireless.wirelessStaMode.wirelessApPsk,
			 sizeof(wlan.wireless.wirelessStaMode.wirelessApPsk), "%s",
			 PASS_Buffer);
	wlan.wireless.wirelessMode = NSDK_NETWORK_WIRELESS_MODE_STATIONMODE;
	wlan.wireless.dhcpServer.dhcpAutoSettingEnabled = true;

    SearchFileAndPlay(SOUND_WiFi_setting, NK_False);
    SearchFileAndPlay(SOUND_Please_wait, NK_False);
	//NETSDK_conf_interface_set(4, &wlan, eNSDK_CONF_SAVE_RESTART);
    NETSDK_conf_interface_set_by_delay(4, &wlan, eNSDK_CONF_SAVE_RESTART_WIRELESS, 2);

	return 0;
}

void IPCAM_network_switch_to_ap()
{
	ST_NSDK_NETWORK_INTERFACE n_interface;
	NETSDK_conf_interface_get(4, &n_interface);
	n_interface.wireless.wirelessMode = NSDK_NETWORK_WIRELESS_MODE_ACCESSPOINT;
	printf("%s-%s\n", __FUNCTION__, n_interface.wireless.wirelessApMode.wirelessEssId);
#if defined(P2P)
	//P2PSDKDeinit();
#endif
	char sn_str[32] = {0};
	if(0 == UC_SNumberGet(sn_str)) {
		if(strlen(sn_str)> 10){
			snprintf(n_interface.wireless.wirelessApMode.wirelessEssId, 
				sizeof(n_interface.wireless.wirelessApMode.wirelessEssId), "IPC%s", 
				sn_str);
		}
	}else{
		snprintf(n_interface.wireless.wirelessApMode.wirelessEssId, 
			sizeof(n_interface.wireless.wirelessApMode.wirelessEssId), "IPC123456", 
			sn_str);
	}
	//IPCAM_timer_check_sta_status_stop(); // «–ªªµΩapƒ£ Ωπÿ±’wifi staºÏ≤‚◊¥Ã¨
	SearchFileAndPlay(SOUND_Configuration_mode, NK_False);
	NETSDK_conf_interface_set(4, &n_interface, eNSDK_CONF_SAVE_RESTART);
}

int IPCAM_network_wireless_init(int wired_is_up, LP_NSDK_NETWORK_INTERFACE w_interface, int smart_link_flag)
{
	if(!w_interface){
		return -1;
	}

	if(APP_WIFI_model_exist()){
#if defined(WIFI)
		char * wireless_eth = NULL;
		char cmd_str[256];
        char static_ip[64] = {0};
		struct sta_struct sta_info;
        ST_PRODUCT_TEST_INFO product_info;
        if(NULL != PRODUCT_TEST_get_info(&product_info)){		//STA
            APP_TRACE("run into wifi test mode!:%s", product_info.staEssid);
            JN_Wifi_STA_Init();
            JN_Wifi_STA_Setup(product_info.staEssid,
                              product_info.staPassword,
                              product_info.staStaticIp,
                              product_info.staNetmask,
                              false);
#ifdef LED_CTRL
            initLedContrl(DEF_LED_ID,true,LED_MIN_MODE);
#endif
            // bluetooth FIXMEÂêéÈù¢Ë¶ÅÊîπ‰∏∫Âè™ÂêØÂä®ËìùÁâô
            DEV_BIND_interrupt();
            DEV_BIND_start(ipcam_network_bt_on_recved_data, ipcam_network_dev_bind_get_near_ap);
        }else if(w_interface->wireless.wirelessMode == NSDK_NETWORK_WIRELESS_MODE_ACCESSPOINT){  //AP
			APP_TRACE("wifi start at ap mode");
			wireless_eth = NSDK_NETWORK_WIRELESS_MODE_AP_ETH;
			if(strcmp(w_interface->wireless.dhcpServer.dhcpIpGateway,w_interface->lan.staticGateway)){
				//staticGateway
				strncpy(w_interface->lan.staticGateway,w_interface->wireless.dhcpServer.dhcpIpGateway,sizeof(w_interface->lan.staticGateway));
				//staticPreferredDns
				strncpy(w_interface->dns.staticPreferredDns,w_interface->wireless.dhcpServer.dhcpIpDns,sizeof(w_interface->dns.staticPreferredDns));
				//staticIP
				strncpy(w_interface->lan.staticIP,w_interface->wireless.dhcpServer.dhcpIpGateway,sizeof(w_interface->lan.staticIP));
			}

			//JN_Wifi_Exit();
			JN_Wifi_AP_Init(w_interface->lan.staticIP, w_interface->lan.staticNetmask, w_interface->lan.staticGateway, w_interface->dns.staticPreferredDns);
			JN_Wifi_AP_Setup(w_interface->wireless.wirelessApMode.wirelessEssId, 
				             w_interface->wireless.wirelessApMode.wirelessPsk, 
				             w_interface->wireless.wirelessApMode.wireLessApMode, 
				             w_interface->wireless.wirelessApMode.wirelessWpaMode, 
				             w_interface->wireless.wirelessApMode.wirelessApMode80211nChannel, 
				             w_interface->wireless.dhcpServer.dhcpIpNumber, 
				             w_interface->wireless.dhcpServer.dhcpIpRange, 
				             w_interface->wireless.dhcpServer.dhcpIpDns, 
				             "255.255.255.0",
							 w_interface->wireless.dhcpServer.dhcpIpGateway);

			// bluetooth
			DEV_BIND_interrupt();
			DEV_BIND_start(ipcam_network_bt_on_recved_data, ipcam_network_dev_bind_get_near_ap);

	#ifdef LED_CTRL
			initLedContrl(DEF_LED_ID,true,LED_MAX_MODE);
	#endif
#ifdef SMART_LINK
            /*if(smart_link_flag){
                SMART_link_init(NSDK_NETWORK_WIRELESS_MODE_STA_ETH);
            }*/
#endif
		}else if(w_interface->wireless.wirelessMode == NSDK_NETWORK_WIRELESS_MODE_STATIONMODE){   //STA
			APP_TRACE("wifi start at sta mode");
			wireless_eth = NSDK_NETWORK_WIRELESS_MODE_STA_ETH;
			//JN_Wifi_Exit();
			memset(&sta_info, 0, sizeof(sta_info));
            memset(static_ip, 0, sizeof(static_ip));
			JN_Wifi_STA_Setparam(&sta_info, w_interface->wireless.wirelessStaMode.wirelessApEssId, w_interface->wireless.wirelessStaMode.wirelessApPsk);
			JN_Wifi_STA_Init();
            if((WPA_getWifiConnectedFlag() == false)
                && (w_interface->wireless.dhcpServer.dhcpAutoSettingEnabled == true)) {
                snprintf(static_ip, sizeof(static_ip), "0.0.0.0");
            }
            else {
                snprintf(static_ip, sizeof(static_ip), w_interface->lan.staticIP);
            }
			JN_Wifi_STA_Setup( w_interface->wireless.wirelessStaMode.wirelessApEssId,
							   w_interface->wireless.wirelessStaMode.wirelessApPsk,
				               static_ip, 
				               w_interface->lan.staticNetmask,
				               w_interface->wireless.dhcpServer.dhcpAutoSettingEnabled);

	#ifdef LED_CTRL
			initLedContrl(DEF_LED_ID,true,LED_MIN_MODE);
	#endif
			wifi_wpa_init();
		}/*else if(w_interface->wireless.wirelessMode == NSDK_NETWORK_WIRELESS_MODE_REPEATER){//STA(WLAN0) + AP(WLAN1)
			APP_TRACE("wifi start at repeater mode");
			char staticIP[64];
			char *ifaceSTA = NSDK_NETWORK_WIRELESS_MODE_REPEATER_STA_ETH;
			char *ifaceAP = NSDK_NETWORK_WIRELESS_MODE_REPEATER_AP_ETH;
			int start_bridge = APP_WIFI_If_Exist(ifaceAP);
			snprintf(staticIP, sizeof(staticIP), "0.0.0.0");
			memset(&sta_info, 0, sizeof(sta_info));
			JN_Wifi_STA_Setparam(&sta_info, w_interface->wireless.wirelessStaMode.wirelessApEssId, w_interface->wireless.wirelessStaMode.wirelessApPsk, 1);
			JN_Wifi_STA_Init(ifaceSTA);
			if(start_bridge){
				JN_Wifi_AP_Init(ifaceAP, staticIP, w_interface->lan.staticNetmask, w_interface->lan.staticGateway, w_interface->dns.staticPreferredDns);
				APP_Wifi_Concurrent_Set_Bridge(ifaceSTA, ifaceAP, w_interface->lan.staticIP, "255.255.255.0");
				wireless_eth = NSDK_NETWORK_WIRELESS_MODE_REPEATER_ETH;
			}
			else{
				APP_TRACE("%s is not exist, wifi start at sta mode", ifaceAP);
				snprintf(staticIP, sizeof(staticIP), w_interface->lan.staticIP);
				wireless_eth = NSDK_NETWORK_WIRELESS_MODE_STA_ETH;
			}
			sleep(1);

			JN_Wifi_STA_Setup(ifaceSTA, &sta_info, 
				              staticIP, w_interface->lan.staticNetmask,
				              w_interface->wireless.dhcpServer.dhcpAutoSettingEnabled, start_bridge ? NSDK_NETWORK_WIRELESS_MODE_REPEATER_ETH : NULL);
			//AP‘⁄STA¡¨Ω”…œ∫ÛΩ¯––÷ÿ∆Ù
			//APP_Wifi_Concurrent_RestartAp
		}*/else if(w_interface->wireless.wirelessMode == NSDK_NETWORK_WIRELESS_MODE_NONE){  //none
			APP_TRACE("wifi do not start");
			//JN_Wifi_Exit();
		}
		//SMART_link_init();

		if(wireless_eth && !wired_is_up){
			ifconf_interface_t ifconf_irf;
            char *eth_name = "eth0";
            char ip[32]={""}, netmask[32] = {""};

			sprintf(cmd_str, "route del default gw 0.0.0.0 dev %s; route del default gw 0.0.0.0 dev %s", eth_name, wireless_eth);
			NK_SYSTEM(cmd_str);

            memset(&ifconf_irf, 0, sizeof(ifconf_irf));
            ifconf_get_interface(eth_name, &ifconf_irf);
            ifconf_irf.ipaddr.s_addr &= ifconf_irf.netmask.s_addr;
            sprintf(ip, "%s", ifconf_ipv4_ntoa(ifconf_irf.ipaddr));
            sprintf(netmask, "%s", ifconf_ipv4_ntoa(ifconf_irf.netmask));
            sprintf(cmd_str, "route del -net %s netmask %s dev %s", ip, netmask, eth_name);
            NK_SYSTEM(cmd_str);
			sprintf(cmd_str, "route add default gw %s dev %s", w_interface->lan.staticGateway, wireless_eth);
			NK_SYSTEM(cmd_str);

			setenv("DEF_ETHER", wireless_eth, true);			//setenv ÂèØ‰ª•Áî®Êù•Ê∑ªÂä†Êàñ‰øÆÊîπÁéØÂ¢ÉÂèòÈáè
            memset(&ifconf_irf, 0, sizeof(ifconf_irf));
			ifconf_get_interface(wireless_eth, &ifconf_irf);
			snprintf(cmd_str, sizeof(cmd_str), "%02x:%02x:%02x:%02x:%02x:%02x", ifconf_irf.hwaddr.s_b[0], ifconf_irf.hwaddr.s_b[1],
			 ifconf_irf.hwaddr.s_b[2], ifconf_irf.hwaddr.s_b[3], ifconf_irf.hwaddr.s_b[4],ifconf_irf.hwaddr.s_b[5]);
			setenv("DEVICE_MAC", cmd_str, true);
		}
#endif//defined(WIFI)
	}

	return 0;
}

int net_adapt_enable(void)
{
	ST_NSDK_NETWORK_INTERFACE ja_interface;
	int adjust_en;
	char def_eth[128];
	if(NULL == getenv("DEF_ETHER")){
		snprintf(def_eth, sizeof(def_eth), "eth0");
	}else{
		snprintf(def_eth, sizeof(def_eth), "%s", getenv("DEF_ETHER"));
	}

	if ( strcmp("eth0", def_eth) == 0 ) {
		NETSDK_conf_interface_get(1, &ja_interface);
		adjust_en = 1;
	} else {
		NETSDK_conf_interface_get(4, &ja_interface);
		adjust_en = (strlen(ja_interface.wireless.repeaterDevId) > 0) ? 1:0;
	}	

	int connect_cnt;
#if defined(TFCARD)
	char buf[32];
	NK_Int sdcard_status = NK_TFCARD_get_status(buf);
	if(sdcard_status != emTFCARD_STATUS_OK && MEDIABUF_get_used(0) > 1){
		connect_cnt = MEDIABUF_get_used(0) - 1;
	}else{
		connect_cnt = MEDIABUF_get_used(0);
	}
#else
	connect_cnt = MEDIABUF_get_used(0);
#endif
	APP_TRACE("def_eth %s, addressingType %d, connect_cnt %d, adjust_en %d", 
				def_eth, ja_interface.lan.addressingType, connect_cnt, adjust_en);

	if((ja_interface.lan.addressingType == kNSDK_NETWORK_LAN_ADDRESSINGTYPE_DYNAMIC) 
		&& (connect_cnt < 1) 
		&& (1 == adjust_en) ){
		return 1;
	}

	return 0;
}

int IPCAM_network_init()
{
	int i = 0, ii = 0, wired_is_up = 0, flag = 0;
	//Sysenv_t* sysenv = SYSENV_dup();
	SYSCONF_t* sysconf = SYSCONF_dup();
	//////////////////////////////////////////////////////////////
	// ifconf
	ifconf_interface_t intrface;
	ifconf_dns_t dns;

	APP_HICHIP_Lock_init();
	memset(&intrface, 0, sizeof(intrface));
	memset(&dns, 0, sizeof(dns));
	// ip mask gateway broadcast
	/*intrface.ipaddr.s_addr = sysconf->ipcam.network.lan.static_ip.s_addr;
	intrface.netmask.s_addr = sysconf->ipcam.network.lan.static_netmask.s_addr;
	intrface.gateway.s_addr = sysconf->ipcam.network.lan.static_gateway.s_addr;
	intrface.broadcast.s_addr = 0;*/
#if defined(WIFI)
	JN_Wifi_Exit();
#endif //defined(WIFI)

	// FIXME:
	ST_NSDK_SYSTEM_DEVICE_INFO dev_info;
	if(NETSDK_conf_system_get_device_info(&dev_info)){
		char *token = NULL;
		char *macAddress = strdupa(dev_info.macAddress);
		APP_TRACE("MAC Address: %s", macAddress);
		for(i = 0; i < sizeof(intrface.hwaddr.s_b) / sizeof(intrface.hwaddr.s_b[0]); ++i){
			char *s = strtok_r((0 == i ? macAddress : NULL), ":", &token);
			int n = 0;
			STR_TO_UPPER(s);
			sscanf(s, "%X", &n);
			intrface.hwaddr.s_b[i] = n;
			//intrface.hwaddr.s_b[i] = sysconf->ipcam.network.mac.s[i];
		}
		setenv("DEVICE_MAC", dev_info.macAddress, true);
	}
	ST_NSDK_NETWORK_INTERFACE lan0, virtual_lan, default_lan, n_interface;
	NETSDK_conf_interface_get(1, &lan0);
	NETSDK_conf_interface_get(2, &virtual_lan);
	NETSDK_conf_interface_get(3, &default_lan);
	NETSDK_conf_interface_get(4, &n_interface);//wlan

	//Virtual IP
	flag = NETSDK_tmp_interface_get(1, &lan0);
	intrface.ipaddr.s_addr = inet_addr(lan0.lan.staticIP);
	intrface.netmask.s_addr = inet_addr(lan0.lan.staticNetmask);

    network_ifconf_set_interface("eth0", &intrface);			//≥ı ºªØeth0,∆Ù∂Øeth0
    sleep(1);													//–Ëµ»¥˝eth0∆Ù∂ØÕÍ±œ
	wired_is_up = (check_nic("eth0") == 0);
	if(APP_WIFI_model_exist() && !wired_is_up){
		//”–wifiƒ£øÈµ´√ª≤ÂÕ¯œﬂ
	}else{
 		intrface.gateway.s_addr = inet_addr(lan0.lan.staticGateway);
	}
	if(flag){
		NETSDK_conf_interface_get(1, &lan0);
	}
#if defined(WIFI)
	NK_WIFI_adapter_monitor_thread_start(APP_WIFI_self_reboot(), APP_WIFI_check_smartlink_status());
#endif //defined(WIFI)

	intrface.broadcast.s_addr = 0;

	ST_NSDK_NETWORK_PORT port[4];
	for(i = 0; i<4; i++){
		NETSDK_conf_port_get(i+1, &port[i]);
	};

	//for DNVR
	char str_set_vlan[256];
	memset(str_set_vlan, 0, sizeof(str_set_vlan));
	sprintf(str_set_vlan, "ifconfig eth0:1 %s netmask %s", 
		virtual_lan.lan.staticIP, virtual_lan.lan.staticNetmask);
	printf("cmd:%s\r\n", str_set_vlan);
	NK_SYSTEM(str_set_vlan);

	intrface.mtu = 1500;
	intrface.is_up = true;
	network_ifconf_set_interface("eth0", &intrface);			//»Áπ˚Œ™”–œﬂ¡¨Ω”£¨…Ë÷√”–œﬂÕ¯πÿ
	setenv("DEF_ETHER", "eth0", true);

#if defined(DHCP)
    bool inter;
    inter = network_check_interface();
    if(inter) {
        if(lan0.lan.addressingType == kNSDK_NETWORK_LAN_ADDRESSINGTYPE_DYNAMIC) {
            NET_openUdhcpc("eth0");
        }
    }
    else {
    }
#endif// !defined DHCP

	char *str_backup_ip = "ifconfig eth0:2 192.168.168.168 netmask 255.255.255.0";
	//memset(str_backup_ip, 0, sizeof(str_backup_ip));
	//sprintf(str_set_vlan, "ifconfig eth0:2 192.168.2.34 netmask 255.255.255.0",)
	printf("cmd:%s\r\n", str_backup_ip);
	NK_SYSTEM(str_backup_ip);
	// dns
	dns.preferred.s_addr = inet_addr(lan0.dns.staticPreferredDns);
	dns.alternate.s_addr = inet_addr(lan0.dns.staticAlternateDns);
	ifconf_set_dns(&dns);	
	//ifconf_set_interface("eth0", &intrface);
	/*char cmd_str[128];
	sprintf(cmd_str, "route add default gw %s", lan0.lan.staticGateway); 
	NK_SYSTEM(cmd_str);
	ipcam_network_refresh_arp(lan0.lan.staticGateway);*/
	//////////////////////////////////////////////////////////////
	
	//timezone sync
	// FIXME:
	ST_NSDK_SYSTEM_TIME sys_time;
	if(NETSDK_conf_system_get_time(&sys_time)){		
		APP_TRACE("GMT: %d", sys_time.greenwichMeanTime);
		GMT_SET(sys_time.greenwichMeanTime);
		
		if(sys_time.ntpEnabled){
			NTP_start(sys_time.ntpServerDomain, NULL, 0, 0); // “ÚŒ™◊Ó–¬µƒ–ﬁ∏ƒ£¨ntpÕ¨≤Ω ±≤ª…Ë÷√ ±«¯£¨À˘“‘≤Œ ˝4≤ª…˙–ß
		}
	}
	ST_NSDK_SYSTEM_DST dst_time = {0};
	if(NETSDK_conf_system_get_DST_info(&dst_time)){
		DST_SET(dst_time.enable ? dst_time.offset : 0, 
			dst_time.week[0].month, dst_time.week[0].week, dst_time.week[0].weekday, dst_time.week[0].hour, dst_time.week[0].minute, 
			dst_time.week[1].month, dst_time.week[1].week, dst_time.week[1].weekday, dst_time.week[1].hour, dst_time.week[1].minute);
	}
#if defined(VOICE_TALK)
	NK_N1Device_TwoWayTalk_init();
#endif	

	// web server
	WEBS_init(getenv("WEBDIR"));
	WEBS_empty_cgi();

	WEBS_add_cgi("/cgi-bin/gw2.cgi", cgi_gw_main2, kH_METH_GET);
	WEBS_add_cgi("/cgi-bin/upload.cgi", CGI_system_upgrade2, kH_METH_POST);
	WEBS_add_cgi("/cgi-bin/upgrade_rate.cgi", CGI_system_upgrade_get_rate2, kH_METH_GET);
	WEBS_add_cgi("/bubble/live", BUBBLE_over_http_cgi2, kH_METH_GET);
	
	WEBS_add_cgi("/snapshot", WEB_CGI_snapshot,kH_METH_ALL);
	WEBS_add_cgi("/snapshot.jpg", WEB_CGI_snapshot,kH_METH_ALL);
	
	WEBS_add_cgi("/user/user_list.xml", WEB_CGI_user_list,kH_METH_ALL);
	WEBS_add_cgi("/user/add_user.xml", WEB_CGI_add_user,kH_METH_ALL);
	WEBS_add_cgi("/user/del_user.xml", WEB_CGI_del_user,kH_METH_ALL);
	WEBS_add_cgi("/user/edit_user.xml", WEB_CGI_edit_user,kH_METH_ALL);
	WEBS_add_cgi("/user/set_pass.xml", WEB_CGI_user_set_password,kH_METH_ALL);
	WEBS_add_cgi("/user/user_reset", WEB_CGI_user_reset,kH_METH_ALL);
	WEBS_add_cgi("/user/get_sn_num", WEB_CGI_user_get_rand, kH_METH_ALL);
	//for dana id QRcode
	WEBS_add_cgi("/tmp/dana_id.bmp", WEB_CGI_get_dana_id_QRCode, kH_METH_ALL);
	//for isp cfg ini
	WEBS_add_cgi("/isp_cfg", WEB_CGI_isp_cfg, kH_METH_ALL);
	//for defect pixel
	WEBS_add_cgi("/cgi-bin/cal_defect_pixel.cgi", WEB_CGI_cal_defect_pixel, kH_METH_GET);

	/// Two Way Talk Temporary, Frank
	WEBS_add_cgi("/cgi-bin/Chat", HTTP_CGI_Chat, kH_METH_GET);

	//add netsdk cgi
	APP_NETSDK_http_init(); 

	// pie cake
	WEBS_add_cgi("/PieCake/login", PIECAKE_login, kH_METH_GET);
	WEBS_add_cgi("/PieCake/live", PIECAKE_live_stream, kH_METH_GET);
	WEBS_add_cgi("/PieCake/live/picture", PIECAKE_live_picture, kH_METH_GET);
	WEBS_add_cgi("/PieCake/ptz", PIECAKE_ptz, kH_METH_GET);
#if defined(SDCARD)
	WEBS_add_cgi("/PieCake/sdCard/media/search", PIECAKE_sdcard_media_search, kH_METH_GET);	
	WEBS_add_cgi("/PieCake/sdCard/media/playback", PIECAKE_sdcard_media_playback, kH_METH_GET);
#endif
	// Tencent WeChat
	WEBS_add_cgi("/WeChat", WECHAT_http_service, kH_METH_GET);

#if  !defined(HI3516E_V1)
	// web server hikvision v1.0 cgi interfaces
	HIKVISIOv10_init();
#endif

	// dump cgi
	//WEBS_dump_cgi();

	// web server user
	WEBS_empty_user();
	//WEBS_add_user("haha", "yoyo");
	//WEBS_dump_user();

	/////////////////////////////////////////////////////////////
	//Wifi
	IPCAM_network_wireless_init(wired_is_up, &n_interface, 1);
#ifdef DIAL
    DIAL_deamon_run();
#endif

	////////////////////////////////////////////////////////////
#if defined(FTPSERVER)
	LP_FTP_SERVER ftp_serv = NULL;
	APP_FTP_init(60021,"/media/");
	APP_add_ftp("USER","2",FTP_cmd_user);
	APP_add_ftp("PASS","2",FTP_cmd_pass);	
	APP_add_ftp("PWD","2",FTP_cmd_pwd);		
	APP_add_ftp("CWD","2",FTP_cmd_cwd);			
	APP_add_ftp("PASV","2",FTP_cmd_pasv);
	APP_add_ftp("TYPE","2",FTP_cmd_type);
	APP_add_ftp("SMNT","2",FTP_cmd_smnt);
	APP_add_ftp("MKD","2",FTP_cmd_mkd);
	APP_add_ftp("RMD","2",FTP_cmd_rmd);
	APP_add_ftp("DELE","2",FTP_cmd_dele);
	APP_add_ftp("CDUP","2",FTP_cmd_cdup);		
	APP_add_ftp("REIN","2",FTP_cmd_rein);
	APP_add_ftp("QUIT","2",FTP_cmd_quit);
	APP_add_ftp("PORT","2",FTP_cmd_port);
	APP_add_ftp("LIST","2",FTP_cmd_list);
    APP_add_ftp("RETR","2",FTP_cmd_retr);	
    APP_add_ftp("STOR" ,"2",FTP_cmd_stor);
    APP_add_ftp("RNFR","2",FTP_cmd_rnfr);	
    APP_add_ftp("RNTO","2",FTP_cmd_rnto);
	APP_add_ftp_user("admin","juantech","0");
#endif
	//////////////////////////////////////////////////////////////
	// rtsp
	//RTSP_session_init();

#if  !defined(HI3516E_V1)
	//////////////////////////////////////////////////////////////
	// set usrm hook
	HTTP_AUTH_set_rtsp_auth_hook(USRM_get_password);
	RTMP_set_auth_hook(USRM_get_password);
#endif

	//////////////////////////////////////////////////////////////
	// spook
	SPOOK_init(port[0].value);
//	SPOOK_addrlist_as_black();
//	SPOOK_addrlist_as_white();
//	SPOOK_addrlist_add("192.168.1.46");
//	SPOOK_addrlist_add("192.168.1.46");

	//SPOOK_add_session("rtspd", RTSPD_probe, RTSPD_loop);

	//////////////////////////////////////////////////////////////
	// 3rd party server
	APP_HICHIP_init();

#if  !defined(HI3516E_V1)
	network_rtsp_init();
#endif

			//onvif init
    nk_net_adapt_ip_set_enable_hook(net_adapt_enable);
    nk_net_adapt_ip_set_pause_flag(time(NULL), 60);
#if defined(ONVIFNVT)
			SPOOK_add_service("onvif", ONVIF_nvt_probe, ONVIF_nvt_loop);
			ONVIF_SERVER_init(ONVIF_DEV_NVT, "IPC");
			ONVIF_search_daemon_start(network_onvif_wsdd_event_hook, NULL);
#endif
	SPOOK_add_service("owsp", OWSP_probe, OWSP_loop);
	SPOOK_add_service("bubble", BUBBLE_probe, BUBBLE_loop);
	SPOOK_add_service("web", WEBS_probe, WEBS_loop);
#if  !defined(HI3516E_V1)
	SPOOK_add_service("rtmp", RTMPD_probe, RTMPD_loop);
#endif
	SPOOK_add_service("regRW", SENSOR_REGRW_probe, SENSOR_REGRW_loop);


	//bubble
	ST_BUBBLE_attr bubble;
	bubble.GET_VENC_TYPE = bubble_venc_type;
	bubble.GET_VENC_RESOLUTION = bubble_venc_resolution;
	BUBBLE_init(&bubble);

#ifdef DDNS
	//ddns
	if(lan0.ddns.enabled== true){
		DDNS_PARA_t _ddns;
		memset(&_ddns,0,sizeof(DDNS_PARA_t));
		_ddns.provider = lan0.ddns.ddnsProvider;
		strcpy(_ddns.changeip.register_url, lan0.ddns.ddnsUrl);
		strcpy(_ddns.changeip.username, lan0.ddns.ddnsUserName);
		strcpy(_ddns.changeip.password, lan0.ddns.ddnsPassword);
		DDNS_start(_ddns);
	}
#endif

	//esee
	char sn_code[64];
	char sn[32];
	int ret = -1;
	memset(sn, 0, sizeof(sn));
	ret = UC_SNumberGet(sn);
	sprintf(sn_code, "%s%s", "JA", sn);
	if((0 == ret) && lan0.esee.enabled){
#if defined(P2P)	
	P2PStart(sn_code, sysconf->ipcam.info.software_version, "JUAN@GZ");
#endif//defined(RUDPA)
	}

	// smtp
	//SMTP_init(true);
//	ipcam_email_start();


	//VSIPLIB_init("eth0");
#if defined(ANTS)
	ANTSLIB_init("eth0", port[0].value != port[1].value ? 
		port[1].value :
		port[1].value+1);
#endif//defined(ANTS)
	
	//GB28181_start();
	//WECHAT_CLIENT_init(NULL, 0);

	//dana protocol init
#if defined(DANALE)
	DanaLib_init();
#endif

#if defined(N1)
	corsee_discovery_init();
#endif
	//remote_upgrade_init();
	return 0;
}

void IPCAM_network_destroy()
{
#if defined(ONVIFNVT)
	ONVIF_search_daemon_stop();
	ONVIF_SERVER_deinit();
#endif
	//GB28181_stop();
	//WECHAT_CLIENT_destroy();
	//ANTSLIB_destroy();
	//VSIPLIB_destroy();
#if defined(RUDPA)
	RUDPA_destroy();
#endif	
//	ipcam_email_stop();
#ifdef SMTP
	SMTP_destroy();
#endif
	//////////////////////////////////////////////////////////////
	//esee
	ESEE_CLIENT_destroy();
#ifdef UPNP
	//upnp
	UPNP_stop();
#endif

#ifdef DDNS
	//ddns
	DDNS_quit();
#endif
	//////////////////////////////////////////////////////////////
	// 3rd party server
	APP_HICHIP_destroy();
	//////////////////////////////////////////////////////////////
	// spook
	SPOOK_destroy();

	//dana
#if defined(DANALE)
	DanaLib_destroy();
#endif
	APP_HICHIP_Lock_destroy();
#ifdef DIAL
    DIAL_deamon_stop();
#endif
	return;
}

void IPCAM_network_restart()
{ 
	ifconf_interface_t intrface;
	ST_NSDK_NETWORK_INTERFACE lan0, vlan, n_interface;
	int wired_is_up = 0, flag = 0;
#if defined(WIFI)
	NK_WiFI_set_adapter_minitor_pause_flag(true);
#endif
	APP_HICHIP_destroy();
#if defined(WIFI)
	APP_WIFI_Wifi_Exit();
	APP_WIFI_model_remove();

//fix me:  if there is no smart_link in PX, it should be deleted
#if defined(WIFI) && defined(SMART_LINK)
	SMART_link_deinit(NSDK_NETWORK_WIRELESS_MODE_STA_ETH);
#endif

#endif //defined(WIFI)
	NETSDK_conf_interface_get(1, &lan0);
	NETSDK_conf_interface_get(2, &vlan);
	NETSDK_conf_interface_get(4, &n_interface);//wifi
	//Virtual IP
	flag = NETSDK_tmp_interface_get(1, &lan0);

	memset(&intrface, 0, sizeof(ifconf_interface_t));
	intrface.ipaddr.s_addr = inet_addr(vlan.lan.staticIP);
	intrface.netmask.s_addr = inet_addr(vlan.lan.staticNetmask);
	ifconf_set_interface("eth0:1", &intrface);

	memset(&intrface, 0, sizeof(ifconf_interface_t));
	intrface.ipaddr.s_addr = inet_addr(lan0.lan.staticIP);
	intrface.netmask.s_addr = inet_addr(lan0.lan.staticNetmask);
	wired_is_up = (check_nic("eth0") == 0);
	if(APP_WIFI_model_exist() && !wired_is_up){
		//”–wifiƒ£øÈµ´√ª≤ÂÕ¯œﬂ
	}else{
		intrface.gateway.s_addr = inet_addr(lan0.lan.staticGateway);
		setenv("DEF_ETHER", "eth0", true);
	}

	if(flag){
		NETSDK_conf_interface_get(1, &lan0);
	}

	network_ifconf_set_interface("eth0", &intrface);

#if defined(DHCP)
		bool inter;
		inter = network_check_interface();
		if(inter) {
			if(lan0.lan.addressingType == kNSDK_NETWORK_LAN_ADDRESSINGTYPE_DYNAMIC) {
				NET_openUdhcpc("eth0");
			}
		}
		else {
		}
#endif// !defined DHCP

	//ipcam_network_refresh_arp(lan0.lan.staticGateway);
	//Wifi	
	IPCAM_network_wireless_init(wired_is_up, &n_interface, 0);

	APP_HICHIP_init();

	//DNS
	ifconf_dns_t dns;
	memset(&dns, 0, sizeof(dns));
	dns.preferred.s_addr = inet_addr(lan0.dns.staticPreferredDns);
	dns.alternate.s_addr = inet_addr(lan0.dns.staticAlternateDns);
	ifconf_set_dns(&dns);	

	//timezone sync
	// FIXME:
	ST_NSDK_SYSTEM_TIME sys_time;
	if(NETSDK_conf_system_get_time(&sys_time)){
		APP_TRACE("GMT: %d", sys_time.greenwichMeanTime);
		GMT_SET(sys_time.greenwichMeanTime);
		
		if(sys_time.ntpEnabled){
			NTP_start(sys_time.ntpServerDomain, NULL, 0, 0); // “ÚŒ™◊Ó–¬µƒ–ﬁ∏ƒ£¨ntpÕ¨≤Ω ±≤ª…Ë÷√ ±«¯£¨À˘“‘≤Œ ˝4≤ª…˙–ß
		}
	}
#if defined(WIFI)
	NK_WiFI_set_adapter_minitor_pause_flag(false);
#endif
}

void IPCAM_
network_wireless_restart()
{
    ST_NSDK_NETWORK_INTERFACE n_interface;
    int wired_is_up = 0;

#if defined(WIFI)
    NK_SYSTEM("ifconfig wlan0 down");
    NK_SYSTEM("kill -9 `pidof wpa_supplicant`");

    /* Âõ†‰∏∫ÊúâÁ∫øÂíåÊó†Á∫øÈÉΩ‰ºöÂêØÂä®udhcpcËøõÁ®ãÔºåÊâÄ‰ª•Ë¶ÅÂà§Êñ≠Âá∫ÊúâÁ∫øÂíåÊó†Á∫øÁöÑudhcpcÔºåÊ≠§Â§ÑÈÄªËæëÂêéÈù¢ÈúÄË¶ÅÊõ¥Êîπ */
    NK_SYSTEM("kill -9 `ps | grep \"udhcpc -q -i wlan0\" | head -1 | awk '{print $1}'`"); // FIXME
    NK_SYSTEM("kill -9 `pidof hostapd`");
    NK_SYSTEM("kill -9 `pidof udhcpd`");
#endif //defined(WIFI)

    NETSDK_conf_interface_get(4, &n_interface);//wifi
    IPCAM_network_wireless_init(wired_is_up, &n_interface, 0);

}

