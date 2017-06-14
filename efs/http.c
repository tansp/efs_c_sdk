/*
 ============================================================================
 Name        : http.c
 Author      : gosun.com
 Copyright   : 2017(c) beijing efs Information Technologies Co., Ltd.
 Description :
 ============================================================================
 */

#include "http.h"
#include "region.h"
#include "../cJSON/cJSON.h"
#include <curl/curl.h>

#if defined(_WIN32)
#pragma comment(lib, "curllib.lib")
#endif

/*============================================================================*/
/* type Efs_Mutex */

#if defined(_WIN32)

void Efs_Mutex_Init(Efs_Mutex* self)
{
	InitializeCriticalSection(self);
}

void Efs_Mutex_Cleanup(Efs_Mutex* self)
{
	DeleteCriticalSection(self);
}

void Efs_Mutex_Lock(Efs_Mutex* self)
{
	EnterCriticalSection(self);
}

void Efs_Mutex_Unlock(Efs_Mutex* self)
{
	LeaveCriticalSection(self);
}

#else

void Efs_Mutex_Init(Efs_Mutex* self)
{
	pthread_mutex_init(self, NULL);
}

void Efs_Mutex_Cleanup(Efs_Mutex* self)
{
	pthread_mutex_destroy(self);
}

void Efs_Mutex_Lock(Efs_Mutex* self)
{
	pthread_mutex_lock(self);
}

void Efs_Mutex_Unlock(Efs_Mutex* self)
{
	pthread_mutex_unlock(self);
}

#endif

/*============================================================================*/
/* Global */

void Efs_Buffer_formatInit();

void Efs_Global_Init(long flags)
{
	Efs_Buffer_formatInit();
	Efs_Rgn_Enable();
	curl_global_init(CURL_GLOBAL_ALL);
}

void Efs_Global_Cleanup()
{
	curl_global_cleanup();
}

/*============================================================================*/
/* func Efs_call */

static const char g_statusCodeError[] = "http status code is not OK";

Efs_Error Efs_callex(CURL* curl, Efs_Buffer *resp, Efs_Json** ret, Efs_Bool simpleError, Efs_Buffer *resph)
{
	Efs_Error err;
	CURLcode curlCode;
	long httpCode;
	Efs_Json* root;

	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, Efs_Buffer_Fwrite);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, resp);
	if (resph != NULL) {
		curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, Efs_Buffer_Fwrite);
		curl_easy_setopt(curl, CURLOPT_WRITEHEADER, resph);
	}

	curlCode = curl_easy_perform(curl);

	if (curlCode == 0) {
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
		if (Efs_Buffer_Len(resp) != 0) {
			root = cJSON_Parse(Efs_Buffer_CStr(resp));
		} else {
			root = NULL;
		}
		*ret = root;
		err.code = (int)httpCode;
		if (httpCode / 100 != 2) {
			if (simpleError) {
				err.message = g_statusCodeError;
			} else {
				err.message = Efs_Json_GetString(root, "error", g_statusCodeError);
			}
		} else {
			err.message = "OK";
		}
	} else {
		*ret = NULL;
		err.code = curlCode;
		err.message = "curl_easy_perform error";
	}

	return err;
}

/*============================================================================*/
/* type Efs_Json */

const char* Efs_Json_GetString(Efs_Json* self, const char* key, const char* defval)
{
	Efs_Json* sub;
	if (self == NULL) {
		return defval;
	}
	sub = cJSON_GetObjectItem(self, key);
	if (sub != NULL && sub->type == cJSON_String) {
		return sub->valuestring;
	} else {
		return defval;
	}
}

const char* Efs_Json_GetStringAt(Efs_Json* self, int n, const char* defval)
{
	Efs_Json* sub;
	if (self == NULL) {
		return defval;
	}
	sub = cJSON_GetArrayItem(self, n);
	if (sub != NULL && sub->type == cJSON_String) {
		return sub->valuestring;
	} else {
		return defval;
	}
}

Efs_Int64 Efs_Json_GetInt64(Efs_Json* self, const char* key, Efs_Int64 defval)
{
	Efs_Json* sub;
	if (self == NULL) {
		return defval;
	}
	sub = cJSON_GetObjectItem(self, key);
	if (sub != NULL && sub->type == cJSON_Number) {
		return (Efs_Int64)sub->valuedouble;
	} else {
		return defval;
	}
}

int Efs_Json_GetBoolean(Efs_Json* self, const char* key, int defval)
{
	Efs_Json* sub;
	if (self == NULL) {
		return defval;
	} // if
	sub = cJSON_GetObjectItem(self, key);
	if (sub != NULL) {
		if (sub->type == cJSON_False) {
			return 0;
		} else if (sub->type == cJSON_True) {
			return 1;
		} // if
	} // if
	return defval;
} // Efs_Json_GetBoolean

Efs_Json* Efs_Json_GetObjectItem(Efs_Json* self, const char* key, Efs_Json* defval)
{
	Efs_Json* sub;
	if (self == NULL) {
		return defval;
	}
	sub = cJSON_GetObjectItem(self, key);
	if (sub != NULL) {
		return sub;
	} else {
		return defval;
	}
} // Efs_Json_GetObjectItem

Efs_Json* Efs_Json_GetArrayItem(Efs_Json* self, int n, Efs_Json* defval)
{
	Efs_Json* sub;
	if (self == NULL) {
		return defval;
	}
	sub = cJSON_GetArrayItem(self, n);
	if (sub != NULL) {
		return sub;
	} else {
		return defval;
	}
} // Efs_Json_GetArrayItem

void Efs_Json_Destroy(Efs_Json* self)
{
	cJSON_Delete(self);
} // Efs_Json_Destroy

Efs_Uint32 Efs_Json_GetInt(Efs_Json* self, const char* key, Efs_Uint32 defval)
{
	Efs_Json* sub;
	if (self == NULL) {
		return defval;
	}
	sub = cJSON_GetObjectItem(self, key);
	if (sub != NULL && sub->type == cJSON_Number) {
		return (Efs_Uint32)sub->valueint;
	} else {
		return defval;
	}
}

/*============================================================================*/
/* type Efs_Client */

Efs_Auth Efs_NoAuth = {
	NULL,
	NULL
};

void Efs_Client_InitEx(Efs_Client* self, Efs_Auth auth, size_t bufSize)
{
	self->curl = curl_easy_init();
	self->root = NULL;
	self->auth = auth;

	Efs_Buffer_Init(&self->b, bufSize);
	Efs_Buffer_Init(&self->respHeader, bufSize);

	self->boundNic = NULL;

	self->lowSpeedLimit = 0;
	self->lowSpeedTime = 0;

	self->regionTable = Efs_Rgn_Table_Create();
}

void Efs_Client_InitNoAuth(Efs_Client* self, size_t bufSize)
{
	Efs_Client_InitEx(self, Efs_NoAuth, bufSize);
}

void Efs_Client_Cleanup(Efs_Client* self)
{
	if (self->auth.itbl != NULL) {
		self->auth.itbl->Release(self->auth.self);
		self->auth.itbl = NULL;
	}
	if (self->curl != NULL) {
		curl_easy_cleanup((CURL*)self->curl);
		self->curl = NULL;
	}
	if (self->root != NULL) {
		cJSON_Delete(self->root);
		self->root = NULL;
	}
	if (self->regionTable != NULL) {
		Efs_Rgn_Table_Destroy(self->regionTable);
		self->regionTable = NULL;
	} // if
	Efs_Buffer_Cleanup(&self->b);
	Efs_Buffer_Cleanup(&self->respHeader);
}

void Efs_Client_BindNic(Efs_Client* self, const char* nic)
{
	self->boundNic = nic;
} // Efs_Client_BindNic

void Efs_Client_SetLowSpeedLimit(Efs_Client* self, long lowSpeedLimit, long lowSpeedTime)
{
	self->lowSpeedLimit = lowSpeedLimit;
	self->lowSpeedTime = lowSpeedTime;
} // Efs_Client_SetLowSpeedLimit

CURL* Efs_Client_reset(Efs_Client* self)
{
	CURL* curl = (CURL*)self->curl;

	curl_easy_reset(curl);
	Efs_Buffer_Reset(&self->b);
	Efs_Buffer_Reset(&self->respHeader);
	if (self->root != NULL) {
		cJSON_Delete(self->root);
		self->root = NULL;
	}
	
	// Set this option to allow multi-threaded application to get signals, etc
	// Setting CURLOPT_NOSIGNAL to 1 makes libcurl NOT ask the system to ignore SIGPIPE signals
	// See also https://curl.haxx.se/libcurl/c/CURLOPT_NOSIGNAL.html
	// FIXED by fengyh 2017-03-22 10:30
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL,1);

	return curl;
}

static CURL* Efs_Client_initcall(Efs_Client* self, const char* url)
{
	CURL* curl = Efs_Client_reset(self);

	curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
	curl_easy_setopt(curl, CURLOPT_URL, url);

	return curl;
}

static Efs_Error Efs_Client_callWithBody(
	Efs_Client* self, Efs_Json** ret, const char* url,
	const char* body, Efs_Int64 bodyLen, const char* mimeType)
{
	int retCode = 0;
	Efs_Error err;
	const char* ctxType;
	char ctxLength[64];
	Efs_Header* headers = NULL;
	CURL* curl = (CURL*)self->curl;

	// Bind the NIC for sending packets.
	if (self->boundNic != NULL) {
		retCode = curl_easy_setopt(curl, CURLOPT_INTERFACE, self->boundNic);
		if (retCode == CURLE_INTERFACE_FAILED) {
			err.code = 9994;
			err.message = "Can not bind the given NIC";
			return err;
		}
	}

	curl_easy_setopt(curl, CURLOPT_POST, 1);

	if (mimeType == NULL) {
		ctxType = "Content-Type: application/octet-stream";
	} else {
		ctxType = Efs_String_Concat2("Content-Type: ", mimeType);
	}

	Efs_snprintf(ctxLength, 64, "Content-Length: %lld", bodyLen);
	headers = curl_slist_append(NULL, ctxLength);
	headers = curl_slist_append(headers, ctxType);
	headers = curl_slist_append(headers, "Expect:");

	if (self->auth.itbl != NULL) {
		if (body == NULL) {
			err = self->auth.itbl->Auth(self->auth.self, &headers, url, NULL, 0);
		} else {
			err = self->auth.itbl->Auth(self->auth.self, &headers, url, body, (size_t)bodyLen);
		}

		if (err.code != 200) {
			return err;
		}
	}

	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

	err = Efs_callex(curl, &self->b, &self->root, Efs_False, &self->respHeader);

	curl_slist_free_all(headers);
	if (mimeType != NULL) {
		free((void*)ctxType);
	}

	*ret = self->root;
	return err;
}

Efs_Error Efs_Client_CallWithBinary(
	Efs_Client* self, Efs_Json** ret, const char* url,
	Efs_Reader body, Efs_Int64 bodyLen, const char* mimeType)
{
	CURL* curl = Efs_Client_initcall(self, url);

	curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, bodyLen);
	curl_easy_setopt(curl, CURLOPT_READFUNCTION, body.Read);
	curl_easy_setopt(curl, CURLOPT_READDATA, body.self);

	return Efs_Client_callWithBody(self, ret, url, NULL, bodyLen, mimeType);
}

Efs_Error Efs_Client_CallWithBuffer(
	Efs_Client* self, Efs_Json** ret, const char* url,
	const char* body, size_t bodyLen, const char* mimeType)
{
	CURL* curl = Efs_Client_initcall(self, url);

	curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, bodyLen);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);

	return Efs_Client_callWithBody(self, ret, url, body, bodyLen, mimeType);
}

Efs_Error Efs_Client_CallWithBuffer2(
	Efs_Client* self, Efs_Json** ret, const char* url,
	const char* body, size_t bodyLen, const char* mimeType)
{
	CURL* curl = Efs_Client_initcall(self, url);

	curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, bodyLen);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);

	return Efs_Client_callWithBody(self, ret, url, NULL, bodyLen, mimeType);
}

Efs_Error Efs_Client_Call(Efs_Client* self, Efs_Json** ret, const char* url)
{
	Efs_Error err;
	Efs_Header* headers = NULL;
	CURL* curl = Efs_Client_initcall(self, url);

	if (self->auth.itbl != NULL) {
		err = self->auth.itbl->Auth(self->auth.self, &headers, url, NULL, 0);
		if (err.code != 200) {
			return err;
		}
	}

	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);


	err = Efs_callex(curl, &self->b, &self->root, Efs_False, &self->respHeader);
	/*
	 * Bug No.(4601) Wang Xiaotao 2013\10\12 17:09:02
	 * Change for : free  var headers 'variable'
	 * Reason     : memory leak!
	 */
    curl_slist_free_all(headers);
	*ret = self->root;
	return err;
}

Efs_Error Efs_Client_CallNoRet(Efs_Client* self, const char* url)
{
	Efs_Error err;
	Efs_Header* headers = NULL;
	CURL* curl = Efs_Client_initcall(self, url);

	if (self->auth.itbl != NULL) {
		err = self->auth.itbl->Auth(self->auth.self, &headers, url, NULL, 0);
		if (err.code != 200) {
			return err;
		}
	}

	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

	/*
	 * Bug No.(4601) Wang Xiaotao 2013\10\12 17:09:02
	 * Change for : free  var headers 'variable'
	 * Reason     : memory leak!
	 */
	err = Efs_callex(curl, &self->b, &self->root, Efs_False, &self->respHeader);
	curl_slist_free_all(headers);
	return err;
}

