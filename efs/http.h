/*
 ============================================================================
 Name        : http.h
 Author      : gosun.com
 Copyright   : 2017(c) beijing efs Information Technologies Co., Ltd.
 Description : 
 ============================================================================
 */

#ifndef EFS_HTTP_H
#define EFS_HTTP_H

#include "base.h"
#include "conf.h"

/*============================================================================*/
/* Global */

#ifdef __cplusplus
extern "C"
{
#endif

extern void Efs_Global_Init(long flags);
extern void Efs_Global_Cleanup();

extern void Efs_MacAuth_Init();
extern void Efs_MacAuth_Cleanup();

extern void Efs_Servend_Init(long flags);
extern void Efs_Servend_Cleanup();

#ifdef __cplusplus
}
#endif

/*============================================================================*/
/* type Efs_Mutex */

#if defined(_WIN32)
#include <windows.h>
typedef CRITICAL_SECTION Efs_Mutex;
#else
#include <pthread.h>
typedef pthread_mutex_t Efs_Mutex;
#endif

#ifdef __cplusplus
extern "C"
{
#endif

EFS_DLLAPI extern void Efs_Mutex_Init(Efs_Mutex* self);
EFS_DLLAPI extern void Efs_Mutex_Cleanup(Efs_Mutex* self);

EFS_DLLAPI extern void Efs_Mutex_Lock(Efs_Mutex* self);
EFS_DLLAPI extern void Efs_Mutex_Unlock(Efs_Mutex* self);

/*============================================================================*/
/* type Efs_Json */

typedef struct cJSON Efs_Json;

EFS_DLLAPI extern const char* Efs_Json_GetString(Efs_Json* self, const char* key, const char* defval);
EFS_DLLAPI extern const char* Efs_Json_GetStringAt(Efs_Json* self, int n, const char* defval);
EFS_DLLAPI extern Efs_Int64 Efs_Json_GetInt64(Efs_Json* self, const char* key, Efs_Int64 defval);
EFS_DLLAPI extern int Efs_Json_GetBoolean(Efs_Json* self, const char* key, int defval);
EFS_DLLAPI extern Efs_Json* Efs_Json_GetObjectItem(Efs_Json* self, const char* key, Efs_Json* defval);
EFS_DLLAPI extern Efs_Json* Efs_Json_GetArrayItem(Efs_Json* self, int n, Efs_Json* defval);
EFS_DLLAPI extern void Efs_Json_Destroy(Efs_Json* self);

/*============================================================================*/
/* type Efs_Auth */

#pragma pack(1)

typedef struct curl_slist Efs_Header;

typedef struct _Efs_Auth_Itbl {
	Efs_Error (*Auth)(void* self, Efs_Header** header, const char* url, const char* addition, size_t addlen);
	void (*Release)(void* self);
} Efs_Auth_Itbl;

typedef struct _Efs_Auth {
	void* self;
	Efs_Auth_Itbl* itbl;
} Efs_Auth;

EFS_DLLAPI extern Efs_Auth Efs_NoAuth;

/*============================================================================*/
/* type Efs_Client */

struct _Efs_Rgn_RegionTable;

typedef struct _Efs_Client {
	void* curl;
	Efs_Auth auth;
	Efs_Json* root;
	Efs_Buffer b;
	Efs_Buffer respHeader;

	// Use the following field to specify which NIC to use for sending packets.
	const char* boundNic;

	// Use the following field to specify the average transfer speed in bytes per second (Bps)
	// that the transfer should be below during lowSpeedTime seconds for this SDK to consider
	// it to be too slow and abort.
	long lowSpeedLimit;

	// Use the following field to specify the time in number seconds that
	// the transfer speed should be below the logSpeedLimit for this SDK to consider it
	// too slow and abort.
	long lowSpeedTime;

	// Use the following field to manange information of multi-region.
	struct _Efs_Rgn_RegionTable * regionTable;
} Efs_Client;

EFS_DLLAPI extern void Efs_Client_InitEx(Efs_Client* self, Efs_Auth auth, size_t bufSize);
EFS_DLLAPI extern void Efs_Client_Cleanup(Efs_Client* self);
EFS_DLLAPI extern void Efs_Client_BindNic(Efs_Client* self, const char* nic);
EFS_DLLAPI extern void Efs_Client_SetLowSpeedLimit(Efs_Client* self, long lowSpeedLimit, long lowSpeedTime);

EFS_DLLAPI extern Efs_Error Efs_Client_Call(Efs_Client* self, Efs_Json** ret, const char* url);
EFS_DLLAPI extern Efs_Error Efs_Client_CallNoRet(Efs_Client* self, const char* url);
EFS_DLLAPI extern Efs_Error Efs_Client_CallWithBinary(
	Efs_Client* self, Efs_Json** ret, const char* url,
	Efs_Reader body, Efs_Int64 bodyLen, const char* mimeType);
EFS_DLLAPI extern Efs_Error Efs_Client_CallWithBuffer(
	Efs_Client* self, Efs_Json** ret, const char* url,
	const char* body, size_t bodyLen, const char* mimeType);

EFS_DLLAPI extern Efs_Error Efs_Client_CallWithBuffer2(
	Efs_Client* self, Efs_Json** ret, const char* url,
	const char* body, size_t bodyLen, const char* mimeType);

/*============================================================================*/
/* func Efs_Client_InitNoAuth/InitMacAuth  */

typedef struct _Efs_Mac {
	const char* accessKey;
	const char* secretKey;
} Efs_Mac;

Efs_Auth Efs_MacAuth(Efs_Mac* mac);

EFS_DLLAPI extern char* Efs_Mac_Sign(Efs_Mac* self, char* data);
EFS_DLLAPI extern char* Efs_Mac_SignToken(Efs_Mac* self, char* data);

EFS_DLLAPI extern void Efs_Client_InitNoAuth(Efs_Client* self, size_t bufSize);
EFS_DLLAPI extern void Efs_Client_InitMacAuth(Efs_Client* self, size_t bufSize, Efs_Mac* mac);

/*============================================================================*/

#pragma pack()

#ifdef __cplusplus
}
#endif

#endif /* EFS_HTTP_H */

