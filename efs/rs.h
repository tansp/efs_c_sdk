/*
 ============================================================================
 Name        : rs.h
 Author      : gosun.com
 Copyright   : 2017(c) beijing efs Information Technologies Co., Ltd.
 Description : 
 ============================================================================
 */

#ifndef EFS_RS_H
#define EFS_RS_H

#include "http.h"

#pragma pack(1)

#ifdef __cplusplus
extern "C"
{
#endif

/*============================================================================*/
/* type PutPolicy, GetPolicy */

/* @gist put-policy */

typedef struct _Efs_RS_PutPolicy {
    const char* scope;
    const char* callbackUrl;
    const char* callbackBody;
    const char* returnUrl;
    const char* returnBody;
    const char* endUser;
    const char* asyncOps;
    const char* persistentOps;
    const char* persistentNotifyUrl;
    const char* persistentPipeline;
    const char* mimeLimit;
    Efs_Uint64 fsizeLimit;
    Efs_Uint32 detectMime;
    Efs_Uint32 insertOnly;
    Efs_Uint32 expires;
    Efs_Uint32 deleteAfterDays;
} Efs_RS_PutPolicy;

/* @endgist */

typedef struct _Efs_RS_GetPolicy {
    Efs_Uint32 expires;
} Efs_RS_GetPolicy;

EFS_DLLAPI extern char* Efs_RS_PutPolicy_Token(Efs_RS_PutPolicy* policy, Efs_Mac* mac);
EFS_DLLAPI extern char* Efs_RS_GetPolicy_MakeRequest(Efs_RS_GetPolicy* policy, const char* baseUrl, Efs_Mac* mac);
EFS_DLLAPI extern char* Efs_RS_MakeBaseUrl(const char* domain, const char* key);

/*============================================================================*/
/* func Efs_RS_Stat */

/* @gist statret */

typedef struct _Efs_RS_StatRet {
	const char* hash;
	const char* mimeType;
	Efs_Int64 fsize;	
	Efs_Int64 putTime;
} Efs_RS_StatRet;

/* @endgist */

EFS_DLLAPI extern Efs_Error Efs_RS_Stat(
	Efs_Client* self, Efs_RS_StatRet* ret, const char* bucket, const char* key);

/*============================================================================*/
/* func Efs_RS_Delete */

EFS_DLLAPI extern Efs_Error Efs_RS_Delete(Efs_Client* self, const char* bucket, const char* key);

/*============================================================================*/
/* func Efs_RS_Copy */

EFS_DLLAPI extern Efs_Error Efs_RS_Copy(Efs_Client* self, 
        const char* tableNameSrc, const char* keySrc, 
        const char* tableNameDest, const char* keyDest);

/*============================================================================*/
/* func Efs_RS_Move */

EFS_DLLAPI extern Efs_Error Efs_RS_Move(Efs_Client* self, 
        const char* tableNameSrc, const char* keySrc, 
        const char* tableNameDest, const char* keyDest);

/*============================================================================*/
/* func Efs_RS_BatchStat */

/* @gist entrypath */

typedef struct _Efs_RS_EntryPath {
    const char* bucket;
    const char* key;
} Efs_RS_EntryPath;

/* @endgist */

/* @gist batchstatret */

typedef struct _Efs_RS_BatchStatRet {
    Efs_RS_StatRet data;
    const char* error;
    int code;
}Efs_RS_BatchStatRet;

/* @endgist */

typedef int Efs_ItemCount;

EFS_DLLAPI extern Efs_Error Efs_RS_BatchStat(
        Efs_Client* self, Efs_RS_BatchStatRet* rets,
        Efs_RS_EntryPath* entries, Efs_ItemCount entryCount);

/*============================================================================*/
/* func Efs_RS_BatchDelete */

/* @gist batchitemret */

typedef struct _Efs_RS_BatchItemRet {
    const char* error;
    int code;
}Efs_RS_BatchItemRet;

/* @endgist */

EFS_DLLAPI extern Efs_Error Efs_RS_BatchDelete(
        Efs_Client* self, Efs_RS_BatchItemRet* rets,
        Efs_RS_EntryPath* entries, Efs_ItemCount entryCount);

/*============================================================================*/
/* func Efs_RS_BatchMove/Copy */

/* @gist entrypathpair */

typedef struct _Efs_RS_EntryPathPair {
    Efs_RS_EntryPath src;
    Efs_RS_EntryPath dest;
} Efs_RS_EntryPathPair;

/* @endgist */

EFS_DLLAPI extern Efs_Error Efs_RS_BatchMove(
        Efs_Client* self, Efs_RS_BatchItemRet* rets,
        Efs_RS_EntryPathPair* entryPairs, Efs_ItemCount entryCount);

EFS_DLLAPI extern Efs_Error Efs_RS_BatchCopy(
        Efs_Client* self, Efs_RS_BatchItemRet* rets,
        Efs_RS_EntryPathPair* entryPairs, Efs_ItemCount entryCount);

/*============================================================================*/

#pragma pack()

#ifdef __cplusplus
}
#endif

#endif /* EFS_RS_H */
