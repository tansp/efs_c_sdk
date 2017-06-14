/*
 ============================================================================
 Name        : io.h
 Author      : gosun.com
 Copyright   : 2017(c) beijing efs Information Technologies Co., Ltd.
 Description : 
 ============================================================================
 */

#ifndef EFS_IO_H
#define EFS_IO_H

#include "http.h"
#include "region.h"
#include "reader.h"

#pragma pack(1)

#ifdef __cplusplus
extern "C"
{
#endif

/*============================================================================*/
/* type Efs_Io_PutExtra */

typedef struct _Efs_Io_PutExtraParam {
	const char* key;
	const char* value;
	struct _Efs_Io_PutExtraParam* next;
} Efs_Io_PutExtraParam;

typedef struct _Efs_Io_PutExtra {
	Efs_Io_PutExtraParam* params;
	const char* mimeType;
	Efs_Uint32 crc32;
	Efs_Uint32 checkCrc32;

	// For those file systems that save file name as Unicode strings,
	// use this field to name the local file name in UTF-8 format for CURL.
	const char* localFileName;

	// For those who want to invoke a upload callback on the business server
	// which returns a JSON object.
	void* callbackRet;
	Efs_Error (*callbackRetParser)(void*, Efs_Json*);

	// For those who want to send request to specific host.
	const char* upHost;
	Efs_Uint32 upHostFlags;
	const char* upBucket;
	const char* accessKey;
	const char* uptoken;

	// For those who want to abort uploading data to server.
	void * upAbortUserData;
	Efs_Rd_FnAbort upAbortCallback;

	// Use the following field to specify the size of an uploading file definitively.
	size_t upFileSize;
} Efs_Io_PutExtra;

/*============================================================================*/
/* type Efs_Io_PutRet */

typedef struct _Efs_Io_PutRet {
	const char* hash;
	const char* key;
    const char* persistentId;
} Efs_Io_PutRet;

typedef size_t (*rdFunc)(char* buffer, size_t size, size_t n, void* rptr);

/*============================================================================*/
/* func Efs_Io_PutXXX */

#ifndef EFS_UNDEFINED_KEY
#define EFS_UNDEFINED_KEY		NULL
#endif

EFS_DLLAPI extern Efs_Error Efs_Io_PutFile(
	Efs_Client* self, Efs_Io_PutRet* ret,
	const char* uptoken, const char* key, const char* localFile, Efs_Io_PutExtra* extra);

EFS_DLLAPI extern Efs_Error Efs_Io_PutBuffer(
	Efs_Client* self, Efs_Io_PutRet* ret,
	const char* uptoken, const char* key, const char* buf, size_t fsize, Efs_Io_PutExtra* extra);

EFS_DLLAPI extern Efs_Error Efs_Io_PutStream(
    Efs_Client* self, 
	Efs_Io_PutRet* ret,
    const char* uptoken, const char* key, 
	void* ctx, // 'ctx' is the same as rdr's last param
	size_t fsize, 
	rdFunc rdr, 
	Efs_Io_PutExtra* extra);

/*============================================================================*/

#pragma pack()

#ifdef __cplusplus
}
#endif

#endif // EFS_IO_H

