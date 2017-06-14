#include <stdio.h>
#include <stdlib.h>
#include "../efs/rs.h"
#include "../efs/io.h"
#include "../efs/http.h"

static int offset = 0;
static int sizeLimit = 0;

int abortCallback(void * userData, char * buf, size_t size)
{
	int * offset = (int *) userData;
	if ((*offset += size) >= sizeLimit) return 1;
	return 0;
}

int main(int argc, char * argv[])
{
	Efs_Error err;
	Efs_Client cli;
	Efs_Mac mac;
	Efs_Io_PutExtra extra;
	Efs_Io_PutRet ret;
	Efs_RS_PutPolicy pp;
	const char * uptoken = NULL;
	const char * bucket = NULL;
	const char * key = NULL;
	const char * localFile = NULL;

	Efs_Global_Init(0);
	Efs_Servend_Init(0);
	Efs_MacAuth_Init();

	Efs_Rgn_Disable();

	memset(&extra, 0, sizeof(extra));
	memset(&pp, 0, sizeof(pp));

	if (argc < 6) {
		printf("Usage: put_file_abortable <ACCESS_KEY> <SECRET_KEY> <BUCKET> <KEY> <LOCAL_FILE> [FILE_SIZE [MAX_FILE_SIZE]]\n");
		return 1;
	} // if

	mac.accessKey = argv[1];
	mac.secretKey = argv[2];
	bucket = argv[3];
	key = argv[4];
	localFile = argv[5];

	if (argc >= 7) {
		extra.upFileSize = atoi(argv[6]);
	} // if

	if (argc >= 8) {
		sizeLimit = atoi(argv[7]);
		extra.upAbortCallback = &abortCallback;
		extra.upAbortUserData = &offset;
	} // if

	pp.scope = Efs_String_Format(512, "%s:%s", bucket, key);
	uptoken = Efs_RS_PutPolicy_Token(&pp, &mac);

	Efs_Client_InitNoAuth(&cli, 8192);

	err = Efs_Io_PutFile(&cli, &ret, uptoken, key, localFile, &extra);
	free((void*)uptoken);
	free((void*)pp.scope);
	printf("code=%d msg=[%s]\n", err.code, err.message);

	Efs_Client_Cleanup(&cli);
	Efs_MacAuth_Cleanup();
	Efs_Servend_Cleanup();
	Efs_Global_Cleanup();

	if (err.code != 200) {
		return 1;
	} // if

	return 0;
} // main
