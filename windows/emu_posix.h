/*
 ============================================================================
 Name        : emu_posix.h
 Author      : gosun.com
 Copyright   : 2017(c) beijing efs Information Technologies Co., Ltd.
 Description : 
 ============================================================================
 */
#ifndef EFS_EMU_POSIX_H
#define EFS_EMU_POSIX_H

#include <windows.h>
#include <sys/types.h>

#include "macro.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*============================================================================*/
/* type ssize_t */

#ifndef _W64
#define _W64
#endif

typedef _W64 int ssize_t;

/*============================================================================*/
/* type Efs_File */

#define Efs_Posix_Handle			HANDLE
#define Efs_Posix_InvalidHandle	INVALID_HANDLE_VALUE

typedef _int64 Emu_Off_T;
typedef struct _Emu_FileInfo {
	Emu_Off_T       st_size;    /* total size, in bytes */
	time_t          st_atime;   /* time of last access */
	time_t          st_mtime;   /* time of last modification */
	time_t          st_ctime;   /* time of last status change */
} Emu_FileInfo;

EFS_DLLAPI extern Efs_Posix_Handle Efs_Posix_Open(const char* file, int oflag, int mode);
EFS_DLLAPI extern ssize_t Efs_Posix_Pread2(Efs_Posix_Handle fd, void* buf, size_t nbytes, Emu_Off_T offset);
EFS_DLLAPI extern int Efs_Posix_Fstat2(Efs_Posix_Handle fd, Emu_FileInfo* buf);
EFS_DLLAPI extern int Efs_Posix_Close(Efs_Posix_Handle fd);
EFS_DLLAPI extern unsigned _int64 Efs_Posix_GetTimeOfDay(void);

/*============================================================================*/

#ifdef __cplusplus
}
#endif

#endif /* EFS_EMU_POSIX_H */
