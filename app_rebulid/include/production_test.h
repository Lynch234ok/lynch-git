#ifndef _PRODUCTION_TEST_H
#define _PRODUCTION_TEST_H
#ifdef __cplusplus
extern "C" {
#endif

typedef struct product_test_info{
	char staEssid[32];
	char staPassword[32];
	char staStaticIp[32];
	char staNetmask[32];
	char staGateway[32];
}ST_PRODUCT_TEST_INFO, *LP_PRODUCT_TEST_INFO;

extern LP_PRODUCT_TEST_INFO PRODUCT_TEST_get_info(LP_PRODUCT_TEST_INFO info);
extern int PRODUCT_TEST_init();
extern int PRODUCT_TEST_destroy();

#ifdef __cplusplus
};
#endif
#endif /*_PRODUCTION_TEST_H*/

