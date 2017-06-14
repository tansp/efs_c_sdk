/*
 ============================================================================
 Name        : base.h
 Author      : gosun.com
 Copyright   : 2017(c) beijing efs Information Technologies Co., Ltd.
 Description : 
 ============================================================================
 */

#ifndef EFS_BASE_H
#define EFS_BASE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "macro.h"

/*============================================================================*/
/* func type ssize_t */

#ifdef _WIN32

#include <sys/types.h>

#ifndef _W64
#define _W64
#endif

typedef _W64 int ssize_t;

#endif

#if defined(_MSC_VER)

typedef _int64 Efs_Off_T;

#else

typedef off_t Efs_Off_T;

#endif

#pragma pack(1)

#ifdef __cplusplus
extern "C"
{
#endif

/*============================================================================*/
/* func Efs_Zero */

#define Efs_Zero(v)		memset(&v, 0, sizeof(v))

/*============================================================================*/
/* func Efs_snprintf */

#if defined(_MSC_VER)
#define Efs_snprintf		_snprintf
#else
#define Efs_snprintf		snprintf
#endif

/*============================================================================*/
/* type Efs_Int64, Efs_Uint32 */

#if defined(_MSC_VER)
typedef _int64 Efs_Int64;
typedef unsigned _int64 Efs_Uint64;
#else
typedef long long Efs_Int64;
typedef unsigned long long Efs_Uint64;
#endif

typedef unsigned int Efs_Uint32;
typedef unsigned short Efs_Uint16;

/*============================================================================*/
/* type Efs_Bool */

typedef int Efs_Bool;

enum {
	Efs_False = 0,
	Efs_True = 1
};

/*============================================================================*/
/* type Efs_Error */

/* @gist error */

typedef struct _Efs_Error {
	int code;
	const char* message;
} Efs_Error;

/* @endgist */

EFS_DLLAPI extern Efs_Error Efs_OK;

/*============================================================================*/
/* type Efs_Free */

void Efs_Free(void* addr);

/*============================================================================*/
/* type Efs_Count */

typedef long Efs_Count;

EFS_DLLAPI extern Efs_Count Efs_Count_Inc(Efs_Count* self);
EFS_DLLAPI extern Efs_Count Efs_Count_Dec(Efs_Count* self);

/*============================================================================*/
/* func Efs_String_Concat */

EFS_DLLAPI extern char* Efs_String_Concat2(const char* s1, const char* s2);
EFS_DLLAPI extern char* Efs_String_Concat3(const char* s1, const char* s2, const char* s3);
EFS_DLLAPI extern char* Efs_String_Concat(const char* s1, ...);

EFS_DLLAPI extern char* Efs_String_Format(size_t initSize, const char* fmt, ...);

EFS_DLLAPI extern char* Efs_String_Join(const char* deli, char* strs[], int strCount);
EFS_DLLAPI extern char* Efs_String_Dup(const char* src);

/*============================================================================*/
/* func Efs_String_Encode */

EFS_DLLAPI extern char* Efs_Memory_Encode(const char* buf, const size_t cb);
EFS_DLLAPI extern char* Efs_String_Encode(const char* s);
EFS_DLLAPI extern char* Efs_String_Decode(const char* s);

/*============================================================================*/
/* func Efs_QueryEscape */

char* Efs_PathEscape(const char* s, Efs_Bool* fesc);
char* Efs_QueryEscape(const char* s, Efs_Bool* fesc);

/*============================================================================*/
/* func Efs_Seconds */

Efs_Int64 Efs_Seconds();

/*============================================================================*/
/* type Efs_Reader */

typedef size_t (*Efs_FnRead)(void *buf, size_t, size_t n, void *self);

typedef struct _Efs_Reader {
	void* self;
	Efs_FnRead Read;
} Efs_Reader;

EFS_DLLAPI extern Efs_Reader Efs_FILE_Reader(FILE* fp);

/*============================================================================*/
/* type Efs_Writer */

typedef size_t (*Efs_FnWrite)(const void *buf, size_t, size_t n, void *self);

typedef struct _Efs_Writer {
	void* self;
	Efs_FnWrite Write;
} Efs_Writer;

EFS_DLLAPI extern Efs_Writer Efs_FILE_Writer(FILE* fp);
EFS_DLLAPI extern Efs_Error Efs_Copy(Efs_Writer w, Efs_Reader r, void* buf, size_t n, Efs_Int64* ret);

#define Efs_Stderr Efs_FILE_Writer(stderr)

/*============================================================================*/
/* type Efs_ReaderAt */

typedef	ssize_t (*Efs_FnReadAt)(void* self, void *buf, size_t bytes, Efs_Off_T offset);

typedef struct _Efs_ReaderAt {
	void* self;
	Efs_FnReadAt ReadAt;
} Efs_ReaderAt;

/*============================================================================*/
/* type Efs_Buffer */

typedef struct _Efs_Valist {
	va_list items;
} Efs_Valist;

typedef struct _Efs_Buffer {
	char* buf;
	char* curr;
	char* bufEnd;
} Efs_Buffer;

EFS_DLLAPI extern void Efs_Buffer_Init(Efs_Buffer* self, size_t initSize);
EFS_DLLAPI extern void Efs_Buffer_Reset(Efs_Buffer* self);
EFS_DLLAPI extern void Efs_Buffer_AppendInt(Efs_Buffer* self, Efs_Int64 v);
EFS_DLLAPI extern void Efs_Buffer_AppendUint(Efs_Buffer* self, Efs_Uint64 v);
EFS_DLLAPI extern void Efs_Buffer_AppendError(Efs_Buffer* self, Efs_Error v);
EFS_DLLAPI extern void Efs_Buffer_AppendEncodedBinary(Efs_Buffer* self, const char* buf, size_t cb);
EFS_DLLAPI extern void Efs_Buffer_AppendFormat(Efs_Buffer* self, const char* fmt, ...);
EFS_DLLAPI extern void Efs_Buffer_AppendFormatV(Efs_Buffer* self, const char* fmt, Efs_Valist* args);
EFS_DLLAPI extern void Efs_Buffer_Cleanup(Efs_Buffer* self);

EFS_DLLAPI extern const char* Efs_Buffer_CStr(Efs_Buffer* self);
EFS_DLLAPI extern const char* Efs_Buffer_Format(Efs_Buffer* self, const char* fmt, ...);

EFS_DLLAPI extern void Efs_Buffer_PutChar(Efs_Buffer* self, char ch);

EFS_DLLAPI extern size_t Efs_Buffer_Len(Efs_Buffer* self);
EFS_DLLAPI extern size_t Efs_Buffer_Write(Efs_Buffer* self, const void* buf, size_t n);
EFS_DLLAPI extern size_t Efs_Buffer_Fwrite(const void* buf, size_t, size_t n, void* self);

EFS_DLLAPI extern Efs_Writer Efs_BufWriter(Efs_Buffer* self);

EFS_DLLAPI extern char* Efs_Buffer_Expand(Efs_Buffer* self, size_t n);
EFS_DLLAPI extern void Efs_Buffer_Commit(Efs_Buffer* self, char* p);

typedef void (*Efs_FnAppender)(Efs_Buffer* self, Efs_Valist* ap);

EFS_DLLAPI extern void Efs_Format_Register(char esc, Efs_FnAppender appender);

/*============================================================================*/
/* func Efs_Null_Fwrite */

EFS_DLLAPI extern size_t Efs_Null_Fwrite(const void* buf, size_t, size_t n, void* self);

EFS_DLLAPI extern Efs_Writer Efs_Discard;

/*============================================================================*/
/* type Efs_ReadBuf */

typedef struct _Efs_ReadBuf {
	const char* buf;
	Efs_Off_T off;
	Efs_Off_T limit;
} Efs_ReadBuf;

EFS_DLLAPI extern Efs_Reader Efs_BufReader(Efs_ReadBuf* self, const char* buf, size_t bytes);
EFS_DLLAPI extern Efs_ReaderAt Efs_BufReaderAt(Efs_ReadBuf* self, const char* buf, size_t bytes);

/*============================================================================*/
/* type Efs_Tee */

typedef struct _Efs_Tee {
	Efs_Reader r;
	Efs_Writer w;
} Efs_Tee;

EFS_DLLAPI extern Efs_Reader Efs_TeeReader(Efs_Tee* self, Efs_Reader r, Efs_Writer w);

/*============================================================================*/
/* type Efs_Section */

typedef struct _Efs_Section {
	Efs_ReaderAt r;
	Efs_Off_T off;
	Efs_Off_T limit;
} Efs_Section;

EFS_DLLAPI extern Efs_Reader Efs_SectionReader(Efs_Section* self, Efs_ReaderAt r, Efs_Off_T off, size_t n);

/*============================================================================*/
/* type Efs_Crc32 */

EFS_DLLAPI extern unsigned long Efs_Crc32_Update(unsigned long inCrc32, const void *buf, size_t bufLen);

typedef struct _Efs_Crc32 {
	unsigned long val;
} Efs_Crc32;

EFS_DLLAPI extern Efs_Writer Efs_Crc32Writer(Efs_Crc32* self, unsigned long inCrc32);

/*============================================================================*/
/* type Efs_File */

typedef struct _Efs_File Efs_File;

#if defined(_MSC_VER)
typedef struct _Efs_FileInfo {
	Efs_Off_T     st_size;    /* total size, in bytes */
	time_t          st_atime;   /* time of last access */
	time_t          st_mtime;   /* time of last modification */
	time_t          st_ctime;   /* time of last status change */
} Efs_FileInfo;
#else

#include <sys/stat.h>

typedef struct stat Efs_FileInfo;

#endif

EFS_DLLAPI extern Efs_Error Efs_File_Open(Efs_File** pp, const char* file);
EFS_DLLAPI extern Efs_Error Efs_File_Stat(Efs_File* self, Efs_FileInfo* fi);

#define Efs_FileInfo_Fsize(fi) ((fi).st_size)

EFS_DLLAPI extern void Efs_File_Close(void* self);

EFS_DLLAPI extern ssize_t Efs_File_ReadAt(void* self, void *buf, size_t bytes, Efs_Off_T offset);

EFS_DLLAPI extern Efs_ReaderAt Efs_FileReaderAt(Efs_File* self);

/*============================================================================*/
/* type Efs_Log */

#define Efs_Ldebug	0
#define Efs_Linfo		1
#define Efs_Lwarn		2
#define Efs_Lerror	3
#define Efs_Lpanic	4
#define Efs_Lfatal	5

EFS_DLLAPI extern void Efs_Logv(Efs_Writer w, int level, const char* fmt, Efs_Valist* args);

EFS_DLLAPI extern void Efs_Stderr_Info(const char* fmt, ...);
EFS_DLLAPI extern void Efs_Stderr_Warn(const char* fmt, ...);

EFS_DLLAPI extern void Efs_Null_Log(const char* fmt, ...);

#ifndef Efs_Log_Info

#ifdef EFS_DISABLE_LOG

#define Efs_Log_Info	Efs_Null_Log
#define Efs_Log_Warn	Efs_Null_Log

#else

#define Efs_Log_Info	Efs_Stderr_Info
#define Efs_Log_Warn	Efs_Stderr_Warn

#endif

#endif

/*============================================================================*/

#pragma pack()

#ifdef __cplusplus
}
#endif

#endif /* EFS_BASE_H */
