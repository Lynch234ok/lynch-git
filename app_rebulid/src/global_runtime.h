
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#ifndef GLOBAL_RUNTIME_H_
#define GLOBAL_RUNTIME_H_
#ifdef __cplusplus
extern "C" {
#endif

#define REMOTE_UPGRADE_RESTART_FLAG_PATH "/tmp/ota_flag"
#define TFCARD_OLD_RECORD_PATH	"/media/tf/record"

/* 在debug时，用以标记开启/关闭功能 */
#define G_DEBUG_FLAG_FILE      "/media/tf/debug.ini"

#define G_NK_MAX_TWOWAYTALK_NUM	(1)
#define G_NK_MAX_PLAYBACK_NUM	(2)

extern char g_esee_id[32];
extern bool g_authorized;
extern uint32_t g_hardware_version;
extern int g_encryp_chip_type;
extern uint32_t g_encryp_chip_odm1;
extern uint32_t g_encryp_chip_odm2;

extern int GLOBAL_event_lock_init();
extern int GLOBAL_event_lock_deinit();
extern int GLOBAL_event_lock();
extern int GLOBAL_event_unlock();
extern int GLOBAL_enter_twowaytalk();
extern int GLOBAL_leave_twowaytalk();
extern int GLOBAL_enter_playback();
extern int GLOBAL_leave_playback();
extern void GLOBAL_remove_conf_file();
extern uint32_t GLOBAL_get_sys_mem_mb();
extern uint32_t GLOBAL_reboot_system();
extern void GLOBAL_exit();

extern int GLOBAL_enter_live();
extern int GLOBAL_leave_live();

/*
功能:从一个文件中读取struct timeval格式时间,然后设置系统时间
*/
extern int32_t GLOBAL_setTimeFromFile(void);

/*
功能:保存struct timeval格式的时间到文件中
*/
extern int32_t GLOBAL_saveFileFromTime(void);

/*
	标记是否存在旧格式录像文件
	主要用于程序启动时标记，后面直接查询GLOBAL_isOldTypeRecord接口判断是否有旧格式录像
*/
extern void GLOBAL_setOldTypeRecordFlag();

/*
	存在旧格式录像返回true，false没有旧格式录像
*/
extern bool GLOBAL_isOldTypeRecord();

extern bool GLOBAL_sn_front();
#ifdef __cplusplus
};
#endif
#endif //GLOBAL_RUNTIME_H_

