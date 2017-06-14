/*
 ============================================================================
 Name        : region.c
 Author      : gosun.com
 Copyright   : 2016(c) Shanghai Efs Information Technologies Co., Ltd.
 Description :
 ============================================================================
 */

#include <ctype.h>
#include <curl/curl.h>

#include "conf.h"
#include "tm.h"
#include "region.h"

#ifdef __cplusplus
extern "C"
{
#endif

static Efs_Uint32 Efs_Rgn_enabling = 1;

EFS_DLLAPI extern void Efs_Rgn_Enable(void)
{
	Efs_Rgn_enabling = 1;
} // Efs_Rgn_Enable

EFS_DLLAPI extern void Efs_Rgn_Disable(void)
{
	Efs_Rgn_enabling = 0;
} // Efs_Rgn_Disable

EFS_DLLAPI extern Efs_Uint32 Efs_Rgn_IsEnabled(void)
{
	return (Efs_Rgn_enabling != 0);
} // Efs_Rgn_IsEnabled

static inline int Efs_Rgn_Info_isValidHost(const char * str)
{
	return (strstr(str, "http") == str) || (strstr(str, "HTTP") == str);
} // Efs_Rgn_Info_isValidHost

static void Efs_Rgn_Info_measureHosts(Efs_Json * root, Efs_Uint32 * bufLen, Efs_Uint32 * upHostCount, Efs_Uint32 * ioHostCount)
{
	int i = 0;
	Efs_Json * arr = NULL;
	const char * str = NULL;

	arr = Efs_Json_GetObjectItem(root, "up", NULL);
	if (arr) {
		while ((str = Efs_Json_GetStringAt(arr, i++, NULL))) {
			if (!Efs_Rgn_Info_isValidHost(str)) {
				// Skip URLs which contains incorrect schema.
				continue;
			} // if
			*bufLen += strlen(str) + 1;
			*upHostCount += 1;
		} // while
	} // if

	i = 0;
	arr = Efs_Json_GetObjectItem(root, "io", NULL);
	if (arr) {
		while ((str = Efs_Json_GetStringAt(arr, i++, NULL))) {
			if (!Efs_Rgn_Info_isValidHost(str)) {
				// Skip URLs which contains incorrect schema.
				continue;
			} // if
			*bufLen += strlen(str) + 1;
			*ioHostCount += 1;
		} // while
	} // if
} // Efs_Rgn_Info_measureHosts

static void Efs_Rgn_Info_duplicateHosts(Efs_Json * root, Efs_Rgn_HostInfo *** upHosts, Efs_Rgn_HostInfo *** ioHosts, char ** strPos, Efs_Uint32 hostFlags)
{
	int i = 0;
	Efs_Uint32 len = 0;
	Efs_Json * arr = NULL;
	const char * str = NULL;

	arr = Efs_Json_GetObjectItem(root, "up", NULL);
	if (arr) {
		while ((str = Efs_Json_GetStringAt(arr, i++, NULL))) {
			if (!Efs_Rgn_Info_isValidHost(str)) {
				// Skip URLs which contains incorrect schema.
				continue;
			} // if
			(**upHosts)->flags = hostFlags;
			(**upHosts)->host = *strPos;
			len = strlen(str);
			memcpy((void*)(**upHosts)->host, str, len);
			(*strPos) += len + 1;
			*upHosts += 1;
		} // while
	} // if

	i = 0;
	arr = Efs_Json_GetObjectItem(root, "io", NULL);
	if (arr) {
		while ((str = Efs_Json_GetStringAt(arr, i++, NULL))) {
			if (!Efs_Rgn_Info_isValidHost(str)) {
				// Skip URLs which contains incorrect schema.
				continue;
			} // if
			(**ioHosts)->flags = hostFlags | EFS_RGN_DOWNLOAD_HOST;
			(**ioHosts)->host = *strPos;
			len = strlen(str);
			memcpy((void*)(**ioHosts)->host, str, len);
			(*strPos) += len + 1;
			*ioHosts += 1;
		} // while
	} // if
} // Efs_Rgn_Info_duplicateHosts

EFS_DLLAPI extern Efs_Error Efs_Rgn_Info_Fetch(Efs_Client * cli, Efs_Rgn_RegionInfo ** rgnInfo, const char * bucket, const char * accessKey)
{
	Efs_Error err;
	Efs_Json * root = NULL;
	Efs_Json * http = NULL;
	Efs_Json * https = NULL;
	Efs_Uint32 i = 0;
	Efs_Uint32 upHostCount = 0;
	Efs_Uint32 ioHostCount = 0;
	Efs_Rgn_HostInfo ** upHosts = NULL;
	Efs_Rgn_HostInfo ** ioHosts = NULL;
	Efs_Rgn_RegionInfo * newRgnInfo = NULL;
	char * buf = NULL;
	char * pos = NULL;
	char * url = NULL;
	Efs_Uint32 bufLen = 0;

	url = Efs_String_Format(256, "%s/v1/query?ak=%s&bucket=%s", EFS_UC_HOST, accessKey, bucket);
	err = Efs_Client_Call(cli, &root, url);
	free(url);
	if (err.code != 200) {
		return err;
	} // if

	bufLen += sizeof(Efs_Rgn_RegionInfo) + strlen(bucket) + 1;

	http = Efs_Json_GetObjectItem(root, "http", NULL);
	if (http) {
		Efs_Rgn_Info_measureHosts(http, &bufLen, &upHostCount, &ioHostCount);
	} // if
	https = Efs_Json_GetObjectItem(root, "https", NULL);
	if (https) {
		Efs_Rgn_Info_measureHosts(https, &bufLen, &upHostCount, &ioHostCount);
	} // if
	bufLen += (sizeof(Efs_Rgn_HostInfo*) + sizeof(Efs_Rgn_HostInfo)) * (upHostCount + ioHostCount);

	buf = calloc(1, bufLen);
	if (!buf) {
		err.code = 499;
		err.message = "No enough memory";
		return err;
	} // buf

	pos = buf;

	newRgnInfo = (Efs_Rgn_RegionInfo*)pos;
	pos += sizeof(Efs_Rgn_RegionInfo);

	newRgnInfo->upHosts = upHosts = (Efs_Rgn_HostInfo**)pos;
	pos += sizeof(Efs_Rgn_HostInfo*) * upHostCount;

	for (i = 0; i < upHostCount; i += 1) {
		newRgnInfo->upHosts[i] = (Efs_Rgn_HostInfo*)(pos);
		pos += sizeof(Efs_Rgn_HostInfo);
	} // for

	newRgnInfo->ioHosts = ioHosts = (Efs_Rgn_HostInfo**)pos;
	pos += sizeof(Efs_Rgn_HostInfo*) * ioHostCount;

	for (i = 0; i < ioHostCount; i += 1) {
		newRgnInfo->ioHosts[i] = (Efs_Rgn_HostInfo*)(pos);
		pos += sizeof(Efs_Rgn_HostInfo);
	} // for

	if (http) {
		Efs_Rgn_Info_duplicateHosts(http, &upHosts, &ioHosts, &pos, EFS_RGN_HTTP_HOST);
	} // if
	if (https) {
		Efs_Rgn_Info_duplicateHosts(https, &upHosts, &ioHosts, &pos, EFS_RGN_HTTPS_HOST);
	} // if

	newRgnInfo->upHostCount = upHostCount;
	newRgnInfo->ioHostCount = ioHostCount;
	newRgnInfo->global = Efs_Json_GetBoolean(root, "global", 0);
	newRgnInfo->ttl = Efs_Json_GetInt64(root, "ttl", 86400) ;
	newRgnInfo->nextTimestampToUpdate = newRgnInfo->ttl + Efs_Tm_LocalTime();

	newRgnInfo->bucket = pos;
	memcpy((void*)newRgnInfo->bucket, bucket, strlen(bucket));

	*rgnInfo = newRgnInfo;
	return Efs_OK;
} // Efs_Rgn_Info_Fetch

static Efs_Error Efs_Rgn_parseQueryArguments(const char * uptoken, char ** bucket, char ** accessKey)
{
	Efs_Error err;
	const char * begin = uptoken;
	const char * end = uptoken;
	char * putPolicy = NULL;

	end = strchr(begin, ':');
	if (!end) {
		err.code = 9989;
		err.message = "Invalid uptoken";
		return err;
	} // if

	*accessKey = calloc(1, end - begin + 1);
	if (!*accessKey) {
		err.code = 499;
		err.message = "No enough memory";
		return err;
	} // if
	memcpy(*accessKey, begin, end - begin);

	begin = strchr(end + 1, ':');
	if (!begin) {
		free(*accessKey);
		*accessKey = NULL;
		err.code = 9989;
		err.message = "Invalid uptoken";
		return err;
	} // if

	putPolicy = Efs_String_Decode(begin);
	begin = strstr(putPolicy, "\"scope\"");
	if (!begin) {
		free(*accessKey);
		*accessKey = NULL;
		err.code = 9989;
		err.message = "Invalid uptoken";
		return err;
	} // if

	begin += strlen("\"scope\"");
	while (isspace(*begin) || *begin == ':') {
		begin += 1;
	} // while
	if (*begin != '"') {
		free(putPolicy);
		free(*accessKey);
		*accessKey = NULL;

		err.code = 9989;
		err.message = "Invalid uptoken";
		return err;
	} // if

	begin += 1;
	end = begin;
	while (1) {
		if (*end == ':' || (*end == '"' && *(end - 1) != '\\')) {
			break;
		} // if
		end += 1;
	} // while

	*bucket = calloc(1, end - begin + 1);
	if (!*bucket) {
		free(putPolicy);
		free(*accessKey);
		*accessKey = NULL;

		err.code = 499;
		err.message = "No enough memory";
		return err;
	} // if
	memcpy(*bucket, begin, end - begin);
	return Efs_OK;
} // Efs_Rgn_parseQueryArguments

EFS_DLLAPI extern Efs_Error Efs_Rgn_Info_FetchByUptoken(Efs_Client * cli, Efs_Rgn_RegionInfo ** rgnInfo, const char * uptoken)
{
	Efs_Error err;
	char * bucket = NULL;
	char * accessKey = NULL;

	err = Efs_Rgn_parseQueryArguments(uptoken, &bucket, &accessKey);
	if (err.code != 200) {
		return err;
	} // if

	err = Efs_Rgn_Info_Fetch(cli, rgnInfo, bucket, accessKey);
	free(bucket);
	free(accessKey);
	return err;
} // Efs_Rgn_Info_FetchByUptoken

EFS_DLLAPI extern void Efs_Rgn_Info_Destroy(Efs_Rgn_RegionInfo * rgnInfo)
{
	free(rgnInfo);
} // Efs_Rgn_Info_Destroy

EFS_DLLAPI extern Efs_Uint32 Efs_Rgn_Info_HasExpirated(Efs_Rgn_RegionInfo * rgnInfo)
{
	return (rgnInfo->nextTimestampToUpdate <= Efs_Tm_LocalTime());
} // Efs_Rgn_Info_HasExpirated

EFS_DLLAPI extern Efs_Uint32 Efs_Rgn_Info_CountUpHost(Efs_Rgn_RegionInfo * rgnInfo)
{
	return rgnInfo->upHostCount;
} // Efs_Rgn_Info_CountUpHost

EFS_DLLAPI extern Efs_Uint32 Efs_Rgn_Info_CountIoHost(Efs_Rgn_RegionInfo * rgnInfo)
{
	return rgnInfo->ioHostCount;
} // Efs_Rgn_Info_CountIoHost

EFS_DLLAPI extern const char * Efs_Rgn_Info_GetHost(Efs_Rgn_RegionInfo * rgnInfo, Efs_Uint32 n, Efs_Uint32 hostFlags)
{
	if ((hostFlags & EFS_RGN_HTTPS_HOST) == 0) {
		hostFlags |= EFS_RGN_HTTP_HOST;
	} // if
	if (hostFlags & EFS_RGN_DOWNLOAD_HOST) {
		if (n < rgnInfo->ioHostCount && (rgnInfo->ioHosts[n]->flags & hostFlags) == hostFlags) {
			return rgnInfo->ioHosts[n]->host;
		} // if
	} else {
		if (n < rgnInfo->upHostCount && (rgnInfo->upHosts[n]->flags & hostFlags) == hostFlags) {
			return rgnInfo->upHosts[n]->host;
		} // if
	} // if
	return NULL;
} // Efs_Rgn_Info_GetHost

EFS_DLLAPI extern Efs_Rgn_RegionTable * Efs_Rgn_Table_Create(void)
{
	Efs_Rgn_RegionTable * new_tbl = NULL;

	new_tbl = calloc(1, sizeof(Efs_Rgn_RegionTable));
	if (!new_tbl) {
		return NULL;
	} // if

	return new_tbl;
} // Efs_Rgn_Create

EFS_DLLAPI extern void Efs_Rgn_Table_Destroy(Efs_Rgn_RegionTable * rgnTable)
{
	Efs_Uint32 i = 0;
	if (rgnTable) {
		for (i = 0; i < rgnTable->rgnCount; i += 1) {
			Efs_Rgn_Info_Destroy(rgnTable->regions[i]);
		} // for
		free(rgnTable);
	} // if
} // Efs_Rgn_Destroy

EFS_DLLAPI extern Efs_Error Efs_Rgn_Table_FetchAndUpdate(Efs_Rgn_RegionTable * rgnTable, Efs_Client * cli, const char * bucket, const char * accessKey)
{
	Efs_Error err;
	Efs_Rgn_RegionInfo * newRgnInfo = NULL;
	
	err = Efs_Rgn_Info_Fetch(cli, &newRgnInfo, bucket, accessKey);
	if (err.code != 200) {
		return err;
	} // if

	return Efs_Rgn_Table_SetRegionInfo(rgnTable, newRgnInfo);
} // Efs_Rgn_Table_FetchAndUpdate

EFS_DLLAPI extern Efs_Error Efs_Rgn_Table_FetchAndUpdateByUptoken(Efs_Rgn_RegionTable * rgnTable, Efs_Client * cli, const char * uptoken)
{
	Efs_Error err;
	Efs_Rgn_RegionInfo * newRgnInfo = NULL;
	
	err = Efs_Rgn_Info_FetchByUptoken(cli, &newRgnInfo, uptoken);
	if (err.code != 200) {
		return err;
	} // if

	return Efs_Rgn_Table_SetRegionInfo(rgnTable, newRgnInfo);
} // Efs_Rgn_Table_FetchAndUpdateByUptoken

EFS_DLLAPI extern Efs_Error Efs_Rgn_Table_SetRegionInfo(Efs_Rgn_RegionTable * rgnTable, Efs_Rgn_RegionInfo * rgnInfo)
{
	Efs_Error err;
	Efs_Uint32 i = 0;
	Efs_Rgn_RegionInfo ** newRegions = NULL;

	for (i = 0; i < rgnTable->rgnCount; i += 1) {
		if (strcmp(rgnTable->regions[i]->bucket, rgnInfo->bucket) == 0) {
			Efs_Rgn_Info_Destroy(rgnTable->regions[i]);
			rgnTable->regions[i] = rgnInfo;
			return Efs_OK;
		} // if
	} // for

	newRegions = calloc(1, sizeof(Efs_Rgn_RegionInfo *) * rgnTable->rgnCount + 1);
	if (!newRegions) {
		err.code = 499;
		err.message = "No enough memory";
		return err;
	} // if

	if (rgnTable->rgnCount > 0) {
		memcpy(newRegions, rgnTable->regions, sizeof(Efs_Rgn_RegionInfo *) * rgnTable->rgnCount);
	} // if
	newRegions[rgnTable->rgnCount] = rgnInfo;

	free(rgnTable->regions);
	rgnTable->regions = newRegions;
	rgnTable->rgnCount += 1;
	
	return Efs_OK;
} // Efs_Rgn_Table_SetRegionInfo

EFS_DLLAPI extern Efs_Rgn_RegionInfo * Efs_Rgn_Table_GetRegionInfo(Efs_Rgn_RegionTable * rgnTable, const char * bucket)
{
	Efs_Uint32 i = 0;
	for (i = 0; i < rgnTable->rgnCount; i += 1) {
		if (strcmp(rgnTable->regions[i]->bucket, bucket) == 0) {
			return rgnTable->regions[i];
		} // if
	} // for
	return NULL;
} // Efs_Rgn_Table_GetRegionInfo

EFS_DLLAPI extern Efs_Error Efs_Rgn_Table_GetHost(
	Efs_Rgn_RegionTable * rgnTable,
	Efs_Client * cli,
	const char * bucket,
	const char * accessKey,
	Efs_Uint32 hostFlags,
	const char ** upHost,
	Efs_Rgn_HostVote * vote)
{
	Efs_Error err;
	Efs_Rgn_RegionInfo * rgnInfo = NULL;
	Efs_Rgn_RegionInfo * newRgnInfo = NULL;

	memset(vote, 0, sizeof(Efs_Rgn_HostVote));
	rgnInfo = Efs_Rgn_Table_GetRegionInfo(rgnTable, bucket);
	if (!rgnInfo || Efs_Rgn_Info_HasExpirated(rgnInfo)) {
		err = Efs_Rgn_Info_Fetch(cli, &newRgnInfo, bucket, accessKey);
		if (err.code != 200) {
			return err;
		} // if

		err = Efs_Rgn_Table_SetRegionInfo(rgnTable, newRgnInfo);
		if (err.code != 200) {
			return err;
		} // if

		rgnInfo = newRgnInfo;
	} // if

	if ((hostFlags & EFS_RGN_HTTPS_HOST) == 0) {
		hostFlags |= EFS_RGN_HTTP_HOST;
	} // if
	*upHost = Efs_Rgn_Info_GetHost(rgnInfo, 0, hostFlags);

	if (vote) {
		vote->rgnInfo = rgnInfo;
		vote->host = &rgnInfo->upHosts[0];
		vote->hostFlags = hostFlags;
		vote->hosts = rgnInfo->upHosts;
		vote->hostCount = rgnInfo->upHostCount;
	} // if
	return Efs_OK;
} // Efs_Rgn_Table_GetHost

EFS_DLLAPI extern Efs_Error Efs_Rgn_Table_GetHostByUptoken(
	Efs_Rgn_RegionTable * rgnTable,
	Efs_Client * cli,
	const char * uptoken,
	Efs_Uint32 hostFlags,
	const char ** upHost,
	Efs_Rgn_HostVote * vote)
{
	Efs_Error err;
	char * bucket = NULL;
	char * accessKey = NULL;

	err = Efs_Rgn_parseQueryArguments(uptoken, &bucket, &accessKey);
	if (err.code != 200) {
		return err;
	} // if

	err = Efs_Rgn_Table_GetHost(rgnTable, cli, bucket, accessKey, hostFlags, upHost, vote);
	free(bucket);
	free(accessKey);
	return Efs_OK;
} // Efs_Rgn_Table_GetHostByUptoken

static void Efs_Rgn_Vote_downgradeHost(Efs_Rgn_HostVote * vote)
{
	Efs_Rgn_HostInfo ** next = vote->host + 1;
	Efs_Rgn_HostInfo * t = NULL;

	while (next < (vote->hosts + vote->hostCount) && ((*next)->flags & vote->hostFlags) == vote->hostFlags) {
		if ((*next)->voteCount >= (*vote->host)->voteCount) {
			t = *next;
			*next = *(vote->host);
			*(vote->host) = t;
		} // if
		vote->host = next;
		next += 1;
	} // while
} // Efs_Rgn_Vote_downgradeHost

EFS_DLLAPI extern void Efs_Rgn_Table_VoteHost(Efs_Rgn_RegionTable * rgnTable, Efs_Rgn_HostVote * vote, Efs_Error err)
{
	if (!vote->rgnInfo) {
		return;
	} // if
	if (err.code >= 100) {
		(*vote->host)->voteCount += 1;
		return;
	} // if
	switch (err.code) {
		case CURLE_UNSUPPORTED_PROTOCOL:
		case CURLE_COULDNT_RESOLVE_PROXY:
		case CURLE_COULDNT_RESOLVE_HOST:
		case CURLE_COULDNT_CONNECT:
			(*vote->host)->voteCount /= 4;
			Efs_Rgn_Vote_downgradeHost(vote);
			break;

		default:
			break;
	} // switch
} // Efs_Rgn_VoteHost
