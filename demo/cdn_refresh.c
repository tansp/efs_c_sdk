#include <stdio.h>
#include <stdlib.h>
#include "../efs/http.h"
#include "../efs/cdn.h"

int main(int argc, char * argv[])
{
	Efs_Error err;
	Efs_Client cli;
	Efs_Mac mac;
	Efs_Cdn_RefreshRet ret;

	Efs_Global_Init(0);
	Efs_Servend_Init(0);
	Efs_MacAuth_Init();

	mac.accessKey = argv[1];
	mac.secretKey = argv[2];

	const char* urls[2] = { "http://a.com/1.html","http://b.com/2.html" };

	Efs_Client_InitMacAuth(&cli, 1024, &mac);

	err = Efs_Cdn_RefreshUrls(&cli, &ret, urls, 1);

	printf("code:%d\n msg:%s\n", err.code, err.message);

	//if (err.code == 200) {
		printf("---------------------------------------------------\n");
		printf("         code : %d\n", ret.code);
		printf("    msg/error : %s\n", ret.error);
		printf("    requestId : %s\n", ret.requestId);
		printf("  invalidUrls : %s\n", ret.invalidUrls);
		printf("  invalidDirs : %s\n", ret.invalidDirs);
		printf("  urlQuotaDay : %d\n", ret.urlQuotaDay);
		printf("urlSurplusDay : %d\n", ret.urlSurplusDay);
		printf("  dirQuotaDay : %d\n", ret.dirQuotaDay);
		printf("dirSurplusDay : %d\n", ret.dirSurplusDay);
		printf("---------------------------------------------------\n");
	//}

	Efs_Free_CdnRefreshRet(&ret);

	Efs_Client_Cleanup(&cli);
	//Efs_MacAuth_Cleanup();
	Efs_Servend_Cleanup();
	Efs_Global_Cleanup();

	return 0;
} // main
