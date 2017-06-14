/*
 ============================================================================
 Name        : rs.c
 Author      : gosun.com
 Copyright   : 2017(c) beijing efs Information Technologies Co., Ltd.
 Description : 
 ============================================================================
 */
#include "rs.h"
#include "../cJSON/cJSON.h"
#include <time.h>

/*============================================================================*/
/* type Efs_RS_PutPolicy/GetPolicy */

char* Efs_RS_PutPolicy_Token(Efs_RS_PutPolicy* auth, Efs_Mac* mac)
{
	int expires;
	time_t deadline;
	char* authstr;
	char* token;

	cJSON* root = cJSON_CreateObject();

	if (auth->scope) {
		cJSON_AddStringToObject(root, "scope", auth->scope);
	}
	if (auth->callbackUrl) {
		cJSON_AddStringToObject(root, "callbackUrl", auth->callbackUrl);
	}
	if (auth->callbackBody) {
		cJSON_AddStringToObject(root, "callbackBody", auth->callbackBody);
	}
	if (auth->asyncOps) {
		cJSON_AddStringToObject(root, "asyncOps", auth->asyncOps);
	}
	if (auth->returnUrl) {
		cJSON_AddStringToObject(root, "returnUrl", auth->returnUrl);
	}
	if (auth->returnBody) {
		cJSON_AddStringToObject(root, "returnBody", auth->returnBody);
	}
	if (auth->endUser) {
		cJSON_AddStringToObject(root, "endUser", auth->endUser);
	}
	if (auth->persistentOps) {
		cJSON_AddStringToObject(root, "persistentOps", auth->persistentOps);
	}
	if (auth->persistentNotifyUrl) {
		cJSON_AddStringToObject(root, "persistentNotifyUrl", auth->persistentNotifyUrl);
	}
	if (auth->persistentPipeline) {
		cJSON_AddStringToObject(root, "persistentPipeline", auth->persistentPipeline);
	}
	if (auth->mimeLimit) {
		cJSON_AddStringToObject(root, "mimeLimit", auth->mimeLimit);
	}

	if (auth->fsizeLimit) {
		cJSON_AddNumberToObject(root, "fsizeLimit", auth->fsizeLimit);
	}
	if (auth->detectMime) {
		cJSON_AddNumberToObject(root, "detectMime", auth->detectMime);
	}
	if (auth->insertOnly) {
		cJSON_AddNumberToObject(root, "insertOnly", auth->insertOnly);
	}
    if (auth->deleteAfterDays > 0) {
		cJSON_AddNumberToObject(root, "deleteAfterDays", auth->deleteAfterDays);
    }

	if (auth->expires) {
		expires = auth->expires;
	} else {
		expires = 3600; // 1小时
	}
	time(&deadline);
	deadline += expires;
	cJSON_AddNumberToObject(root, "deadline", deadline);

	authstr = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);

	token = Efs_Mac_SignToken(mac, authstr);
	Efs_Free(authstr);

	return token;
}

char* Efs_RS_GetPolicy_MakeRequest(Efs_RS_GetPolicy* auth, const char* baseUrl, Efs_Mac* mac)
{
	int expires;
	time_t deadline;
	char  e[11];
	char* authstr;
	char* token;
	char* request;

	if (auth->expires) {
		expires = auth->expires;
	} else {
		expires = 3600; // 1小时
	}
	time(&deadline);
	deadline += expires;
	sprintf(e, "%u", (unsigned int)deadline);

	if (strchr(baseUrl, '?') != NULL) {
		authstr = Efs_String_Concat3(baseUrl, "&e=", e);
	} else {
		authstr = Efs_String_Concat3(baseUrl, "?e=", e);
	}

	token = Efs_Mac_Sign(mac, authstr);

	request = Efs_String_Concat3(authstr, "&token=", token);

	Efs_Free(token);
	Efs_Free(authstr);

	return request;
}

char* Efs_RS_MakeBaseUrl(const char* domain, const char* key)
{
	Efs_Bool fesc;
	char* baseUrl;
	char* escapedKey = Efs_PathEscape(key, &fesc);

	baseUrl = Efs_String_Concat("http://", domain, "/", escapedKey, NULL);

	if (fesc) {
		Efs_Free(escapedKey);
	}

	return baseUrl;
}

/*============================================================================*/
/* func Efs_RS_Stat */

Efs_Error Efs_RS_Stat(
	Efs_Client* self, Efs_RS_StatRet* ret, const char* tableName, const char* key)
{
	Efs_Error err;
	cJSON* root;

	char* entryURI = Efs_String_Concat3(tableName, ":", key);
	char* entryURIEncoded = Efs_String_Encode(entryURI);
	char* url = Efs_String_Concat3(EFS_RS_HOST, "/stat/", entryURIEncoded);

	Efs_Free(entryURI);
	Efs_Free(entryURIEncoded);

	err = Efs_Client_Call(self, &root, url);
	Efs_Free(url);

	if (err.code == 200) {
		ret->hash = Efs_Json_GetString(root, "hash", 0);
		ret->mimeType = Efs_Json_GetString(root, "mimeType", 0);
		ret->fsize = Efs_Json_GetInt64(root, "fsize", 0);
		ret->putTime = Efs_Json_GetInt64(root, "putTime", 0);
	}
	return err;
}

/*============================================================================*/
/* func Efs_RS_Delete */

Efs_Error Efs_RS_Delete(Efs_Client* self, const char* tableName, const char* key)
{
	Efs_Error err;

	char* entryURI = Efs_String_Concat3(tableName, ":", key);
	char* entryURIEncoded = Efs_String_Encode(entryURI);
	char* url = Efs_String_Concat3(EFS_RS_HOST, "/delete/", entryURIEncoded);

	Efs_Free(entryURI);
	Efs_Free(entryURIEncoded);

	err = Efs_Client_CallNoRet(self, url);
	Efs_Free(url);

	return err;
}

/*============================================================================*/
/* func Efs_RS_Copy */

Efs_Error Efs_RS_Copy(Efs_Client* self, 
	const char* tableNameSrc, const char* keySrc,
	const char* tableNameDest, const char* keyDest)
{
	Efs_Error err;

	char* entryURISrc = Efs_String_Concat3(tableNameSrc, ":", keySrc);
	char* entryURISrcEncoded = Efs_String_Encode(entryURISrc);
	char* entryURIDest = Efs_String_Concat3(tableNameDest, ":", keyDest);
	char* entryURIDestEncoded = Efs_String_Encode(entryURIDest);
	char* urlPart = Efs_String_Concat3(entryURISrcEncoded, "/", entryURIDestEncoded);

	char* url = Efs_String_Concat3(EFS_RS_HOST, "/copy/", urlPart);

	free(entryURISrc);
	free(entryURISrcEncoded);
	free(entryURIDest);
	free(entryURIDestEncoded);
	free(urlPart);

	err = Efs_Client_CallNoRet(self, url);
	free(url);

	return err;
}

/*============================================================================*/
/* func Efs_RS_Move */

Efs_Error Efs_RS_Move(Efs_Client* self, 
	const char* tableNameSrc, const char* keySrc,
	const char* tableNameDest, const char* keyDest)
{
	Efs_Error err;

	char* entryURISrc = Efs_String_Concat3(tableNameSrc, ":", keySrc);
	char* entryURISrcEncoded = Efs_String_Encode(entryURISrc);
	char* entryURIDest = Efs_String_Concat3(tableNameDest, ":", keyDest);
	char* entryURIDestEncoded = Efs_String_Encode(entryURIDest);
	char* urlPart = Efs_String_Concat3(entryURISrcEncoded, "/", entryURIDestEncoded);
	char* url = Efs_String_Concat3(EFS_RS_HOST, "/move/", urlPart);

	free(entryURISrc);
	free(entryURISrcEncoded);
	free(entryURIDest);
	free(entryURIDestEncoded);
	free(urlPart);

	err = Efs_Client_CallNoRet(self, url);
	free(url);

	return err;
}

/*============================================================================*/
/* func Efs_RS_BatchStat */

Efs_Error Efs_RS_BatchStat(
	Efs_Client* self, Efs_RS_BatchStatRet* rets,
	Efs_RS_EntryPath* entries, Efs_ItemCount entryCount)
{
	int code;
	Efs_Error err;
	cJSON *root, *arrayItem, *dataItem;
	char *body = NULL, *bodyTmp = NULL;
	char *entryURI, *entryURIEncoded, *opBody;
	Efs_RS_EntryPath* entry = entries;
	Efs_ItemCount curr = 0;
	Efs_ItemCount retSize = 0;
	char* url = Efs_String_Concat2(EFS_RS_HOST, "/batch");

	while (curr < entryCount) {
		entryURI = Efs_String_Concat3(entry->bucket, ":", entry->key);
		entryURIEncoded = Efs_String_Encode(entryURI);
		opBody = Efs_String_Concat2("op=/stat/", entryURIEncoded);
		free(entryURI);
		free(entryURIEncoded);

		if (!body) {
			bodyTmp = opBody;
		} else {
			bodyTmp = Efs_String_Concat3(body, "&", opBody);
			free(opBody);
		}
		free(body);
		body = bodyTmp;
		curr++;
		entry = &entries[curr];
	}

	err = Efs_Client_CallWithBuffer(self, &root, 
	url, body, strlen(body), "application/x-www-form-urlencoded");
	free(url);
	/*
	 * Bug No.(4672) Wang Xiaotao  2013\10\15 17:56:00 
	 * Change for : free  var 'body'
	 * Reason     : memory leak!
	 */
	free(body);

	retSize = cJSON_GetArraySize(root);
	
	curr = 0;
	while (curr < retSize) {
		arrayItem = cJSON_GetArrayItem(root, curr);
		code = (int)Efs_Json_GetInt64(arrayItem, "code", 0);
		dataItem = cJSON_GetObjectItem(arrayItem, "data");

		rets[curr].code = code;

		if (code != 200) {
			rets[curr].error = Efs_Json_GetString(dataItem, "error", 0);
		} else {
			rets[curr].data.hash = Efs_Json_GetString(dataItem, "hash", 0);
			rets[curr].data.mimeType = Efs_Json_GetString(dataItem, "mimeType", 0);
			rets[curr].data.fsize = Efs_Json_GetInt64(dataItem, "fsize", 0);
			rets[curr].data.putTime = Efs_Json_GetInt64(dataItem, "putTime", 0);
		}
		curr++;
	}

	return err;
}

/*============================================================================*/
/* func Efs_RS_BatchDelete */

Efs_Error Efs_RS_BatchDelete(
	Efs_Client* self, Efs_RS_BatchItemRet* rets,
	Efs_RS_EntryPath* entries, Efs_ItemCount entryCount)
{
	int code;
	Efs_Error err;
	cJSON *root, *arrayItem, *dataItem;
	char *body = NULL, *bodyTmp = NULL;
	char *entryURI, *entryURIEncoded, *opBody;
	Efs_ItemCount curr = 0;
	Efs_ItemCount retSize = 0;
	Efs_RS_EntryPath* entry = entries;
	char* url = Efs_String_Concat2(EFS_RS_HOST, "/batch");

	curr = 0;
	while (curr < entryCount) {
		entryURI = Efs_String_Concat3(entry->bucket, ":", entry->key);
		entryURIEncoded = Efs_String_Encode(entryURI);
		opBody = Efs_String_Concat2("op=/delete/", entryURIEncoded);
		free(entryURI);
		free(entryURIEncoded);

		if (!body) {
			bodyTmp = opBody;
		} else {
			bodyTmp = Efs_String_Concat3(body, "&", opBody);
			free(opBody);
		}
		free(body);
		body = bodyTmp;
		curr++;
		entry = &entries[curr];
	}

	err = Efs_Client_CallWithBuffer(self, &root, 
	url, body, strlen(body), "application/x-www-form-urlencoded");
	free(url);
	/*
	 * Bug No.(4672) Wang Xiaotao  2013\10\15 17:56:00 
	 * Change for : free  var 'body'
	 * Reason     : memory leak!
	 */
	free(body);

	retSize = cJSON_GetArraySize(root);

	curr = 0;
	while (curr < retSize) {
		arrayItem = cJSON_GetArrayItem(root, curr);
		code = (int)Efs_Json_GetInt64(arrayItem, "code", 0);
		dataItem = cJSON_GetObjectItem(arrayItem, "data");

		rets[curr].code = code;

		if (code != 200) {
			rets[curr].error = Efs_Json_GetString(dataItem, "error", 0);
		}
		curr++;
	}

	return err;
}

/*============================================================================*/
/* func Efs_RS_BatchMove */

Efs_Error Efs_RS_BatchMove(
	Efs_Client* self, Efs_RS_BatchItemRet* rets,
	Efs_RS_EntryPathPair* entryPairs, Efs_ItemCount entryCount)
{
	int code;
	Efs_Error err;
	cJSON *root, *arrayItem, *dataItem;
	char *body = NULL, *bodyTmp = NULL;
	char *entryURISrc, *entryURISrcEncoded, *opBody;
	char *entryURIDest, *entryURIDestEncoded, *bodyPart;
	Efs_ItemCount curr = 0;
	Efs_ItemCount retSize = 0;
	Efs_RS_EntryPathPair* entryPair = entryPairs;
	char* url = Efs_String_Concat2(EFS_RS_HOST, "/batch");

	curr = 0;
	while (curr < entryCount) {
		entryURISrc = Efs_String_Concat3(entryPair->src.bucket, ":", entryPair->src.key);
		entryURISrcEncoded = Efs_String_Encode(entryURISrc);
		entryURIDest = Efs_String_Concat3(entryPair->dest.bucket, ":", entryPair->dest.key);
		entryURIDestEncoded = Efs_String_Encode(entryURIDest);

		bodyPart = Efs_String_Concat3(entryURISrcEncoded, "/", entryURIDestEncoded);
		opBody = Efs_String_Concat2("op=/move/", bodyPart);
		free(entryURISrc);
		free(entryURISrcEncoded);
		free(entryURIDest);
		free(entryURIDestEncoded);
		free(bodyPart);

		if (!body) {
			bodyTmp = opBody;
		} else {
			bodyTmp = Efs_String_Concat3(body, "&", opBody);
			free(opBody);
		}
		free(body);
		body = bodyTmp;
		curr++;
		entryPair = &entryPairs[curr];
	}

	err = Efs_Client_CallWithBuffer(self, &root, 
	url, body, strlen(body), "application/x-www-form-urlencoded");
	free(url);
	/*
	 * Bug No.(4672) Wang Xiaotao  2013\10\15 17:56:00 
	 * Change for : free  var 'body'
	 * Reason     : memory leak!
	 */
	free(body);

	retSize = cJSON_GetArraySize(root);
	
	curr = 0;
	while (curr < retSize) {
		arrayItem = cJSON_GetArrayItem(root, curr);
		code = (int)Efs_Json_GetInt64(arrayItem, "code", 0);
		dataItem = cJSON_GetObjectItem(arrayItem, "data");

		rets[curr].code = code;

		if (code != 200) {
			rets[curr].error = Efs_Json_GetString(dataItem, "error", 0);
		}
		curr++;
	}

	return err;
}

/*============================================================================*/
/* func Efs_RS_BatchCopy */

Efs_Error Efs_RS_BatchCopy(
	Efs_Client* self, Efs_RS_BatchItemRet* rets,
	Efs_RS_EntryPathPair* entryPairs, Efs_ItemCount entryCount)
{
	int code;
	Efs_Error err;
	cJSON *root, *arrayItem, *dataItem;
	char *body = NULL, *bodyTmp = NULL;
	char *entryURISrc, *entryURISrcEncoded, *opBody;
	char *entryURIDest, *entryURIDestEncoded, *bodyPart;
	Efs_ItemCount curr = 0;
	Efs_ItemCount retSize = 0;
	Efs_RS_EntryPathPair* entryPair = entryPairs;
	char* url = Efs_String_Concat2(EFS_RS_HOST, "/batch");

	curr = 0;
	while (curr < entryCount) {
		entryURISrc = Efs_String_Concat3(entryPair->src.bucket, ":", entryPair->src.key);
		entryURISrcEncoded = Efs_String_Encode(entryURISrc);
		entryURIDest = Efs_String_Concat3(entryPair->dest.bucket, ":", entryPair->dest.key);
		entryURIDestEncoded = Efs_String_Encode(entryURIDest);
		
		bodyPart = Efs_String_Concat3(entryURISrcEncoded, "/", entryURIDestEncoded);
		opBody = Efs_String_Concat2("op=/copy/", bodyPart);
		free(entryURISrc);
		free(entryURISrcEncoded);
		free(entryURIDest);
		free(entryURIDestEncoded);
		free(bodyPart);

		if (!body) {
			bodyTmp = opBody;
		} else {
			bodyTmp = Efs_String_Concat3(body, "&", opBody);
			free(opBody);
		}
		free(body);
		body = bodyTmp;
		curr++;
		entryPair = &entryPairs[curr];
	}

	err = Efs_Client_CallWithBuffer(self, &root, 
	url, body, strlen(body), "application/x-www-form-urlencoded");
	free(url);
	/*
	 * Bug No.(4672) Wang Xiaotao  2013\10\15 17:56:00 
	 * Change for : free  var 'body'
	 * Reason     : memory leak!
	 */
	free(body);

	retSize = cJSON_GetArraySize(root);
	
	curr = 0;
	while (curr < retSize) {
		arrayItem = cJSON_GetArrayItem(root, curr);
		code = (int)Efs_Json_GetInt64(arrayItem, "code", 0);
		dataItem = cJSON_GetObjectItem(arrayItem, "data");

		rets[curr].code = code;

		if (code != 200) {
			rets[curr].error = Efs_Json_GetString(dataItem, "error", 0);
		}
		curr++;
	}

	return err;
}
