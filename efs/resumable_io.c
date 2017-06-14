/*
 ============================================================================
 Name        : resumable_io.c
 Author      : gosun.com
 Copyright   : 2017(c) beijing efs Information Technologies Co., Ltd.
 Description :
 ============================================================================
 */

#include "region.h"
#include "resumable_io.h"
#include <curl/curl.h>
#include <sys/stat.h>

#define	blockBits			22
#define blockMask			((1 << blockBits) - 1)

#define defaultTryTimes		3
#define defaultWorkers		4
#define defaultChunkSize	(256 * 1024) // 256k

/*============================================================================*/
/* type Efs_Rio_ST - SingleThread */

#if defined(_WIN32)

#include <windows.h>

typedef struct _Efs_Rio_MTWG_Data
{
    Efs_Count addedCount;
    Efs_Count doneCount;
    HANDLE event;
} Efs_Rio_MTWG_Data;

static void Efs_Rio_MTWG_Add(void* self, int n)
{
    Efs_Count_Inc(&((Efs_Rio_MTWG_Data*)self)->addedCount);
} // Efs_Rio_MTWG_Add

static void Efs_Rio_MTWG_Done(void* self)
{
    Efs_Count_Inc(&((Efs_Rio_MTWG_Data*)self)->doneCount);
    SetEvent(((Efs_Rio_MTWG_Data*)self)->event);
} // Efs_Rio_MTWG_Done

static void Efs_Rio_MTWG_Wait(void* self)
{
    Efs_Rio_MTWG_Data * data = (Efs_Rio_MTWG_Data*)self;
    Efs_Count lastDoneCount = data->doneCount;
    DWORD ret = 0;

    while (lastDoneCount != data->addedCount) {
        ret = WaitForSingleObject(((Efs_Rio_MTWG_Data*)self)->event, INFINITE);
        if (ret == WAIT_OBJECT_0) {
            lastDoneCount = data->doneCount;
        }
    } // while
} // Efs_Rio_MTWG_Wait

static void Efs_Rio_MTWG_Release(void* self)
{
    CloseHandle(((Efs_Rio_MTWG_Data*)self)->event);
    free(self);
} // Efs_Rio_MTWG_Release

static Efs_Rio_WaitGroup_Itbl Efs_Rio_MTWG_Itbl = {
    &Efs_Rio_MTWG_Add,
    &Efs_Rio_MTWG_Done,
    &Efs_Rio_MTWG_Wait,
    &Efs_Rio_MTWG_Release,
};

Efs_Rio_WaitGroup Efs_Rio_MTWG_Create(void)
{
    Efs_Rio_WaitGroup wg;
    Efs_Rio_MTWG_Data * newData = NULL;

    newData = (Efs_Rio_MTWG_Data*)malloc(sizeof(*newData));
    newData->addedCount = 0;
    newData->doneCount = 0;
    newData->event = CreateEvent(NULL, FALSE, FALSE, NULL);

    wg.itbl = &Efs_Rio_MTWG_Itbl;
    wg.self = newData;
    return wg;
} // Efs_Rio_MTWG_Create

#endif

static void Efs_Rio_STWG_Add(void* self, int n) {}
static void Efs_Rio_STWG_Done(void* self)	{}
static void Efs_Rio_STWG_Wait(void* self)	{}
static void Efs_Rio_STWG_Release(void* self) {}

static Efs_Rio_WaitGroup_Itbl Efs_Rio_STWG_Itbl = {
	Efs_Rio_STWG_Add,
	Efs_Rio_STWG_Done,
	Efs_Rio_STWG_Wait,
	Efs_Rio_STWG_Release
};

static Efs_Rio_WaitGroup Efs_Rio_STWG = {
	NULL, &Efs_Rio_STWG_Itbl
};

static Efs_Rio_WaitGroup Efs_Rio_ST_WaitGroup(void* self) {
	return Efs_Rio_STWG;
}

static Efs_Client* Efs_Rio_ST_ClientTls(void* self, Efs_Client* mc) {
	return mc;
}

static int Efs_Rio_ST_RunTask(void* self, void (*task)(void* params), void* params) {
	task(params);
    return EFS_RIO_NOTIFY_OK;
}

static Efs_Rio_ThreadModel_Itbl Efs_Rio_ST_Itbl = {
	Efs_Rio_ST_WaitGroup,
	Efs_Rio_ST_ClientTls,
	Efs_Rio_ST_RunTask
};

Efs_Rio_ThreadModel Efs_Rio_ST = {
	NULL, &Efs_Rio_ST_Itbl
};

/*============================================================================*/
/* type Efs_Rio_Settings */

static Efs_Rio_Settings settings = {
	defaultWorkers * 4,
	defaultWorkers,
	defaultChunkSize,
	defaultTryTimes,
	{NULL, &Efs_Rio_ST_Itbl}
};

void Efs_Rio_SetSettings(Efs_Rio_Settings* v)
{
	settings = *v;
	if (settings.workers == 0) {
		settings.workers = defaultWorkers;
	}
	if (settings.taskQsize == 0) {
		settings.taskQsize = settings.workers * 4;
	}
	if (settings.chunkSize == 0) {
		settings.chunkSize = defaultChunkSize;
	}
	if (settings.tryTimes == 0) {
		settings.tryTimes = defaultTryTimes;
	}
	if (settings.threadModel.itbl == NULL) {
		settings.threadModel = Efs_Rio_ST;
	}
}

/*============================================================================*/
/* func Efs_UptokenAuth */

static Efs_Error Efs_UptokenAuth_Auth(
	void* self, Efs_Header** header, const char* url, const char* addition, size_t addlen)
{
	Efs_Error err;

	*header = curl_slist_append(*header, self);

	err.code    = 200;
	err.message = "OK";
	return err;
}

static void Efs_UptokenAuth_Release(void* self)
{
	free(self);
}

static Efs_Auth_Itbl Efs_UptokenAuth_Itbl = {
	Efs_UptokenAuth_Auth,
	Efs_UptokenAuth_Release
};

static Efs_Auth Efs_UptokenAuth(const char* uptoken)
{
	char* self = Efs_String_Concat2("Authorization: UpToken ", uptoken);
	Efs_Auth auth = {self, &Efs_UptokenAuth_Itbl};
	return auth;
}

/*============================================================================*/
/* type Efs_Rio_BlkputRet */

static void Efs_Rio_BlkputRet_Cleanup(Efs_Rio_BlkputRet* self)
{
	if (self->ctx != NULL) {
		free((void*)self->ctx);
		memset(self, 0, sizeof(*self));
	}
}

static void Efs_Rio_BlkputRet_Assign(Efs_Rio_BlkputRet* self, Efs_Rio_BlkputRet* ret)
{
	char* p;
	size_t n1 = 0, n2 = 0, n3 = 0;

	Efs_Rio_BlkputRet_Cleanup(self);

	*self = *ret;
	if (ret->ctx == NULL) {
		return;
	}

	n1 = strlen(ret->ctx) + 1;
	n3 = strlen(ret->host) + 1;
	if (ret->checksum) {
		n2 = strlen(ret->checksum) + 1;
	}

	p = (char*)malloc(n1 + n2 + n3);

	memcpy(p, ret->ctx, n1);
	self->ctx = p;

	memcpy(p+n1, ret->host, n3);
	self->host = p+n1;

	if (n2) {
		memcpy(p+n1+n3, ret->checksum, n2);
		self->checksum = p+n1+n3;
	}
}

/*============================================================================*/
/* type Efs_Rio_PutExtra */

static int notifyNil(void* self, int blkIdx, int blkSize, Efs_Rio_BlkputRet* ret) { return EFS_RIO_NOTIFY_OK; }
static int notifyErrNil(void* self, int blkIdx, int blkSize, Efs_Error err) { return EFS_RIO_NOTIFY_OK; }

static Efs_Error ErrInvalidPutProgress = {
	Efs_Rio_InvalidPutProgress, "invalid put progress"
};

static Efs_Error Efs_Rio_PutExtra_Init(
	Efs_Rio_PutExtra* self, Efs_Int64 fsize, Efs_Rio_PutExtra* extra)
{
	size_t cbprog;
	int i, blockCnt = Efs_Rio_BlockCount(fsize);
	int fprog = (extra != NULL) && (extra->progresses != NULL);

	if (fprog && extra->blockCnt != (size_t)blockCnt) {
		return ErrInvalidPutProgress;
	}

	if (extra) {
		*self = *extra;
	} else {
		memset(self, 0, sizeof(Efs_Rio_PutExtra));
	}

	cbprog = sizeof(Efs_Rio_BlkputRet) * blockCnt;
	self->progresses = (Efs_Rio_BlkputRet*)malloc(cbprog);
	self->blockCnt = blockCnt;
	memset(self->progresses, 0, cbprog);
	if (fprog) {
		for (i = 0; i < blockCnt; i++) {
			Efs_Rio_BlkputRet_Assign(&self->progresses[i], &extra->progresses[i]);
		}
	}

	if (self->chunkSize == 0) {
		self->chunkSize = settings.chunkSize;
	}
	if (self->tryTimes == 0) {
		self->tryTimes = settings.tryTimes;
	}
	if (self->notify == NULL) {
		self->notify = notifyNil;
	}
	if (self->notifyErr == NULL) {
		self->notifyErr = notifyErrNil;
	}
	if (self->threadModel.itbl == NULL) {
		self->threadModel = settings.threadModel;
	}

	return Efs_OK;
}

static void Efs_Rio_PutExtra_Cleanup(Efs_Rio_PutExtra* self)
{
	size_t i;
	for (i = 0; i < self->blockCnt; i++) {
		Efs_Rio_BlkputRet_Cleanup(&self->progresses[i]);
	}
	free(self->progresses);
	self->progresses = NULL;
	self->blockCnt = 0;
}

static Efs_Int64 Efs_Rio_PutExtra_ChunkSize(Efs_Rio_PutExtra* self)
{
	if (self) {
		return self->chunkSize;
	}
	return settings.chunkSize;
}

static void Efs_Io_PutExtra_initFrom(Efs_Io_PutExtra* self, Efs_Rio_PutExtra* extra)
{
	if (extra) {
		self->mimeType = extra->mimeType;
		self->localFileName = extra->localFileName;
	} else {
		memset(self, 0, sizeof(*self));
	}
}

/*============================================================================*/

static Efs_Error Efs_Rio_bput(
	Efs_Client* self, Efs_Rio_BlkputRet* ret, Efs_Reader body, int bodyLength, const char* url)
{
	Efs_Rio_BlkputRet retFromResp;
	Efs_Json* root;

	Efs_Error err = Efs_Client_CallWithBinary(self, &root, url, body, bodyLength, NULL);
	if (err.code == 200) {
		retFromResp.ctx = Efs_Json_GetString(root, "ctx", NULL);
		retFromResp.checksum = Efs_Json_GetString(root, "checksum", NULL);
		retFromResp.host = Efs_Json_GetString(root, "host", NULL);
		retFromResp.crc32 = (Efs_Uint32)Efs_Json_GetInt64(root, "crc32", 0);
		retFromResp.offset = (Efs_Uint32)Efs_Json_GetInt64(root, "offset", 0);

		if (retFromResp.ctx == NULL || retFromResp.host == NULL || retFromResp.offset == 0) {
			err.code = 9998;
			err.message = "unexcepted response: invalid ctx, host or offset";
			return err;
		}

		Efs_Rio_BlkputRet_Assign(ret, &retFromResp);
	}

	return err;
}

static Efs_Error Efs_Rio_Mkblock(
	Efs_Client* self, Efs_Rio_BlkputRet* ret, int blkSize, Efs_Reader body, int bodyLength, Efs_Rio_PutExtra* extra)
{
	Efs_Error err;
	Efs_Rgn_HostVote upHostVote;
	const char * upHost = NULL;
	char* url = NULL;

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

	url = Efs_String_Format(128, "%s/mkblk/%d", upHost, blkSize);
	err = Efs_Rio_bput(self, ret, body, bodyLength, url);
	Efs_Free(url);

	//// For using multi-region storage.
	{
		if (Efs_Rgn_IsEnabled()) {
			Efs_Rgn_Table_VoteHost(self->regionTable, &upHostVote, err);
		} // if
	}

	return err;
}

static Efs_Error Efs_Rio_Blockput(
	Efs_Client* self, Efs_Rio_BlkputRet* ret, Efs_Reader body, int bodyLength)
{
	char* url = Efs_String_Format(1024, "%s/bput/%s/%d", ret->host, ret->ctx, (int)ret->offset);
	Efs_Error err = Efs_Rio_bput(self, ret, body, bodyLength, url);
	Efs_Free(url);
	return err;
}

/*============================================================================*/

static Efs_Error ErrUnmatchedChecksum = {
	Efs_Rio_UnmatchedChecksum, "unmatched checksum"
};

static int Efs_TemporaryError(int code) {
	return code != 401;
}

static Efs_Error Efs_Rio_ResumableBlockput(
	Efs_Client* c, Efs_Rio_BlkputRet* ret, Efs_ReaderAt f, int blkIdx, int blkSize, Efs_Rio_PutExtra* extra)
{
	Efs_Error err = {200, NULL};
	Efs_Tee tee;
	Efs_Section section;
	Efs_Reader body, body1;

	Efs_Crc32 crc32;
	Efs_Writer h = Efs_Crc32Writer(&crc32, 0);
	Efs_Int64 offbase = (Efs_Int64)(blkIdx) << blockBits;

	int chunkSize = extra->chunkSize;
	int bodyLength;
	int tryTimes;
    int notifyRet = 0;

	if (ret->ctx == NULL) {

		if (chunkSize < blkSize) {
			bodyLength = chunkSize;
		} else {
			bodyLength = blkSize;
		}

		body1 = Efs_SectionReader(&section, f, (Efs_Off_T)offbase, bodyLength);
		body = Efs_TeeReader(&tee, body1, h);

		err = Efs_Rio_Mkblock(c, ret, blkSize, body, bodyLength, extra);
		if (err.code != 200) {
			return err;
		}
		if (ret->crc32 != crc32.val || (int)(ret->offset) != bodyLength) {
			return ErrUnmatchedChecksum;
		}
		notifyRet = extra->notify(extra->notifyRecvr, blkIdx, blkSize, ret);
        if (notifyRet == EFS_RIO_NOTIFY_EXIT) {
            // Terminate the upload process if  the caller requests
            err.code = Efs_Rio_PutInterrupted;
            err.message = "Interrupted by the caller";
            return err;
        }
	}

	while ((int)(ret->offset) < blkSize) {

		if (chunkSize < blkSize - (int)(ret->offset)) {
			bodyLength = chunkSize;
		} else {
			bodyLength = blkSize - (int)(ret->offset);
		}

		tryTimes = extra->tryTimes;

lzRetry:
		crc32.val = 0;
		body1 = Efs_SectionReader(&section, f, (Efs_Off_T)offbase + (ret->offset), bodyLength);
		body = Efs_TeeReader(&tee, body1, h);

		err = Efs_Rio_Blockput(c, ret, body, bodyLength);
		if (err.code == 200) {
			if (ret->crc32 == crc32.val) {
				notifyRet = extra->notify(extra->notifyRecvr, blkIdx, blkSize, ret);
                if (notifyRet == EFS_RIO_NOTIFY_EXIT) {
                    // Terminate the upload process if the caller requests
                    err.code = Efs_Rio_PutInterrupted;
                    err.message = "Interrupted by the caller";
                    return err;
                }

				continue;
			}
			Efs_Log_Warn("ResumableBlockput: invalid checksum, retry");
			err = ErrUnmatchedChecksum;
		} else {
			if (err.code == Efs_Rio_InvalidCtx) {
				Efs_Rio_BlkputRet_Cleanup(ret); // reset
				Efs_Log_Warn("ResumableBlockput: invalid ctx, please retry");
				return err;
			}
			Efs_Log_Warn("ResumableBlockput %d off:%d failed - %E", blkIdx, (int)ret->offset, err);
		}
		if (tryTimes > 1 && Efs_TemporaryError(err.code)) {
			tryTimes--;
			Efs_Log_Info("ResumableBlockput %E, retrying ...", err);
			goto lzRetry;
		}
		break;
	}
	return err;
}

/*============================================================================*/

static Efs_Error Efs_Rio_Mkfile(
	Efs_Client* c, Efs_Rio_PutRet* ret, const char* key, Efs_Int64 fsize, Efs_Rio_PutExtra* extra)
{
	size_t i, blkCount = extra->blockCnt;
	Efs_Json* root;
	Efs_Error err;
	Efs_Rio_BlkputRet* prog;
	Efs_Buffer url, body;

	Efs_Buffer_Init(&url, 1024);
	Efs_Buffer_AppendFormat(&url, "%s/mkfile/%D", extra->upHost, fsize);

	if (NULL != key) {
		Efs_Buffer_AppendFormat(&url, "/key/%S", key);
	}

	if (extra->mimeType != NULL) {
		Efs_Buffer_AppendFormat(&url, "/mimeType/%S", extra->mimeType);
	}
	
	// if (extra->customMeta != NULL) {
	// 	Efs_Buffer_AppendFormat(&url, "/meta/%S", extra->customMeta);
	// }
	// if (extra->callbackParams != NULL) {
	// 	Efs_Buffer_AppendFormat(&url, "/params/%S", extra->callbackParams);
	// }

	Efs_Buffer_Init(&body, 176 * blkCount);
	for (i = 0; i < blkCount; i++) {
		prog = &extra->progresses[i];
		Efs_Buffer_Write(&body, prog->ctx, strlen(prog->ctx));
		Efs_Buffer_PutChar(&body, ',');
	}
	if (blkCount > 0) {
		body.curr--;
	}

	err = Efs_Client_CallWithBuffer(
		c, &root, Efs_Buffer_CStr(&url), body.buf, body.curr - body.buf, "text/plain");

	Efs_Buffer_Cleanup(&url);
	Efs_Buffer_Cleanup(&body);

	if (err.code == 200) {
		if (extra->callbackRetParser != NULL) {
			err = (*extra->callbackRetParser)(extra->callbackRet, root);
		} else {
			ret->hash = Efs_Json_GetString(root, "hash", NULL);
			ret->key = Efs_Json_GetString(root, "key", NULL);
		}
	}
	return err;
}

static Efs_Error Efs_Rio_Mkfile2(
	Efs_Client* c, Efs_Rio_PutRet* ret, const char* key, Efs_Int64 fsize, Efs_Rio_PutExtra* extra)
{
	size_t i, blkCount = extra->blockCnt;
	Efs_Json* root;
	Efs_Error err;
	Efs_Rgn_HostVote upHostVote;
	const char * upHost = NULL;
	Efs_Rio_BlkputRet* prog;
	Efs_Buffer url, body;
	int j = 0;

	Efs_Buffer_Init(&url, 2048);

	//// For using multi-region storage.
	{
		if ((upHost = extra->upHost) == NULL) {
			if (Efs_Rgn_IsEnabled()) {
				if (extra->upBucket && extra->accessKey) {
					err = Efs_Rgn_Table_GetHost(c->regionTable, c, extra->upBucket, extra->accessKey, extra->upHostFlags, &upHost, &upHostVote);
				} else {
					err = Efs_Rgn_Table_GetHostByUptoken(c->regionTable, c, extra->uptoken, extra->upHostFlags, &upHost, &upHostVote);
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

	Efs_Buffer_AppendFormat(&url, "%s/mkfile/%D", upHost, fsize);

	if (key != NULL) {
		// Allow using empty key
		Efs_Buffer_AppendFormat(&url, "/key/%S", key);
	}
	if (extra->xVarsList != NULL && extra->xVarsCount > 0) {
		for (j = 0; j < extra->xVarsCount; j += 1) {
			Efs_Buffer_AppendFormat(&url, "/%s/%S", (extra->xVarsList[j])[0], (extra->xVarsList[j])[1]);
		} // for
	}

	Efs_Buffer_Init(&body, 176 * blkCount);
	for (i = 0; i < blkCount; i++) {
		prog = &extra->progresses[i];
		Efs_Buffer_Write(&body, prog->ctx, strlen(prog->ctx));
		Efs_Buffer_PutChar(&body, ',');
	}
	if (blkCount > 0) {
		body.curr--;
	}

	err = Efs_Client_CallWithBuffer(
		c, &root, Efs_Buffer_CStr(&url), body.buf, body.curr - body.buf, "text/plain");

	Efs_Buffer_Cleanup(&url);
	Efs_Buffer_Cleanup(&body);

	//// For using multi-region storage.
	{
		if (Efs_Rgn_IsEnabled()) {
			Efs_Rgn_Table_VoteHost(c->regionTable, &upHostVote, err);
		} // if
	}

	if (err.code == 200) {
		ret->hash = Efs_Json_GetString(root, "hash", NULL);
		ret->key = Efs_Json_GetString(root, "key", NULL);
	}
	return err;
}

/*============================================================================*/

int Efs_Rio_BlockCount(Efs_Int64 fsize)
{
	return (int)((fsize + blockMask) >> blockBits);
}

/*============================================================================*/
/* type Efs_Rio_task */

typedef struct _Efs_Rio_task {
	Efs_ReaderAt f;
	Efs_Client* mc;
	Efs_Rio_PutExtra* extra;
	Efs_Rio_WaitGroup wg;
	int* nfails;
	Efs_Count* ninterrupts;
	int blkIdx;
	int blkSize1;
} Efs_Rio_task;

static void Efs_Rio_doTask(void* params)
{
	Efs_Error err;
	Efs_Rio_BlkputRet ret;
	Efs_Rio_task* task = (Efs_Rio_task*)params;
	Efs_Rio_WaitGroup wg = task->wg;
	Efs_Rio_PutExtra* extra = task->extra;
	Efs_Rio_ThreadModel tm = extra->threadModel;
	Efs_Client* c = tm.itbl->ClientTls(tm.self, task->mc);
	int blkIdx = task->blkIdx;
	int tryTimes = extra->tryTimes;

	if ((*task->ninterrupts) > 0) {
		free(task);
		Efs_Count_Inc(task->ninterrupts);
		wg.itbl->Done(wg.self);
		return;
	}

	memset(&ret, 0, sizeof(ret));

lzRetry:
	Efs_Rio_BlkputRet_Assign(&ret, &extra->progresses[blkIdx]);
	err = Efs_Rio_ResumableBlockput(c, &ret, task->f, blkIdx, task->blkSize1, extra);
	if (err.code != 200) {
        if (err.code == Efs_Rio_PutInterrupted) {
            // Terminate the upload process if the caller requests
			Efs_Rio_BlkputRet_Cleanup(&ret);
			Efs_Count_Inc(task->ninterrupts);
			free(task);
			wg.itbl->Done(wg.self);
            return;
        }

		if (tryTimes > 1 && Efs_TemporaryError(err.code)) {
			tryTimes--;
			Efs_Log_Info("resumable.Put %E, retrying ...", err);
			goto lzRetry;
		}
		Efs_Log_Warn("resumable.Put %d failed: %E", blkIdx, err);
		extra->notifyErr(extra->notifyRecvr, task->blkIdx, task->blkSize1, err);
		(*task->nfails)++;
	} else {
		Efs_Rio_BlkputRet_Assign(&extra->progresses[blkIdx], &ret);
	}
	Efs_Rio_BlkputRet_Cleanup(&ret);
	free(task);
	wg.itbl->Done(wg.self);
}

/*============================================================================*/
/* func Efs_Rio_PutXXX */

static Efs_Error ErrPutFailed = {
	Efs_Rio_PutFailed, "resumable put failed"
};
static Efs_Error ErrPutInterrupted = {
	Efs_Rio_PutInterrupted, "resumable put interrupted"
};

Efs_Error Efs_Rio_Put(
	Efs_Client* self, Efs_Rio_PutRet* ret,
	const char* uptoken, const char* key, Efs_ReaderAt f, Efs_Int64 fsize, Efs_Rio_PutExtra* extra1)
{
	Efs_Int64 offbase;
	Efs_Rio_task* task;
	Efs_Rio_WaitGroup wg;
	Efs_Rio_PutExtra extra;
	Efs_Rio_ThreadModel tm;
	Efs_Auth auth, auth1 = self->auth;
	int i, last, blkSize;
	int nfails;
    int retCode;
    Efs_Count ninterrupts;
	Efs_Error err = Efs_Rio_PutExtra_Init(&extra, fsize, extra1);
	if (err.code != 200) {
		return err;
	}

	//// For using multi-region storage.
	{
		if (Efs_Rgn_IsEnabled()) {
			if (!extra.uptoken) {
				extra.uptoken = uptoken;
			} // if
		} // if
	}

	tm = extra.threadModel;
	wg = tm.itbl->WaitGroup(tm.self);

	last = extra.blockCnt - 1;
	blkSize = 1 << blockBits;
	nfails = 0;
    ninterrupts = 0;

	self->auth = auth = Efs_UptokenAuth(uptoken);

	for (i = 0; i < (int)extra.blockCnt; i++) {
		task = (Efs_Rio_task*)malloc(sizeof(Efs_Rio_task));
		task->f = f;
		task->extra = &extra;
		task->mc = self;
		task->wg = wg;
		task->nfails = &nfails;
		task->ninterrupts = &ninterrupts;
		task->blkIdx = i;
		task->blkSize1 = blkSize;
		if (i == last) {
			offbase = (Efs_Int64)(i) << blockBits;
			task->blkSize1 = (int)(fsize - offbase);
		}

		wg.itbl->Add(wg.self, 1);
		retCode = tm.itbl->RunTask(tm.self, Efs_Rio_doTask, task);
		if (retCode == EFS_RIO_NOTIFY_EXIT) {
			wg.itbl->Done(wg.self);
			Efs_Count_Inc(&ninterrupts);
            free(task);
		}

		if (ninterrupts > 0) {
			break;
		}
	} // for

	wg.itbl->Wait(wg.self);
	if (nfails != 0) {
		err = ErrPutFailed;
    } else if (ninterrupts != 0) {
		err = ErrPutInterrupted;
	} else {
		err = Efs_Rio_Mkfile2(self, ret, key, fsize, &extra);
	}

	Efs_Rio_PutExtra_Cleanup(&extra);

	wg.itbl->Release(wg.self);
	auth.itbl->Release(auth.self);
	self->auth = auth1;
	return err;
}

Efs_Error Efs_Rio_PutFile(
	Efs_Client* self, Efs_Rio_PutRet* ret,
	const char* uptoken, const char* key, const char* localFile, Efs_Rio_PutExtra* extra)
{
	Efs_Io_PutExtra extra1;
	Efs_Int64 fsize;
	Efs_FileInfo fi;
	Efs_File* f;
	Efs_Error err = Efs_File_Open(&f, localFile);
	if (err.code != 200) {
		return err;
	}
	err = Efs_File_Stat(f, &fi);
	if (err.code == 200) {
		fsize = Efs_FileInfo_Fsize(fi);
		if (fsize <= Efs_Rio_PutExtra_ChunkSize(extra)) { // file is too small, don't need resumable-io
			Efs_File_Close(f);

			Efs_Zero(extra1);
			Efs_Io_PutExtra_initFrom(&extra1, extra);

			return Efs_Io_PutFile(self, ret, uptoken, key, localFile, &extra1);
		}
		err = Efs_Rio_Put(self, ret, uptoken, key, Efs_FileReaderAt(f), fsize, extra);
	}
	Efs_File_Close(f);
	return err;
}

