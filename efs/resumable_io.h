/*
 ============================================================================
 Name        : resumable_io.h
 Author      : gosun.com
 Copyright   : 2017(c) beijing efs Information Technologies Co., Ltd.
 Description :
 ============================================================================
 */

#ifndef EFS_RESUMABLE_IO_H
#define EFS_RESUMABLE_IO_H

#include "http.h"
#include "io.h"

#pragma pack(1)

#ifdef __cplusplus
extern "C"
{
#endif

/*============================================================================*/

#define Efs_Rio_InvalidCtx			701
#define Efs_Rio_UnmatchedChecksum		9900
#define Efs_Rio_InvalidPutProgress	9901
#define Efs_Rio_PutFailed				9902
#define Efs_Rio_PutInterrupted		9903

/*============================================================================*/
/* type Efs_Rio_WaitGroup */

typedef struct _Efs_Rio_WaitGroup_Itbl {
	void (*Add)(void* self, int n);
	void (*Done)(void* self);
	void (*Wait)(void* self);
	void (*Release)(void* self);
} Efs_Rio_WaitGroup_Itbl;

typedef struct _Efs_Rio_WaitGroup {
	void* self;
	Efs_Rio_WaitGroup_Itbl* itbl;
} Efs_Rio_WaitGroup;

#if defined(_WIN32)

Efs_Rio_WaitGroup Efs_Rio_MTWG_Create(void);

#endif

/*============================================================================*/
/* type Efs_Rio_ThreadModel */

typedef struct _Efs_Rio_ThreadModel_Itbl {
	Efs_Rio_WaitGroup (*WaitGroup)(void* self);
	Efs_Client* (*ClientTls)(void* self, Efs_Client* mc);
	int (*RunTask)(void* self, void (*task)(void* params), void* params);
} Efs_Rio_ThreadModel_Itbl;

typedef struct _Efs_Rio_ThreadModel {
	void* self;
	Efs_Rio_ThreadModel_Itbl* itbl;
} Efs_Rio_ThreadModel;

EFS_DLLAPI extern Efs_Rio_ThreadModel Efs_Rio_ST;

/*============================================================================*/
/* type Efs_Rio_Settings */

typedef struct _Efs_Rio_Settings {
	int taskQsize;
	int workers;
	int chunkSize;
	int tryTimes;
	Efs_Rio_ThreadModel threadModel;
} Efs_Rio_Settings;

EFS_DLLAPI extern void Efs_Rio_SetSettings(Efs_Rio_Settings* v);

/*============================================================================*/
/* type Efs_Rio_PutExtra */

typedef struct _Efs_Rio_BlkputRet {
	const char* ctx;
	const char* checksum;
	Efs_Uint32 crc32;
	Efs_Uint32 offset;
	const char* host;
} Efs_Rio_BlkputRet;

#define EFS_RIO_NOTIFY_OK 0
#define EFS_RIO_NOTIFY_EXIT 1

typedef int (*Efs_Rio_FnNotify)(void* recvr, int blkIdx, int blkSize, Efs_Rio_BlkputRet* ret);
typedef int (*Efs_Rio_FnNotifyErr)(void* recvr, int blkIdx, int blkSize, Efs_Error err);

typedef struct _Efs_Rio_PutExtra {
	const char* callbackParams;
	const char* bucket;
	const char* customMeta;
	const char* mimeType;
	int chunkSize;
	int tryTimes;
	void* notifyRecvr;
	Efs_Rio_FnNotify notify;
	Efs_Rio_FnNotifyErr notifyErr;
	Efs_Rio_BlkputRet* progresses;
	size_t blockCnt;
	Efs_Rio_ThreadModel threadModel;

	// For those file systems that save file name as Unicode strings,
	// use this field to name the local file name in UTF-8 format for CURL.
	const char* localFileName;

	// For those who want to invoke a upload callback on the business server
	// which returns a JSON object.
	void* callbackRet;
	Efs_Error (*callbackRetParser)(void*, Efs_Json*);

	// Use xVarsList to pass user defined variables and xVarsCount to pass the count of them.
	//
	// (extra->xVarsList[i])[0] set as the variable name, e.g. "x:Price".
	// **NOTICE**: User defined variable's name MUST starts with a prefix string "x:".
    //
	// (extra->xVarsList[i])[1] set as the value, e.g. "priceless".
	const char* (*xVarsList)[2];
	int xVarsCount;

	// For those who want to send request to specific host.
	const char* upHost;
	Efs_Uint32 upHostFlags;
	const char* upBucket;
	const char* accessKey;
	const char* uptoken;
} Efs_Rio_PutExtra;

/*============================================================================*/
/* type Efs_Rio_PutRet */

typedef Efs_Io_PutRet Efs_Rio_PutRet;

/*============================================================================*/
/* func Efs_Rio_BlockCount */

EFS_DLLAPI extern int Efs_Rio_BlockCount(Efs_Int64 fsize);

/*============================================================================*/
/* func Efs_Rio_PutXXX */

#ifndef EFS_UNDEFINED_KEY
#define EFS_UNDEFINED_KEY		NULL
#endif

EFS_DLLAPI extern Efs_Error Efs_Rio_Put(
	Efs_Client* self, Efs_Rio_PutRet* ret,
	const char* uptoken, const char* key, Efs_ReaderAt f, Efs_Int64 fsize, Efs_Rio_PutExtra* extra);

EFS_DLLAPI extern Efs_Error Efs_Rio_PutFile(
	Efs_Client* self, Efs_Rio_PutRet* ret,
	const char* uptoken, const char* key, const char* localFile, Efs_Rio_PutExtra* extra);

/*============================================================================*/

#pragma pack()

#ifdef __cplusplus
}
#endif

#endif // EFS_RESUMABLE_IO_H
