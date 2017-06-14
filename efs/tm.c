/*
 ============================================================================
 Name        : tm.c
 Author      : gosun.com
 Copyright   : 2016(c) Shanghai Efs Information Technologies Co., Ltd.
 Description :
 ============================================================================
 */

#include "tm.h"

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef _WIN32

#include "../windows/emu_posix.h" // for type Efs_Posix_GetTimeOfDay

EFS_DLLAPI extern Efs_Uint64 Efs_Tm_LocalTime(void)
{
	return Efs_Posix_GetTimeOfDay();
} // Efs

#else

#include <sys/time.h>

EFS_DLLAPI extern Efs_Uint64 Efs_Tm_LocalTime(void)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec;
} // Efs_Tm_LocalTime

#endif

#ifdef __cplusplus
}
#endif

