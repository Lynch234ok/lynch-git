#ifndef __P2PDEVICE_H__
#define __P2PDEVICE_H__
//#include "mediabufDelegate.h"
#include "mutex_f.h"

#ifdef WIN32
#pragma comment(lib, "P2PSDKDevice.lib")
#endif
#define SIZE_BUFF 200*1024

extern int au_flag;
extern LP_MUTEX au_mutex;

struct P2PDeviceDemo {
	char serialNo[32];
	char eseeId[32];
    char version[32];
    char vendor[32];
    int max_ch;
	void (*OnDevOnline)(const char *eseeid, void *ctx);
	//int (*OnDevAttachStream)(int chn, int streamNo, void *ctx);
	//int (*OnDevDetachStream)(int chn, int streamNo, void *ctx);
	void *ctx;
};

int P2PDeviceStart(struct P2PDeviceDemo* pDemo);

int P2P_sdkdestroy();
#endif
