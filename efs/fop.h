/*
 ============================================================================
 Name        : fop.h
 Author      : gosun.com
 Copyright   : 2017(c) beijing efs Information Technologies Co., Ltd.
 Description : 
 ============================================================================
 */

#ifndef EFS_FOP_H
#define EFS_FOP_H

#include "http.h"

#pragma pack(1)

#ifdef __cplusplus
extern "C"
{
#endif

/*============================================================================*/
/* func Efs_FOP_Pfop */

/* @gist pfopargs */

typedef struct _Efs_FOP_PfopArgs {
	const char * bucket;
	const char * key;
	const char * notifyURL;
	int force;
	const char * pipeline;
} Efs_FOP_PfopArgs;

/* @endgist */

/* @gist pfopret */

typedef struct _Efs_FOP_PfopRet {
	const char*  persistentId;
} Efs_FOP_PfopRet;

/* @endgist */

EFS_DLLAPI extern Efs_Error Efs_FOP_Pfop(Efs_Client* self, Efs_FOP_PfopRet* ret, Efs_FOP_PfopArgs* args, char* fop[], int fopCount);

/*============================================================================*/

#pragma pack()

#ifdef __cplusplus
}
#endif

#endif /* EFS_FOP_H */
