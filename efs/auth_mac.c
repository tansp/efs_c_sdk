/*
 ============================================================================
 Name        : mac_auth.c
 Author      : gosun.com
 Copyright   : 2017(c) beijing efs Information Technologies Co., Ltd.
 Description : 
 ============================================================================
 */

#include "http.h"
#include <curl/curl.h>
#include <openssl/hmac.h>
#include <openssl/engine.h>

#if defined(_WIN32)
#pragma comment(lib, "libeay32.lib")
#endif

/*============================================================================*/
/* Global */

void Efs_MacAuth_Init()
{
	ENGINE_load_builtin_engines();
	ENGINE_register_all_complete();
}

void Efs_MacAuth_Cleanup()
{
}

void Efs_Servend_Init(long flags)
{
	Efs_Global_Init(flags);
	Efs_MacAuth_Init();
}

void Efs_Servend_Cleanup()
{
	Efs_Global_Cleanup();
}

/*============================================================================*/
/* type Efs_Mac */

static Efs_Error Efs_Mac_Auth(
	void* self, Efs_Header** header, const char* url, const char* addition, size_t addlen)
{
	Efs_Error err;
	char* auth;
	char* enc_digest;
	char digest[EVP_MAX_MD_SIZE + 1];
	unsigned int dgtlen = sizeof(digest);
	HMAC_CTX ctx;
	Efs_Mac mac;

	char const* path = strstr(url, "://");
	if (path != NULL) {
		path = strchr(path + 3, '/');
	}
	if (path == NULL) {
		err.code = 400;
		err.message = "invalid url";
		return err;
	}

	if (self) {
		mac = *(Efs_Mac*)self;
	} else {
		mac.accessKey = EFS_ACCESS_KEY;
		mac.secretKey = EFS_SECRET_KEY;
	}

	HMAC_CTX_init(&ctx);
	HMAC_Init_ex(&ctx, mac.secretKey, strlen(mac.secretKey), EVP_sha1(), NULL);
	HMAC_Update(&ctx, path, strlen(path));
	HMAC_Update(&ctx, "\n", 1);

	if (addlen > 0) {
		HMAC_Update(&ctx, addition, addlen);
	} 

	HMAC_Final(&ctx, digest, &dgtlen);
	HMAC_CTX_cleanup(&ctx);

	enc_digest = Efs_Memory_Encode(digest, dgtlen);

	auth = Efs_String_Concat("Authorization: QBox ", mac.accessKey, ":", enc_digest, NULL);
	Efs_Free(enc_digest);

	*header = curl_slist_append(*header, auth);
	Efs_Free(auth);

	return Efs_OK;
}

static void Efs_Mac_Release(void* self)
{
	if (self) {
		free(self);
	}
}

static Efs_Mac* Efs_Mac_Clone(Efs_Mac* mac)
{
	Efs_Mac* p;
	char* accessKey;
	size_t n1, n2;
	if (mac) {
		n1 = strlen(mac->accessKey) + 1;
		n2 = strlen(mac->secretKey) + 1;
		p = (Efs_Mac*)malloc(sizeof(Efs_Mac) + n1 + n2);
		accessKey = (char*)(p + 1);
		memcpy(accessKey, mac->accessKey, n1);
		memcpy(accessKey + n1, mac->secretKey, n2);
		p->accessKey = accessKey;
		p->secretKey = accessKey + n1;
		return p;
	}
	return NULL;
}

static Efs_Auth_Itbl Efs_MacAuth_Itbl = {
	Efs_Mac_Auth,
	Efs_Mac_Release
};

Efs_Auth Efs_MacAuth(Efs_Mac* mac)
{
	Efs_Auth auth = {Efs_Mac_Clone(mac), &Efs_MacAuth_Itbl};
	return auth;
};

void Efs_Client_InitMacAuth(Efs_Client* self, size_t bufSize, Efs_Mac* mac)
{
	Efs_Auth auth = {Efs_Mac_Clone(mac), &Efs_MacAuth_Itbl};
	Efs_Client_InitEx(self, auth, bufSize);
}

/*============================================================================*/
/* func Efs_Mac_Sign*/

char* Efs_Mac_Sign(Efs_Mac* self, char* data)
{
	char* sign;
	char* encoded_digest;
	char digest[EVP_MAX_MD_SIZE + 1];
	unsigned int dgtlen = sizeof(digest);
	HMAC_CTX ctx;
	Efs_Mac mac;

	if (self) {
		mac = *self;
	} else {
		mac.accessKey = EFS_ACCESS_KEY;
		mac.secretKey = EFS_SECRET_KEY;
	}

	HMAC_CTX_init(&ctx);
	HMAC_Init_ex(&ctx, mac.secretKey, strlen(mac.secretKey), EVP_sha1(), NULL);
	HMAC_Update(&ctx, data, strlen(data));
	HMAC_Final(&ctx, digest, &dgtlen);
	HMAC_CTX_cleanup(&ctx);

	encoded_digest = Efs_Memory_Encode(digest, dgtlen);
	sign = Efs_String_Concat3(mac.accessKey, ":", encoded_digest);
	Efs_Free(encoded_digest);

	return sign;
}

/*============================================================================*/
/* func Efs_Mac_SignToken */

char* Efs_Mac_SignToken(Efs_Mac* self, char* policy_str)
{
	char* data;
	char* sign;
	char* token;

	data = Efs_String_Encode(policy_str);
	sign = Efs_Mac_Sign(self, data);
	token = Efs_String_Concat3(sign, ":", data);

	Efs_Free(sign);
	Efs_Free(data);

	return token;
}

/*============================================================================*/

