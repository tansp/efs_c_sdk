/*
 ============================================================================
 Name        : cdn.h
 Author      : gosun.com
 Copyright   : 2017(c) Shanghai Efs Information Technologies Co., Ltd.
 Description :
 ============================================================================
 */

#ifndef EFS_CDN_H
#define EFS_CDN_H

#include "base.h"
#include "macro.h"
#include "http.h"

 #ifdef __cplusplus
 extern "C"
 {
 #endif

#define TRUE 1
#define FALSE 0

typedef unsigned char BOOL;

typedef struct _Efs_Cdn_RefreshRet {
	int    code;
	char*  error;
	char*  requestId;
	char*  invalidUrls;
	char*  invalidDirs;
	int    urlQuotaDay;
	int    urlSurplusDay;
	int    dirQuotaDay;
	int    dirSurplusDay;

} Efs_Cdn_RefreshRet;

typedef struct _Efs_Cdn_PrefetchRet {
	int    code;
	char*  error;
	char*  requestId;
	char*  invalidUrls;
	int    quotaDay;
	int    surplusDay;

} Efs_Cdn_PrefetchRet;

/********************************
 *          time : string
 *     val_china : int
 *   val_oversea : int
 ********************************/
typedef struct _Efs_Cdn_FluxOrBandwidthDataItem {
	char* time;
	int   val_china;
	int   val_oversea;
}Efs_Cdn_FluxDataItem, Efs_Cdn_BandwidthDataItem;

/************************************
 *  domain : string
 *  item_a : array
 *   count : int (item array size)
 ************************************/
typedef struct _Efs_Cdn_FluxData {
	char*                   domain;
	Efs_Cdn_FluxDataItem* item_a;
	int                     count;
	BOOL                    hasValue;
} Efs_Cdn_FluxData;

/************************************
 *  domain : string
 *  item_a : array
 *   count : int (item array size)
 ************************************/
typedef struct _Efs_Cdn_BandwidthData {
	char*                        domain;
	Efs_Cdn_BandwidthDataItem* item_a;
	int                          count;
	BOOL                         hasValue;
} Efs_Cdn_BandwidthData;

/************************************
 *   code : int
 *  error : string
 * data_a : array
 *    num : int (data array size)
 ************************************/
typedef struct _Efs_Cdn_FluxRet {
	int                 code;
	char*               error;
	Efs_Cdn_FluxData* data_a;
	int                 num;
}Efs_Cdn_FluxRet;

typedef struct _Efs_Cdn_BandwidthRet {
	int                      code;
	char*                    error;
	Efs_Cdn_BandwidthData* data_a;
	int                      num;
}Efs_Cdn_BandwidthRet;

/********************************
 *       name : string
 *       size : int
 *      mtime : int
 *        url : string
 ********************************/
typedef struct _Efs_Cdn_LogListDataItem {
	char* name;
	int   size;
	int   mtime;
	char* url;
}Efs_Cdn_LogListDataItem;

/************************************
 *   domain : string
 *   item_a : array
 *    count : int (item array size)
 * hasValue : BOOL
 ************************************/
typedef struct _Efs_Cdn_LogListData {
	char*                      domain;
	Efs_Cdn_LogListDataItem* item_a;
	int                        count;
	BOOL                       hasValue;
}Efs_Cdn_LogListData;

/************************************
 *    code : int
 *   error : string
 *  data_a : array
 *     num : int (data array size)
 ************************************/
typedef struct _Efs_Cdn_LogListRet {
	int                    code;
	char*                  error;
	Efs_Cdn_LogListData* data_a;
	int                    num;
}Efs_Cdn_LogListRet;

#pragma pack(1)

EFS_DLLAPI extern char * Efs_Cdn_MakeDownloadUrlWithDeadline(const char * key, const char * url, Efs_Uint64 deadline);

#pragma pack()

//=====================================================================
// ADDED CDN FUNCTIONS:
// 1. RefreshUrls
// 2. RefreshDirs
// 3. PrefetchUrls
// 4. GetFluxData
// 5. GetBandwidthData
// 6. GetLogList
// *. Parse/Free
// MODIFIED by fengyh 2017-03-28 11:50
//======================================================================

EFS_DLLAPI extern Efs_Error Efs_Cdn_RefreshUrls(Efs_Client* self, Efs_Cdn_RefreshRet* ret, const char* urls[], const int num);

EFS_DLLAPI extern Efs_Error Efs_Cdn_RefreshDirs(Efs_Client* self, Efs_Cdn_RefreshRet* ret, const char* dirs[], const int num);

EFS_DLLAPI extern Efs_Error Efs_Cdn_PrefetchUrls(Efs_Client* self, Efs_Cdn_PrefetchRet* ret, const char* urls[], const int num);

EFS_DLLAPI extern Efs_Error Efs_Cdn_GetFluxData(Efs_Client* self, Efs_Cdn_FluxRet* ret,
	const char* startDate, const char* endDate, const char* granularity, const char* domains[], const int num);

EFS_DLLAPI extern Efs_Error Efs_Cdn_GetBandwidthData(Efs_Client* self, Efs_Cdn_BandwidthRet* ret,
	const char* startDate, const char* endDate, const char* granularity, const char* domains[], const int num);

EFS_DLLAPI extern Efs_Error Efs_Cdn_GetLogList(Efs_Client* self, Efs_Cdn_LogListRet* ret,
	const char* day, const char* domains[], const int num);

EFS_DLLAPI extern Efs_Error Efs_Parse_CdnRefreshRet(Efs_Json* root, Efs_Cdn_RefreshRet* ret);

EFS_DLLAPI extern Efs_Error Efs_Parse_CdnPrefetchRet(Efs_Json* root, Efs_Cdn_PrefetchRet* ret);

EFS_DLLAPI extern Efs_Error Efs_Parse_CdnFluxRet(Efs_Json* root, Efs_Cdn_FluxRet* ret, const char* domains[], const int num);

EFS_DLLAPI extern Efs_Error Efs_Parse_CdnBandwidthRet(Efs_Json* root, Efs_Cdn_BandwidthRet* ret, const char* domains[], const int num);

EFS_DLLAPI extern Efs_Error Efs_Parse_CdnLogListRet(Efs_Json* root, Efs_Cdn_LogListRet* ret, const char* domains[], const int num);

EFS_DLLAPI extern void Efs_Free_CdnRefreshRet(Efs_Cdn_RefreshRet* ret);

EFS_DLLAPI extern void Efs_Free_CdnPrefetchRet(Efs_Cdn_PrefetchRet* ret);

EFS_DLLAPI extern void Efs_Free_CdnFluxRet(Efs_Cdn_FluxRet* ret);

EFS_DLLAPI extern void Efs_Free_CdnBandwidthRet(Efs_Cdn_BandwidthRet* ret);

EFS_DLLAPI extern void Efs_Free_CdnLogListRet(Efs_Cdn_LogListRet* ret);

//=====================================================================

#ifdef __cplusplus
}
#endif

#endif // EFS_CDN_H
