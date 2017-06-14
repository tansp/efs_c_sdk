/*
 ============================================================================
 Name		: region.h
 Author	  : gosun.com
 Copyright   : 2016(c) Shanghai Efs Information Technologies Co., Ltd.
 Description : 
 ============================================================================
 */

#ifndef EFS_REGION_H
#define EFS_REGION_H

#include "http.h"

#pragma pack(1)

#ifdef __cplusplus
extern "C"
{
#endif

EFS_DLLAPI extern void Efs_Rgn_Enable(void);
EFS_DLLAPI extern void Efs_Rgn_Disable(void);
EFS_DLLAPI extern Efs_Uint32 Efs_Rgn_IsEnabled(void);

enum {
	EFS_RGN_HTTP_HOST         = 0x0001,
	EFS_RGN_HTTPS_HOST        = 0x0002,
	EFS_RGN_CDN_HOST          = 0x0004,
	EFS_RGN_DOWNLOAD_HOST     = 0x0008
};

typedef struct _Efs_Rgn_HostInfo {
	const char * host;
	Efs_Uint32 flags;
	Efs_Uint32 voteCount;
} Efs_Rgn_HostInfo;

typedef struct _Efs_Rgn_RegionInfo {
	Efs_Uint64 nextTimestampToUpdate;

	const char * bucket;

	Efs_Int64 ttl;
	Efs_Int64 global;

	Efs_Uint32 upHostCount;
	Efs_Rgn_HostInfo ** upHosts;

	Efs_Uint32 ioHostCount;
	Efs_Rgn_HostInfo ** ioHosts;
} Efs_Rgn_RegionInfo;

EFS_DLLAPI extern Efs_Error Efs_Rgn_Info_Fetch(Efs_Client * cli, Efs_Rgn_RegionInfo ** rgnInfo, const char * bucket, const char * accessKey);
EFS_DLLAPI extern Efs_Error Efs_Rgn_Info_FetchByUptoken(Efs_Client * cli, Efs_Rgn_RegionInfo ** rgnInfo, const char * uptoken);
EFS_DLLAPI extern void Efs_Rgn_Info_Destroy(Efs_Rgn_RegionInfo * rgnInfo);
EFS_DLLAPI extern Efs_Uint32 Efs_Rgn_Info_HasExpirated(Efs_Rgn_RegionInfo * rgnInfo);
EFS_DLLAPI extern Efs_Uint32 Efs_Rgn_Info_CountUpHost(Efs_Rgn_RegionInfo * rgnInfo);
EFS_DLLAPI extern Efs_Uint32 Efs_Rgn_Info_CountIoHost(Efs_Rgn_RegionInfo * rgnInfo);
EFS_DLLAPI extern const char * Efs_Rgn_Info_GetHost(Efs_Rgn_RegionInfo * rgnInfo, Efs_Uint32 n, Efs_Uint32 hostFlags);
EFS_DLLAPI extern const char * Efs_Rgn_Info_GetIoHost(Efs_Rgn_RegionInfo * rgnInfo, Efs_Uint32 n, Efs_Uint32 hostFlags);

typedef struct _Efs_Rgn_RegionTable {
	Efs_Uint32 rgnCount;
	Efs_Rgn_RegionInfo ** regions;
} Efs_Rgn_RegionTable;

typedef struct _Efs_Rgn_HostVote {
	Efs_Rgn_RegionInfo * rgnInfo;
	Efs_Rgn_HostInfo ** host;
	Efs_Rgn_HostInfo ** hosts;
	Efs_Uint32 hostCount;
	Efs_Uint32 hostFlags;
} Efs_Rgn_HostVote;

EFS_DLLAPI extern Efs_Rgn_RegionTable * Efs_Rgn_Table_Create(void);
EFS_DLLAPI extern void Efs_Rgn_Table_Destroy(Efs_Rgn_RegionTable * rgnTable);

EFS_DLLAPI extern Efs_Error Efs_Rgn_Table_FetchAndUpdate(Efs_Rgn_RegionTable * rgnTable, Efs_Client * cli, const char * bucket, const char * access_key);
EFS_DLLAPI extern Efs_Error Efs_Rgn_Table_FetchAndUpdateByUptoken(Efs_Rgn_RegionTable * rgnTable, Efs_Client * cli, const char * uptoken);
EFS_DLLAPI extern Efs_Error Efs_Rgn_Table_SetRegionInfo(Efs_Rgn_RegionTable * rgnTable, Efs_Rgn_RegionInfo * rgnInfo);
EFS_DLLAPI extern Efs_Rgn_RegionInfo * Efs_Rgn_Table_GetRegionInfo(Efs_Rgn_RegionTable * rgnTable, const char * bucket);
EFS_DLLAPI extern Efs_Error Efs_Rgn_Table_GetHost(Efs_Rgn_RegionTable * rgnTable, Efs_Client * cli, const char * bucket, const char * accessKey, Efs_Uint32 hostFlags, const char ** upHost, Efs_Rgn_HostVote * vote);
EFS_DLLAPI extern Efs_Error Efs_Rgn_Table_GetHostByUptoken(Efs_Rgn_RegionTable * rgnTable, Efs_Client * cli, const char * uptoken, Efs_Uint32 hostFlags, const char ** upHost, Efs_Rgn_HostVote * vote);
EFS_DLLAPI extern void Efs_Rgn_Table_VoteHost(Efs_Rgn_RegionTable * rgnTable, Efs_Rgn_HostVote * vote, Efs_Error err);

#ifdef __cplusplus
}
#endif

#pragma pack()

#endif // EFS_REGION_H
