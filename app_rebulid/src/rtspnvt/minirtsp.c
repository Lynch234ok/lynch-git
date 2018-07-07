#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#if defined(IPCAM_SOLUTION)
#include "conf.h"
#endif
#include "vlog.h"
#include "rtspserver.h"

#if defined(_NVR) || defined(_DVR) || defined(_JAMEDIA) || defined(IPCAM_SOLUTION) || defined(PRODUCT_CLASS)
#include "rtspdef.h"
#include "rtsplib.h"
#include "minirtsp.h"
#include "spook.h"

SPOOK_SESSION_PROBE_t MINIRTSP_probe(const void* msg, ssize_t msg_sz)
{
	if((memcmp(msg,"OPTIONS",strlen("OPTIONS")) == 0)
		|| (memcmp(msg,"DESCRIBE",strlen("DESCRIBE")) == 0)
		|| (memcmp(msg,"SET_PARAMETER",strlen("SET_PARAMETER")) == 0)
		|| (memcmp(msg,"GET_PARAMETER",strlen("GET_PARAMETER")) == 0)){
		return SPOOK_PROBE_MATCH;
	}
	return SPOOK_PROBE_MISMATCH;
}

#define ministsp_user_num 32

int *P_trigger[ministsp_user_num];
int minirtsp_cnt = 0;
SPOOK_SESSION_LOOP_t MINIRTSP_loop(bool* trigger, int sock, time_t* read_pts)
{
	ThreadArgs_t args;
	args.data = NULL;
	args.LParam = (void *)trigger;
	args.RParam = sock;
	int i;
	
	if(minirtsp_cnt == 0){
		for(i = 0;i < ministsp_user_num; i++){
			P_trigger[i] = NULL;
		}
	}

	for(i = 0; i < ministsp_user_num ; i++){
		if(P_trigger[i] == NULL){
			P_trigger[i] = (int *)trigger;
			break;
		}
	}
	
	minirtsp_cnt++;
	RTSPS_proc(&args);	
	P_trigger[i] = NULL;
	minirtsp_cnt--;

	return SPOOK_LOOP_SUCCESS;
}


int MINIRTSP_loop_stop()
{
	int i;
	for(i = 0; i < ministsp_user_num; i++){
		if(P_trigger[i] != NULL){
			*P_trigger[i] = false;
			 P_trigger[i] = NULL;
		}
	}
	minirtsp_cnt = 0;
	return 0;
}


#else
#include "rtspclient.h"

int main(int argc,char **argv)
{
	return RTSPC_test(argc,argv);
	const char *usage="./minirtsp [-c | -s ]\r\n";
	if(argc < 2){
		printf(usage);
		return 0;
	}
	if(strcmp(argv[1],"-c")==0){
		VLOG(VLOG_CRIT,"run as player...");
		RTSPC_test(argc,argv);
	}else if(strcmp(argv[1],"-s")==0){
		VLOG(VLOG_CRIT,"run as server...");
		RTSPS_test(argc,argv);
	}else{
		printf(usage);
	}
	return 0;
}
#endif

