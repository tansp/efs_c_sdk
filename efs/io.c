/*
 ============================================================================
 Name        : io.c
 Author      : gosun.com
 Copyright   : 2017(c) beijing efs Information Technologies Co., Ltd.
 Description :
 ============================================================================
 */

#include "io.h"
#include "reader.h"
#include <curl/curl.h>

/*============================================================================*/
/* func Efs_Io_form */

typedef struct _Efs_Io_form {
	struct curl_httppost* formpost;
	struct curl_httppost* lastptr;
} Efs_Io_form;

static Efs_Io_PutExtra efs_defaultExtra = { NULL, NULL, 0, 0, NULL };

static void Efs_Io_form_init(
	Efs_Io_form* self, const char* uptoken, const char* key, Efs_Io_PutExtra** extra)
{
	Efs_Io_PutExtraParam* param;
	struct curl_httppost* formpost = NULL;
	struct curl_httppost* lastptr = NULL;

	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "token", CURLFORM_COPYCONTENTS, uptoken, CURLFORM_END);

	if (*extra == NULL) {
		*extra = &efs_defaultExtra;
	}
	if (key != NULL) {
		curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "key", CURLFORM_COPYCONTENTS, key, CURLFORM_END);
	}
	for (param = (*extra)->params; param != NULL; param = param->next) {
		curl_formadd(
			&formpost, &lastptr, CURLFORM_COPYNAME, param->key, CURLFORM_COPYCONTENTS, param->value, CURLFORM_END);
	}

	self->formpost = formpost;
	self->lastptr = lastptr;
}

/*============================================================================*/
/* func Efs_Io_PutXXX */

CURL* Efs_Client_reset(Efs_Client* self);
Efs_Error Efs_callex(CURL* curl, Efs_Buffer *resp, Efs_Json** ret, Efs_Bool simpleError, Efs_Buffer *resph);

static Efs_Error Efs_Io_call(
	Efs_Client* self, Efs_Io_PutRet* ret, struct curl_httppost* formpost,
	Efs_Io_PutExtra* extra)
{
	int retCode = 0;
	Efs_Error err;
	struct curl_slist* headers = NULL;
	const char * upHost = NULL;
	Efs_Rgn_HostVote upHostVote;

	CURL* curl = Efs_Client_reset(self);

	// Bind the NIC for sending packets.
	if (self->boundNic != NULL) {
		retCode = curl_easy_setopt(curl, CURLOPT_INTERFACE, self->boundNic);
		if (retCode == CURLE_INTERFACE_FAILED) {
			err.code = 9994;
			err.message = "Can not bind the given NIC";
			return err;
		}
	}

    // Specify the low speed limit and time
    if (self->lowSpeedLimit > 0 && self->lowSpeedTime > 0) {
		retCode = curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, self->lowSpeedLimit);
		if (retCode == CURLE_INTERFACE_FAILED) {
			err.code = 9994;
			err.message = "Can not specify the low speed limit";
			return err;
		}
		retCode = curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, self->lowSpeedTime);
		if (retCode == CURLE_INTERFACE_FAILED) {
			err.code = 9994;
			err.message = "Can not specify the low speed time";
			return err;
		}
    }

	headers = curl_slist_append(NULL, "Expect:");

	//// For using multi-region storage.
	{
		if ((upHost = extra->upHost) == NULL) {
			if (Efs_Rgn_IsEnabled()) {
				if (extra->upBucket && extra->accessKey) {
					err = Efs_Rgn_Table_GetHost(self->regionTable, self, extra->upBucket, extra->accessKey, extra->upHostFlags, &upHost, &upHostVote);
				} else {
					err = Efs_Rgn_Table_GetHostByUptoken(self->regionTable, self, extra->uptoken, extra->upHostFlags, &upHost, &upHostVote);
				} // if
				if (err.code != 200) {
					return err;
				} // if
			} else {
				upHost = EFS_UP_HOST;
			} // if

			if (upHost == NULL) {
				err.code = 9988;
				err.message = "No proper upload host name";
				return err;
			} // if
		} // if
	} 

	curl_easy_setopt(curl, CURLOPT_URL, upHost);
	curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

	//// For aborting uploading file.
	if (extra->upAbortCallback) {
		curl_easy_setopt(curl, CURLOPT_READFUNCTION, Efs_Rd_Reader_Callback);
	} // if

	err = Efs_callex(curl, &self->b, &self->root, Efs_False, &self->respHeader);
	if (err.code == 200 && ret != NULL) {
		if (extra->callbackRetParser != NULL) {
			err = (*extra->callbackRetParser)(extra->callbackRet, self->root);
		} else {
			ret->hash = Efs_Json_GetString(self->root, "hash", NULL);
			ret->key = Efs_Json_GetString(self->root, "key", NULL);
			ret->persistentId = Efs_Json_GetString(self->root, "persistentId", NULL);
		} 
	}

	//// For using multi-region storage.
	{
		if (Efs_Rgn_IsEnabled()) {
			Efs_Rgn_Table_VoteHost(self->regionTable, &upHostVote, err);
		} // if
	}

	curl_formfree(formpost);
	/*
	 * Bug No.(4718) Wang Xiaotao 2013\10\17 17:46:07
	 * Change for : free  variable 'headers'
	 * Reason     : memory leak!
	 */
	curl_slist_free_all(headers);
	return err;
}

Efs_Error Efs_Io_PutFile(
	Efs_Client* self, Efs_Io_PutRet* ret,
	const char* uptoken, const char* key, const char* localFile, Efs_Io_PutExtra* extra)
{
	Efs_Error err;
	Efs_FileInfo fi;
	Efs_Rd_Reader rdr;
	Efs_Io_form form;
	size_t fileSize;
	const char * localFileName;
	Efs_Io_form_init(&form, uptoken, key, &extra);

	// BugFix : If the filename attribute of the file form-data section is not assigned or holds an empty string,
	//          and the real file size is larger than 10MB, then the Go server will return an error like
	//          "multipart: message too large".
	//          Assign an arbitary non-empty string to this attribute will force the Go server to write all the data
	//          into a temporary file and then every thing goes right.
	localFileName = (extra->localFileName) ? extra->localFileName : "EFS-C-SDK-UP-FILE";

	//// For aborting uploading file.
	if (extra->upAbortCallback) {
		Efs_Zero(rdr);

		rdr.abortCallback = extra->upAbortCallback;
		rdr.abortUserData = extra->upAbortUserData;

		err = Efs_Rd_Reader_Open(&rdr, localFile);
		if (err.code != 200) {
			return err;
		} // if

		if (extra->upFileSize == 0) {
			Efs_Zero(fi);
			err = Efs_File_Stat(rdr.file, &fi);
			if (err.code != 200) {
				return err;
			} // if

			fileSize = fi.st_size;
		} else {
			fileSize = extra->upFileSize;
		} // if

		curl_formadd(&form.formpost, &form.lastptr, CURLFORM_COPYNAME, "file", CURLFORM_STREAM, &rdr, CURLFORM_CONTENTSLENGTH, (long)fileSize, CURLFORM_FILENAME, localFileName, CURLFORM_END);
	} else {
	    curl_formadd(&form.formpost, &form.lastptr, CURLFORM_COPYNAME, "file", CURLFORM_FILE, localFile, CURLFORM_FILENAME, localFileName, CURLFORM_END);
	} // if

	//// For using multi-region storage.
	{
		if (Efs_Rgn_IsEnabled()) {
			if (!extra->uptoken) {
				extra->uptoken = uptoken;
			} // if
		} // if
	}

	err = Efs_Io_call(self, ret, form.formpost, extra);
	
	//// For aborting uploading file.
	if (extra->upAbortCallback) {
		Efs_Rd_Reader_Close(&rdr);
		if (err.code == CURLE_ABORTED_BY_CALLBACK) {
			if (rdr.status == EFS_RD_ABORT_BY_CALLBACK) {
				err.code = 9987;
				err.message = "Upload progress has been aborted by caller";
			} else if (rdr.status == EFS_RD_ABORT_BY_READAT) {
				err.code = 9986;
				err.message = "Upload progress has been aborted by Efs_File_ReadAt()";
			} // if
		} // if
	} // if

	return err;
}

Efs_Error Efs_Io_PutBuffer(
	Efs_Client* self, Efs_Io_PutRet* ret,
	const char* uptoken, const char* key, const char* buf, size_t fsize, Efs_Io_PutExtra* extra)
{
	Efs_Io_form form;
	Efs_Io_form_init(&form, uptoken, key, &extra);

    if (key == NULL) {
        // Use an empty string instead of the NULL pointer to prevent the curl lib from crashing
        // when read it.
        // **NOTICE**: The magic variable $(filename) will be set as empty string.
        key = "";
    }

	curl_formadd(
		&form.formpost, &form.lastptr, CURLFORM_COPYNAME, "file",
		CURLFORM_BUFFER, key, CURLFORM_BUFFERPTR, buf, CURLFORM_BUFFERLENGTH, fsize, CURLFORM_END);

	//// For using multi-region storage.
	{
		if (Efs_Rgn_IsEnabled()) {
			if (!extra->uptoken) {
				extra->uptoken = uptoken;
			} // if
		} // if
	}

	return Efs_Io_call(self, ret, form.formpost, extra);
}

// This function  will be called by 'Efs_Io_PutStream'
// In this function, readFunc(read-stream-data) will be set
static Efs_Error Efs_Io_call_with_callback(
        Efs_Client* self, Efs_Io_PutRet* ret, 
		struct curl_httppost* formpost,
        rdFunc rdr,
        Efs_Io_PutExtra* extra)
{
    int retCode = 0;
    Efs_Error err;
    struct curl_slist* headers = NULL;

    CURL* curl = Efs_Client_reset(self);

    // Bind the NIC for sending packets.
    if (self->boundNic != NULL) {
        retCode = curl_easy_setopt(curl, CURLOPT_INTERFACE, self->boundNic);
        if (retCode == CURLE_INTERFACE_FAILED) {
            err.code = 9994;
            err.message = "Can not bind the given NIC";
            return err;
        }
    }

    headers = curl_slist_append(NULL, "Expect:");

    curl_easy_setopt(curl, CURLOPT_URL, EFS_UP_HOST);
    curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, rdr);

    err = Efs_callex(curl, &self->b, &self->root, Efs_False, &self->respHeader);
    if (err.code == 200 && ret != NULL) {
        if (extra->callbackRetParser != NULL) {
            err = (*extra->callbackRetParser)(extra->callbackRet, self->root);
        } else {
            ret->hash = Efs_Json_GetString(self->root, "hash", NULL);
            ret->key = Efs_Json_GetString(self->root, "key", NULL);
        }
    }

    curl_formfree(formpost);
    curl_slist_free_all(headers);
    return err;
}

Efs_Error Efs_Io_PutStream(
    Efs_Client* self, Efs_Io_PutRet* ret,
    const char* uptoken, const char* key, 
	void* ctx, size_t fsize, rdFunc rdr, 
	Efs_Io_PutExtra* extra)
{
    Efs_Io_form form;
    Efs_Io_form_init(&form, uptoken, key, &extra);

    if (key == NULL) {
        // Use an empty string instead of the NULL pointer to prevent the curl lib from crashing
        // when read it.
        // **NOTICE**: The magic variable $(filename) will be set as empty string.
        key = "";
    }

	// Add 'filename' property to make it like a file upload one
	// Otherwise it may report: CURL_ERROR(18) or "multipart/message too large"
	// See https://curl.haxx.se/libcurl/c/curl_formadd.html#CURLFORMSTREAM
	// FIXED by fengyh 2017-03-22 10:30
    curl_formadd(
                &form.formpost, &form.lastptr,
                CURLFORM_COPYNAME, "file",
				CURLFORM_FILENAME, "filename",
                CURLFORM_STREAM, ctx,
                CURLFORM_CONTENTSLENGTH, fsize,
                CURLFORM_END);

    return Efs_Io_call_with_callback(self, ret, form.formpost, rdr, extra);
}
