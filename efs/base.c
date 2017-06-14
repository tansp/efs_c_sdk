/*
 ============================================================================
 Name        : base.c
 Author      : gosun.com
 Copyright   : 2017(c) beijing efs Information Technologies Co., Ltd.
 Description :
 ============================================================================
 */

#include "base.h"
#include "../b64/urlsafe_b64.h"
#include <assert.h>
#include <time.h>
#include <errno.h>

/*============================================================================*/
/* type Efs_Free */

void Efs_Free(void* addr)
{
	free(addr);
}

/*============================================================================*/
/* type Efs_Count */

#if defined(_WIN32)

#include <windows.h>

Efs_Count Efs_Count_Inc(Efs_Count* self)
{
	return InterlockedIncrement(self);
}

Efs_Count Efs_Count_Dec(Efs_Count* self)
{
	return InterlockedDecrement(self);
}

#else

Efs_Count Efs_Count_Inc(Efs_Count* self)
{
	return __sync_add_and_fetch(self, 1);
}

Efs_Count Efs_Count_Dec(Efs_Count* self)
{
	return __sync_sub_and_fetch(self, 1);
}

#endif

/*============================================================================*/
/* func Efs_Seconds */

Efs_Int64 Efs_Seconds()
{
	return (Efs_Int64)time(NULL);
}

/*============================================================================*/
/* func Efs_QueryEscape */

typedef enum {
	encodePath,
	encodeUserPassword,
	encodeQueryComponent,
	encodeFragment,
} escapeMode;

// Return true if the specified character should be escaped when
// appearing in a URL string, according to RFC 3986.
static int Efs_shouldEscape(int c, escapeMode mode)
{
	if (('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z') || ('0' <= c && c <= '9')) {
		return 0;
	}

	switch (c) {
	case '-': case '_': case '.': case '~': // §2.3 Unreserved characters (mark)
		return 0;
	case '$': case '&': case '+': case ',': case '/': case ':': case ';': case '=': case '?': case '@': // §2.2 Reserved characters (reserved)
		switch (mode) {
			case encodePath: // §3.3
				return c == '?';
			case encodeUserPassword: // §3.2.2
				return c == '@' || c == '/' || c == ':';
			case encodeQueryComponent: // §3.4
				return 1;
			case encodeFragment: // §4.1
				return 0;
		}
	}

	return 1;
}

static const char Efs_hexTable[] = "0123456789ABCDEF";

static char* Efs_escape(const char* s, escapeMode mode, Efs_Bool* fesc)
{
	int spaceCount = 0;
	int hexCount = 0;
	int i, j, len = strlen(s);
	int c;
	char* t;

	for (i = 0; i < len; i++) {
		// prevent c from sign extension
		c = ((int)s[i]) & 0xFF;
		if (Efs_shouldEscape(c, mode)) {
			if (c == ' ' && mode == encodeQueryComponent) {
				spaceCount++;
			} else {
				hexCount++;
			}
		}
	}

	if (spaceCount == 0 && hexCount == 0) {
		*fesc = Efs_False;
		return (char*)s;
	}

	t = (char*)malloc(len + 2*hexCount + 1);
	j = 0;
	for (i = 0; i < len; i++) {
		// prevent c from sign extension
		c = ((int)s[i]) & 0xFF;
		if (Efs_shouldEscape(c, mode)) {
			if (c == ' ' && mode == encodeQueryComponent) {
				t[j] = '+';
				j++;
			} else {
				t[j] = '%';
				t[j+1] = Efs_hexTable[c>>4];
				t[j+2] = Efs_hexTable[c&15];
				j += 3;
			}
		} else {
			t[j] = s[i];
			j++;
		}
	}
	t[j] = '\0';
	*fesc = Efs_True;
	return t;
}

char* Efs_PathEscape(const char* s, Efs_Bool* fesc)
{
	return Efs_escape(s, encodePath, fesc);
}

char* Efs_QueryEscape(const char* s, Efs_Bool* fesc)
{
	return Efs_escape(s, encodeQueryComponent, fesc);
}

/*============================================================================*/
/* func Efs_String_Concat */

char* Efs_String_Concat2(const char* s1, const char* s2)
{
	size_t len1 = strlen(s1);
	size_t len2 = strlen(s2);
	char* p = (char*)malloc(len1 + len2 + 1);
	memcpy(p, s1, len1);
	memcpy(p + len1, s2, len2);
	p[len1 + len2] = '\0';
	return p;
}

char* Efs_String_Concat3(const char* s1, const char* s2, const char* s3)
{
	size_t len1 = strlen(s1);
	size_t len2 = strlen(s2);
	size_t len3 = strlen(s3);
	char* p = (char*)malloc(len1 + len2 + len3 + 1);
	memcpy(p, s1, len1);
	memcpy(p + len1, s2, len2);
	memcpy(p + len1 + len2, s3, len3);
	p[len1 + len2 + len3] = '\0';
	return p;
}

char* Efs_String_Concat(const char* s1, ...)
{
	va_list ap;
	char* p;
	const char* s;
	size_t len, slen, len1 = strlen(s1);

	va_start(ap, s1);
	len = len1;
	for (;;) {
		s = va_arg(ap, const char*);
		if (s == NULL) {
			break;
		}
		len += strlen(s);
	}

	p = (char*)malloc(len + 1);

	va_start(ap, s1);
	memcpy(p, s1, len1);
	len = len1;
	for (;;) {
		s = va_arg(ap, const char*);
		if (s == NULL) {
			break;
		}
		slen = strlen(s);
		memcpy(p + len, s, slen);
		len += slen;
	}
	p[len] = '\0';
	return p;
}

char* Efs_String_Join(const char* deli, char* strs[], int strCount)
{
	int i = 0;
	char * ret = NULL;
	char * pos = NULL;
	char * tmpRet = NULL;
	size_t totalLen = 0;
	size_t copyLen = 0;
	size_t deliLen = 0;

	if (strCount == 1) {
		return strdup(strs[0]);
	}

	for (i = 0; i < strCount; i += 1) {
		totalLen += strlen(strs[i]);
	} // for

	deliLen = strlen(deli);
	totalLen += deliLen * (strCount - 1);
	ret = (char *)malloc(totalLen + 1);
	if (ret == NULL) {
		return NULL;
	}

	pos = ret;
	copyLen = strlen(strs[0]);
	memcpy(pos, strs[0], copyLen);
	pos += copyLen;

	for (i = 1; i < strCount; i += 1) {
		memcpy(pos, deli, deliLen);
		pos += deliLen;

		copyLen = strlen(strs[i]);
		memcpy(pos, strs[i], copyLen);
		pos += copyLen;
	} // for

	ret[totalLen] = '\0';
	return ret;
} // Efs_String_Join

char* Efs_String_Dup(const char* src)
{
	return strdup(src);
} // Efs_String_Dup

/*============================================================================*/
/* func Efs_String_Encode */

char* Efs_String_Encode(const char* buf)
{
	const size_t cb = strlen(buf);
	const size_t cbDest = urlsafe_b64_encode(buf, cb, NULL, 0);
	char* dest = (char*)malloc(cbDest + 1);
	const size_t cbReal = urlsafe_b64_encode(buf, cb, dest, cbDest);
	dest[cbReal] = '\0';
	return dest;
}

char* Efs_Memory_Encode(const char* buf, const size_t cb)
{
	const size_t cbDest = urlsafe_b64_encode(buf, cb, NULL, 0);
	char* dest = (char*)malloc(cbDest + 1);
	const size_t cbReal = urlsafe_b64_encode(buf, cb, dest, cbDest);
	dest[cbReal] = '\0';
	return dest;
}

char* Efs_String_Decode(const char* buf)
{
	const size_t cb = strlen(buf);
	const size_t cbDest = urlsafe_b64_decode(buf, cb, NULL, 0);
	char* dest = (char*)malloc(cbDest + 1);
	const size_t cbReal = urlsafe_b64_decode(buf, cb, dest, cbDest);
	dest[cbReal] = '\0';
	return dest;
}

/*============================================================================*/
/* type Efs_Buffer */

static void Efs_Buffer_expand(Efs_Buffer* self, size_t expandSize)
{
	size_t oldSize = self->curr - self->buf;
	size_t newSize = (self->bufEnd - self->buf) << 1;
	expandSize += oldSize;
	while (newSize < expandSize) {
		newSize <<= 1;
	}
	self->buf = realloc(self->buf, newSize);
	self->curr = self->buf + oldSize;
	self->bufEnd = self->buf + newSize;
}

void Efs_Buffer_Init(Efs_Buffer* self, size_t initSize)
{
	self->buf = self->curr = (char*)malloc(initSize);
	self->bufEnd = self->buf + initSize;
}

void Efs_Buffer_Reset(Efs_Buffer* self)
{
	self->curr = self->buf;
}

void Efs_Buffer_Cleanup(Efs_Buffer* self)
{
	if (self->buf != NULL) {
		free(self->buf);
		self->buf = NULL;
	}
}

size_t Efs_Buffer_Len(Efs_Buffer* self)
{
	return self->curr - self->buf;
}

const char* Efs_Buffer_CStr(Efs_Buffer* self)
{
	if (self->curr >= self->bufEnd) {
		Efs_Buffer_expand(self, 1);
	}
	*self->curr = '\0';
	return self->buf;
}

void Efs_Buffer_PutChar(Efs_Buffer* self, char ch)
{
	if (self->curr >= self->bufEnd) {
		Efs_Buffer_expand(self, 1);
	}
	*self->curr++ = ch;
}

size_t Efs_Buffer_Write(Efs_Buffer* self, const void* buf, size_t n)
{
	if (self->curr + n > self->bufEnd) {
		Efs_Buffer_expand(self, n);
	}
	memcpy(self->curr, buf, n);
	self->curr += n;
	return n;
}

size_t Efs_Buffer_Fwrite(const void *buf, size_t size, size_t nmemb, void *self)
{
	assert(size == 1);
	return Efs_Buffer_Write((Efs_Buffer*)self, buf, nmemb);
}

Efs_Writer Efs_BufWriter(Efs_Buffer* self)
{
	Efs_Writer writer = {self, Efs_Buffer_Fwrite};
	return writer;
}

/*============================================================================*/
/* Efs Format Functions */

char* Efs_Buffer_Expand(Efs_Buffer* self, size_t n)
{
	if (self->curr + n > self->bufEnd) {
		Efs_Buffer_expand(self, n);
	}
	return self->curr;
}

void Efs_Buffer_Commit(Efs_Buffer* self, char* p)
{
	assert(p >= self->curr);
	assert(p <= self->bufEnd);
	self->curr = p;
}

void Efs_Buffer_AppendUint(Efs_Buffer* self, Efs_Uint64 v)
{
	char buf[32];
	char* p = buf+32;
	for (;;) {
		*--p = '0' + (char)(v % 10);
		v /= 10;
		if (v == 0) {
			break;
		}
	}
	Efs_Buffer_Write(self, p, buf+32-p);
}

void Efs_Buffer_AppendInt(Efs_Buffer* self, Efs_Int64 v)
{
	if (v < 0) {
		v = -v;
		Efs_Buffer_PutChar(self, '-');
	}
	Efs_Buffer_AppendUint(self, v);
}

void Efs_Buffer_AppendError(Efs_Buffer* self, Efs_Error v)
{
	Efs_Buffer_PutChar(self, 'E');
	Efs_Buffer_AppendInt(self, v.code);
	if (v.message) {
		Efs_Buffer_PutChar(self, ' ');
		Efs_Buffer_Write(self, v.message, strlen(v.message));
	}
}

void Efs_Buffer_AppendEncodedBinary(Efs_Buffer* self, const char* buf, size_t cb)
{
	const size_t cbDest = urlsafe_b64_encode(buf, cb, NULL, 0);
	char* dest = Efs_Buffer_Expand(self, cbDest);
	const size_t cbReal = urlsafe_b64_encode(buf, cb, dest, cbDest);
	Efs_Buffer_Commit(self, dest + cbReal);
}

void Efs_Buffer_appendUint(Efs_Buffer* self, Efs_Valist* ap)
{
	unsigned v = va_arg(ap->items, unsigned);
	Efs_Buffer_AppendUint(self, v);
}

void Efs_Buffer_appendInt(Efs_Buffer* self, Efs_Valist* ap)
{
	int v = va_arg(ap->items, int);
	Efs_Buffer_AppendInt(self, v);
}

void Efs_Buffer_appendUint64(Efs_Buffer* self, Efs_Valist* ap)
{
	Efs_Uint64 v = va_arg(ap->items, Efs_Uint64);
	Efs_Buffer_AppendUint(self, v);
}

void Efs_Buffer_appendInt64(Efs_Buffer* self, Efs_Valist* ap)
{
	Efs_Int64 v = va_arg(ap->items, Efs_Int64);
	Efs_Buffer_AppendInt(self, v);
}

void Efs_Buffer_appendString(Efs_Buffer* self, Efs_Valist* ap)
{
	const char* v = va_arg(ap->items, const char*);
	if (v == NULL) {
		v = "(null)";
	}
	Efs_Buffer_Write(self, v, strlen(v));
}

void Efs_Buffer_appendEncodedString(Efs_Buffer* self, Efs_Valist* ap)
{
	const char* v = va_arg(ap->items, const char*);
	size_t n = strlen(v);
	Efs_Buffer_AppendEncodedBinary(self, v, n);
}

void Efs_Buffer_appendError(Efs_Buffer* self, Efs_Valist* ap)
{
	Efs_Error v = va_arg(ap->items, Efs_Error);
	Efs_Buffer_AppendError(self, v);
}

void Efs_Buffer_appendPercent(Efs_Buffer* self, Efs_Valist* ap)
{
	Efs_Buffer_PutChar(self, '%');
}

/*============================================================================*/
/* Efs Format */

typedef struct _Efs_formatProc {
	Efs_FnAppender Append;
	char esc;
} Efs_formatProc;

static Efs_formatProc efs_formatProcs[] = {
	{ Efs_Buffer_appendInt, 'd' },
	{ Efs_Buffer_appendUint, 'u' },
	{ Efs_Buffer_appendInt64, 'D' },
	{ Efs_Buffer_appendUint64, 'U' },
	{ Efs_Buffer_appendString, 's' },
	{ Efs_Buffer_appendEncodedString, 'S' },
	{ Efs_Buffer_appendError, 'E' },
	{ Efs_Buffer_appendPercent, '%' },
};

static Efs_FnAppender efs_Appenders[128] = {0};

void Efs_Format_Register(char esc, Efs_FnAppender appender)
{
	if ((unsigned)esc < 128) {
		efs_Appenders[esc] = appender;
	}
}

void Efs_Buffer_formatInit()
{
	Efs_formatProc* p;
	Efs_formatProc* pEnd = (Efs_formatProc*)((char*)efs_formatProcs + sizeof(efs_formatProcs));
	for (p = efs_formatProcs; p < pEnd; p++) {
		efs_Appenders[p->esc] = p->Append;
	}
}

void Efs_Buffer_AppendFormatV(Efs_Buffer* self, const char* fmt, Efs_Valist* args)
{
	unsigned char ch;
	const char* p;
	Efs_FnAppender appender;

	for (;;) {
		p = strchr(fmt, '%');
		if (p == NULL) {
			break;
		}
		if (p > fmt) {
			Efs_Buffer_Write(self, fmt, p - fmt);
		}
		p++;
		ch = *p++;
		fmt = p;
		if (ch < 128) {
			appender = efs_Appenders[ch];
			if (appender != NULL) {
				appender(self, args);
				continue;
			}
		}
		Efs_Buffer_PutChar(self, '%');
		Efs_Buffer_PutChar(self, ch);
	}
	if (*fmt) {
		Efs_Buffer_Write(self, fmt, strlen(fmt));
	}
}

void Efs_Buffer_AppendFormat(Efs_Buffer* self, const char* fmt, ...)
{
	Efs_Valist args;
	va_start(args.items, fmt);
	Efs_Buffer_AppendFormatV(self, fmt, &args);
}

const char* Efs_Buffer_Format(Efs_Buffer* self, const char* fmt, ...)
{
	Efs_Valist args;
	va_start(args.items, fmt);
	Efs_Buffer_Reset(self);
	Efs_Buffer_AppendFormatV(self, fmt, &args);
	return Efs_Buffer_CStr(self);
}

char* Efs_String_Format(size_t initSize, const char* fmt, ...)
{
	Efs_Valist args;
	Efs_Buffer buf;
	va_start(args.items, fmt);
	Efs_Buffer_Init(&buf, initSize);
	Efs_Buffer_AppendFormatV(&buf, fmt, &args);
	return (char*)Efs_Buffer_CStr(&buf);
}

/*============================================================================*/
/* func Efs_FILE_Reader */

Efs_Reader Efs_FILE_Reader(FILE* fp)
{
	Efs_Reader reader = {fp, (Efs_FnRead)fread};
	return reader;
}

Efs_Writer Efs_FILE_Writer(FILE* fp)
{
	Efs_Writer writer = {fp, (Efs_FnWrite)fwrite};
	return writer;
}

/*============================================================================*/
/* func Efs_Copy */

Efs_Error Efs_OK = {
	200, "OK"
};

Efs_Error Efs_Copy(Efs_Writer w, Efs_Reader r, void* buf, size_t n, Efs_Int64* ret)
{
	Efs_Int64 fsize = 0;
	size_t n1, n2;
	char* p = (char*)buf;
	if (buf == NULL) {
		p = (char*)malloc(n);
	}
	for (;;) {
		n1 = r.Read(p, 1, n, r.self);
		if (n1 > 0) {
			n2 = w.Write(p, 1, n1, w.self);
			fsize += n2;
		} else {
			n2 = 0;
		}
		if (n2 != n) {
			break;
		}
	}
	if (buf == NULL) {
		free(p);
	}
	if (ret) {
		*ret = fsize;
	}
	return Efs_OK;
}

/*============================================================================*/
/* func Efs_Null_Fwrite */

size_t Efs_Null_Fwrite(const void *buf, size_t size, size_t nmemb, void *self)
{
	return nmemb;
}

Efs_Writer Efs_Discard = {
	NULL, Efs_Null_Fwrite
};

/*============================================================================*/
/* func Efs_Null_Log */

void Efs_Null_Log(const char* fmt, ...)
{
}

/*============================================================================*/
/* func Efs_Stderr_Info/Warn */

static const char* efs_Levels[] = {
	"[DEBUG]",
	"[INFO]",
	"[WARN]",
	"[ERROR]",
	"[PANIC]",
	"[FATAL]"
};

void Efs_Logv(Efs_Writer w, int ilvl, const char* fmt, Efs_Valist* args)
{
	const char* level = efs_Levels[ilvl];
	Efs_Buffer log;
	Efs_Buffer_Init(&log, 512);
	Efs_Buffer_Write(&log, level, strlen(level));
	Efs_Buffer_PutChar(&log, ' ');
	Efs_Buffer_AppendFormatV(&log, fmt, args);
	Efs_Buffer_PutChar(&log, '\n');
	w.Write(log.buf, 1, log.curr-log.buf, w.self);
	Efs_Buffer_Cleanup(&log);
}

void Efs_Stderr_Info(const char* fmt, ...)
{
	Efs_Valist args;
	va_start(args.items, fmt);
	Efs_Logv(Efs_Stderr, Efs_Linfo, fmt, &args);
}

void Efs_Stderr_Warn(const char* fmt, ...)
{
	Efs_Valist args;
	va_start(args.items, fmt);
	Efs_Logv(Efs_Stderr, Efs_Lwarn, fmt, &args);
}

/*============================================================================*/

