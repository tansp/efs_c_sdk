/*
 ============================================================================
 Name        : qetag.h
 Author      : gosun.com
 Copyright   : 2017(c) beijing efs Information Technologies Co., Ltd.
 Description : 
 ============================================================================
 */

#include "base.h"
#include "macro.h"

#ifndef EFS_QETAG_H
#define EFS_QETAG_H

#ifdef __cplusplus
extern "C"
{
#endif

#pragma pack(1)

struct _Efs_Qetag_Context;
struct _Efs_Qetag_Block;

// 底层函数
EFS_DLLAPI extern Efs_Error Efs_Qetag_New(struct _Efs_Qetag_Context ** ctx, unsigned int concurrency);
EFS_DLLAPI extern Efs_Error Efs_Qetag_Reset(struct _Efs_Qetag_Context * ctx);
EFS_DLLAPI extern void Efs_Qetag_Destroy(struct _Efs_Qetag_Context * ctx);
EFS_DLLAPI extern Efs_Error Efs_Qetag_Update(struct _Efs_Qetag_Context * ctx, const char * buf, size_t bufSize);
EFS_DLLAPI extern Efs_Error Efs_Qetag_Final(struct _Efs_Qetag_Context * ctx, char ** digest);

EFS_DLLAPI extern Efs_Error Efs_Qetag_AllocateBlock(struct _Efs_Qetag_Context * ctx, struct _Efs_Qetag_Block ** blk, size_t * blkCapacity);
EFS_DLLAPI extern Efs_Error Efs_Qetag_UpdateBlock(struct _Efs_Qetag_Block * blk, const char * buf, size_t bufSize, size_t * blkCapacity);
EFS_DLLAPI extern void Efs_Qetag_CommitBlock(struct _Efs_Qetag_Context * ctx, struct _Efs_Qetag_Block * blk);

// 单线程计算 QETAG
EFS_DLLAPI extern Efs_Error Efs_Qetag_DigestFile(const char * localFile, char ** digest);
EFS_DLLAPI extern Efs_Error Efs_Qetag_DigestBuffer(const char * buf, size_t fsize, char ** digest);

#pragma pack()

#ifdef __cplusplus
}
#endif

#endif /* EFS_QETAG_H */
