#include <pthread.h>
#include <stdint.h>
#include <sys/prctl.h>
#include <app_debug.h>
#include "msg_push.h"
#include "ticker.h"
#include "securedat.h"
#include <sys/prctl.h>
#include "sound.h"
#include "netsdk.h"

#if defined(MSG)

#define INTERVAL_SECONDS    30

typedef struct EseeMsgPush{
	bool usr_enable;
	bool app_enable;
	int interval_seconds;
	pthread_t pid;
    bool mdAlarm;
}stEseeMsgPush;

static stEseeMsgPush MsgPushAttr = {
 .usr_enable = true,//should be set in netsdk
 .app_enable = true,
 .interval_seconds = INTERVAL_SECONDS,
 .pid = -1,
 .mdAlarm = true,
};  

void ESEE_msg_push_enable() {
    if(MsgPushAttr.interval_seconds == 0) {
        MsgPushAttr.app_enable = true;
        MsgPushAttr.mdAlarm = true;
    }
    else if(MsgPushAttr.interval_seconds > 0) {
        MsgPushAttr.interval_seconds--;
    }

}

static void esee_msg_push_proc()
{	
	static char esee_id[16] = "";
	char sn_str[32] = {0};
	int ret = 0;
    int64_t alarm_ts_s;
    ST_NSDK_SYSTEM_TIME systime;
    int timezone_second;


    pthread_detach(pthread_self());
	prctl(PR_SET_NAME, "esee_msg_push_proc");

    NETSDK_conf_system_get_time(&systime);
    timezone_second = ((abs(systime.greenwichMeanTime) / 100) * 3600
                       + (abs(systime.greenwichMeanTime) % 100) * 60)
                      * (systime.greenwichMeanTime > 0 ? 1 : -1);

    if(0 == strlen(esee_id)){
		if(0 == UC_SNumberGet(sn_str)) {
			if(strlen(sn_str)> 10){
				if(sn_str[strlen(sn_str) - 10] == '0'){
					memcpy(esee_id, &sn_str[strlen(sn_str) - 10+1], 9);
				}else{
					memcpy(esee_id, &sn_str[strlen(sn_str) - 10], 10);
				}
			}
		}
	}
	//printf("pid%d--esee ID: %s\n", MsgPushAttr.pid, esee_id);

    alarm_ts_s = (uint64_t)(time(NULL) + timezone_second);
    Esee_msg_send(esee_id, "motion detection", "md", alarm_ts_s);
    if(ret < 0) {  // 假如报警推送发送失败，重新等待下一次移动侦测报警推送
        MsgPushAttr.app_enable = true;
        MsgPushAttr.interval_seconds = 0;
    }
	MsgPushAttr.pid = -1;
	//printf("%s end\n", __FUNCTION__);
}

int ESEE_msg_push()
{
    ST_NSDK_SYSTEM_SETTING sysInfo;

    MsgPushAttr.interval_seconds = INTERVAL_SECONDS;
	if(MsgPushAttr.usr_enable && MsgPushAttr.app_enable && MsgPushAttr.pid >= 0){
		MsgPushAttr.app_enable = false;
		pthread_create(&MsgPushAttr.pid, NULL, (void *)esee_msg_push_proc, NULL);

        if(MsgPushAttr.mdAlarm) {
            MsgPushAttr.mdAlarm = false;
            NETSDK_conf_system_get_setting_info(&sysInfo);
            if(sysInfo.mdAlarm.MotionWarningTone) {
                SearchFileAndPlay(SOUND_Alarm, NK_True);
            }
        }
	}
	return 0;
}
#endif //defined MSG
