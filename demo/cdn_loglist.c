#include <stdio.h>
#include <stdlib.h>
#include "../efs/http.h"
#include "../efs/cdn.h"

int main(int argc, char * argv[])
{
	Efs_Error err;
	Efs_Client cli;
	Efs_Mac mac;
	Efs_Cdn_LogListRet ret;

	Efs_Global_Init(0);
	Efs_Servend_Init(0);
	Efs_MacAuth_Init();

	mac.accessKey = argv[1];
	mac.secretKey = argv[2];

	char* day = "2017-03-28";
	char* domains[2] = { "a.com","b.com" };

	Efs_Client_InitMacAuth(&cli, 1024, &mac);

	err = Efs_Cdn_GetLogList(&cli, &ret, day, domains, 2);

	printf("code:%d\n msg:%s\n", err.code, err.message);

	printf("---------------------------------------------------\n");
	printf(" code : %d\n", ret.code);
	printf("error : %s\n", ret.error);
	printf("  num : %d\n", ret.num);
	for (int i = 0; i < ret.num; ++i) {
		if (ret.data_a[i].hasValue) {
			printf("\n");
			printf("  index : %d/%d\n", i + 1, ret.num);
			printf(" domain : %s\n\n", ret.data_a[i].domain);
			for (int j = 0; j < ret.data_a[i].count; ++j) {
				printf("   part : %d/%d\n", j + 1, ret.data_a[i].count);
				printf("   name : %s\n", ret.data_a[i].item_a[j].name);
				printf("   size : %d\n", ret.data_a[i].item_a[j].size);
				printf("  mtime : %d\n", ret.data_a[i].item_a[j].mtime);
				printf("    url : %s\n", ret.data_a[i].item_a[j].url);
				printf("\n");
			}
			printf("\n");
		}
	}
	printf("---------------------------------------------------\n");

	Efs_Free_CdnLogListRet(&ret);

	Efs_Client_Cleanup(&cli);
	//Efs_MacAuth_Cleanup();
	Efs_Servend_Cleanup();
	Efs_Global_Cleanup();

	return 0;
} // main
