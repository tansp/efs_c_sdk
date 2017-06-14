/*
 ============================================================================
 Name        : conf.h
 Author      : gosun.com
 Copyright   : 2017(c) beijing efs Information Technologies Co., Ltd.
 Description : 
 ============================================================================
 */

#ifndef EFS_MACRO_H
#define EFS_MACRO_H

#if defined(USING_EFS_LIBRARY_DLL)
    #define EFS_DLLAPI __declspec(dllimport)
#elif defined(COMPILING_EFS_LIBRARY_DLL)
    #define EFS_DLLAPI __declspec(dllexport)
#else
    #define EFS_DLLAPI 
#endif

#endif
