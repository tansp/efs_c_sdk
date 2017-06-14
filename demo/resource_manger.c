/******************************************************************************* 
 *  @file      demo.c 2013\10\11 10:55:14 $
 *  @author    Wang Xiaotao<wangxiaotao1980@gmail.com> (中文编码测试)
 ******************************************************************************/

#include "../efs/resumable_io.h"

#include "../efs/base.h"
#include "../efs/rs.h"
#include "../efs/io.h"
#include <stdio.h>
/******************************************************************************/


/*debug 函数*/
void debuginfo(Efs_Client* client, Efs_Error err)
{
    printf("\nerror code: %d, message: %s\n", err.code, err.message);
    printf("response header:\n%s", Efs_Buffer_CStr(&client->respHeader));
    printf("response body:\n%s", Efs_Buffer_CStr(&client->b));
    printf("\n\n\n");
}
/*得到上传文件的token*/
char* upLoadToken(const char* bucket, Efs_Mac* mac)
{
    Efs_RS_PutPolicy putPolicy;
    Efs_Zero(putPolicy);
    putPolicy.scope = bucket;
    return Efs_RS_PutPolicy_Token(&putPolicy, mac);
}
/*得到下载文件的url的token*/
char* downloadUrl(const char* domain, const char* key, Efs_Mac* mac)
{

    char* url = 0;
    char* baseUrl = 0;

    Efs_RS_GetPolicy getPolicy;
    Efs_Zero(getPolicy);

    baseUrl = Efs_RS_MakeBaseUrl(domain, key);
    url = Efs_RS_GetPolicy_MakeRequest(&getPolicy, baseUrl, mac);
    Efs_Free(baseUrl);
    return url;
}



void demoGetFileStat(Efs_Client* pClient, const char* bucketName, const char* keyName)
{

    /* 假设Efs帐号存储下有 bucket名称为bucketName所指字符串， 此bucket下有keyName所指字符串的名称文件，
     * 则此如下方法，查询keyName的文件信息
     */
    Efs_RS_StatRet statRet;
    Efs_Error error = Efs_RS_Stat(pClient, &statRet, bucketName, keyName);
    /* 判断http返回值*/
    if (error.code != 200)
    {   /*非200，不正确返回*/
        printf("get file %s:%s stat error.\n", bucketName, keyName);
        debuginfo(pClient, error);
    }else
    {   /*200, 正确返回了, 你可以通过statRet变量查询一些关于这个文件的信息*/
         printf("get file %s:%s stat success.\n", bucketName, keyName);
    }
}

void demoMoveFile(Efs_Client* pClient, const char* bucketName, const char* src, const char* dest)
{
    /* 假设Efs帐号存储下有 bucket名称为bucketName， 此bucket下有名称为src文件，
     * 则此如下方法，改名子src 为 dest
     */
    Efs_Error error = Efs_RS_Move(pClient, bucketName, src , bucketName, dest);
    if (error.code != 200)
    {
        printf("rename file from %s:%s to %s:%s error.\n", bucketName, src, bucketName, dest);
        debuginfo(pClient, error);
    }
    else
    {
       printf("rename file from %s:%s to %s:%s success.\n", bucketName, src, bucketName, dest);
    }


    /* 以上改名的逆操作
     */
    error = Efs_RS_Move(pClient, bucketName, dest , bucketName, src);
    if (error.code != 200)
    {
        printf("rename file from %s:%s to %s:%s error.\n", bucketName,dest, bucketName, src);
        debuginfo(pClient, error);
    }
    else
    {
       printf("rename file from %s:%s to %s:%s success.\n", bucketName, dest, bucketName, src);
    }
}

void demoCopyFile(Efs_Client* pClient, const char* bucketName, const char* src, const char* dest)
{
    /* 假设Efs帐号存储下有 bucket名称为bucketName， 此bucket下有src文件，
     * 则此如下方法，拷贝src 为 dest
     */
    Efs_Error error = Efs_RS_Copy(pClient, bucketName, src, bucketName, dest);
    if (error.code != 200)
    {
        printf("copy file from %s:%s to %s:%s error.\n", bucketName, src, bucketName, dest);
        debuginfo(pClient, error);
    }
    else
    {
        printf("copy file from %s:%s to %s:%s success.\n", bucketName, src, bucketName, dest);
    }
}

void demoDeleteFile(Efs_Client* pClient, const char* bucketName, const char* keyName)
{
    /* 假设Efs帐号存储下有 bucket名称为bucketName， 此bucket下有 keyName 文件，
     * 则此如下方法, 删除 keyName
     */

    Efs_Error error = Efs_RS_Delete(pClient, bucketName, keyName);
    if (error.code != 200)
    {
        printf("delete file %s:%s error.\n", bucketName, keyName);
        debuginfo(pClient, error);
    }
    else
    {
        printf("delete file %s:%s success.\n", bucketName, keyName);
    }
}

void demoBatchStatFiles(Efs_Client* pClient, const char* bucketName)
{
    /*  假设Efs帐号存储下有 bucket名称为bucketName，此bucket 下有批量文件
     *  此demo function演示如何批量得到七牛云存储文件信息
     */
    Efs_RS_EntryPath entryPath[] = {
                                        {bucketName, "1.txt"},
                                        {bucketName, "2.txt"},
                                        {bucketName, "3.txt"},
                                        {bucketName, "4.txt"},
                                        {bucketName, "5.txt"},
                                       };
    int len = sizeof(entryPath)/sizeof(Efs_RS_EntryPath);
    Efs_RS_BatchStatRet* rets = (Efs_RS_BatchStatRet*)calloc(len, sizeof(Efs_RS_BatchStatRet));
    Efs_Error error = Efs_RS_BatchStat(pClient, rets, entryPath, len);
    if (200 != error.code)
    {
        printf("get files stat error.\n");
        debuginfo(pClient, error);
    }
    else
    {
        printf("get files stat success.\n");
    }
    free(rets);
}

void demoBatchCopyFiles(Efs_Client* pClient, const char* bucketName)
{
    Efs_RS_EntryPathPair entryPathpair1[] ={{{bucketName, "1.txt"}, {bucketName, "1_copy.txt"}},
                                              {{bucketName, "2.txt"}, {bucketName, "2_copy.txt"}},
                                              {{bucketName, "3.txt"}, {bucketName, "3_copy.txt"}},
                                              {{bucketName, "4.txt"}, {bucketName, "4_copy.txt"}},
                                              {{bucketName, "5.txt"}, {bucketName, "5_copy.txt"}},
                                             };
    int len = sizeof(entryPathpair1)/sizeof(Efs_RS_EntryPathPair);
    Efs_RS_BatchItemRet* itemRets = (Efs_RS_BatchItemRet*)calloc(len, sizeof(Efs_RS_BatchItemRet));
    Efs_Error error = Efs_RS_BatchCopy(pClient, itemRets, entryPathpair1, len);
    if (200 != error.code)
    {
        printf("copy files error.\n");
        debuginfo(pClient, error);
    }
    else
    {
        printf("copy files success.\n");
    }
    Efs_Free(itemRets);
}

void demoBatchDeleteFiles(Efs_Client* pClient, const char* bucketName)
{
    Efs_RS_EntryPath entryPath1[] = {
                                        {bucketName, "1_copy.txt"},
                                        {bucketName, "2_copy.txt"},
                                        {bucketName, "3_copy.txt"},
                                        {bucketName, "4_copy.txt"},
                                        {bucketName, "5_copy.txt"},
                                      };
    int len = sizeof(entryPath1)/sizeof(Efs_RS_EntryPath);
     Efs_RS_BatchItemRet* itemRets = (Efs_RS_BatchItemRet*)calloc(len, sizeof(Efs_RS_BatchItemRet));
    Efs_Error error = Efs_RS_BatchDelete(pClient, itemRets, entryPath1, len);
    if (200 != error.code)
    {
        printf("delete files error.\n");
        debuginfo(pClient, error);
    }
    else
    {
        printf("delete files success.\n");
    }
    Efs_Free(itemRets);
}


void demoUploadFile(Efs_Client* pClient, const char* bucketName, Efs_Mac* mac)
{
    const char* uploadName = "testUpload1.hpp";
    /*得到uploadKey*/
    const char* uploadtoken = upLoadToken(bucketName, mac);

    const char* pLocalFilePath = "C:\\3rdLib\\operators.hpp";
    
    Efs_Io_PutRet putRet;
    Efs_Error error = Efs_Io_PutFile(pClient, &putRet, uploadtoken, uploadName, pLocalFilePath, NULL);
    if (error.code != 200) 
    {
        printf("Upload File %s To %s:%s error.\n", pLocalFilePath, bucketName,  uploadName);
        debuginfo(pClient, error);
    }
    else
    {
        printf("Upload File %s To %s:%s success.\n", pLocalFilePath, bucketName,  uploadName);
    }

    Efs_Free((void *)uploadtoken);
}


void demoGetDownloadURL(const char* bucketName, Efs_Mac* mac)
{
    char* domain = Efs_String_Concat2(bucketName, ".u.efsdn.com");
    const char* downloadName = "testUpload1.hpp";
    char* pUrl = downloadUrl(domain, downloadName, mac);

    if (0 == pUrl)
    {
        printf("get URL %s:%s error.\n", bucketName, downloadName);
    }
    else 
    {
        printf("get URL %s:%s is %s.\n", bucketName, downloadName, pUrl);
    }

    Efs_Free(pUrl);
    Efs_Free(domain);
}

int main(int argc, char** argv)
{

    Efs_Client client;
    Efs_Mac    mac;
    char* bucketName = "efs-demo-test";

    mac.accessKey = argv[1];
    mac.secretKey = argv[2];
    // 初始化
    Efs_Servend_Init(-1);
    Efs_Client_InitMacAuth(&client, 1024, &mac);

    /* 此方法展示如何得到七牛云存储的一个文件的信息*/
    //demoGetFileStat(&client, bucketName, "a.txt");
    /* 此方法展示如何更改七牛云存储的一个文件的名称*/
    //demoMoveFile(&client, bucketName, "a.txt", "b.txt");
    /* 此方法展示如何复制七牛云存储的一个文件*/
    //demoCopyFile(&client, bucketName, "a.txt", "a_back.txt");
    /* 此方法展示如何删除七牛云存储的一个文件*/
    //demoDeleteFile(&client, bucketName, "a_back.txt");
    /* 此方法展示如何批量的得到七牛云存储文件的信息*/
    //demoBatchStatFiles(&client, bucketName);
    /* 此方法展示如何批量复制七牛云存储文件*/
    //demoBatchCopyFiles(&client, bucketName);
    /* 此方法展示如何批量删除七牛云存储文件*/
    //demoBatchDeleteFiles(&client, bucketName);

    /* 此方法展示如何上传一个本地文件到服务器*/
    //demoUploadFile(&client, bucketName, &mac);
    /*此方法展示如何得到一个服务器上的文件的，下载url*/
    //demoGetDownloadURL(bucketName, &mac);

    // 反初始化
    Efs_Client_Cleanup(&client);
    Efs_Servend_Cleanup();
    return 0;
}

// 
// -----------------------------------------------------------------------------
