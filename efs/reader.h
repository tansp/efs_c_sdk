/*
 ============================================================================
 Name		: reader.h
 Author	  : gosun.com
 Copyright   : 2012-2016(c) Shanghai Efs Information Technologies Co., Ltd.
 Description : 
 ============================================================================
 */

#ifndef EFS_READER_H
#define EFS_READER_H

#include "base.h"

#pragma pack(1)

#ifdef __cplusplus
extern "C"
{
#endif

/*============================================================================*/
/* Declaration of Controllable Reader */

typedef int (*Efs_Rd_FnAbort)(void * abortUserData, char * buf, size_t size);

enum
{
	EFS_RD_OK = 0,
	EFS_RD_ABORT_BY_CALLBACK,
	EFS_RD_ABORT_BY_READAT
};

typedef struct _Efs_Rd_Reader
{
	Efs_File * file;
	Efs_Off_T offset;

	int status;

	void * abortUserData;
	Efs_Rd_FnAbort abortCallback;
} Efs_Rd_Reader;

EFS_DLLAPI extern Efs_Error Efs_Rd_Reader_Open(Efs_Rd_Reader * rdr, const char * localFileName);
EFS_DLLAPI extern void Efs_Rd_Reader_Close(Efs_Rd_Reader * rdr);

EFS_DLLAPI extern size_t Efs_Rd_Reader_Callback(char * buffer, size_t size, size_t nitems, void * rdr);

#pragma pack()

#ifdef __cplusplus
}
#endif

#endif // EFS_READER_H
