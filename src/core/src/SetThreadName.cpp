//
//  SetThreadName.cpp
//  mush-core
//
//  Created by Josh McNamee on 04/10/2016.
//  Copyright © 2016 josh04. All rights reserved.
//

#include "SetThreadName.hpp"

#ifdef _WIN32
#include "windows.h"
const DWORD MS_VC_EXCEPTION = 0x406D1388;

//#define _CRTDBG_MAP_ALLOC
//#include <stdlib.h>
//#include <crtdbg.h>

#pragma pack(push,8)
typedef struct tagTHREADNAME_INFO
{
    DWORD dwType; // Must be 0x1000.
    LPCSTR szName; // Pointer to name (in user addr space).
    DWORD dwThreadID; // Thread ID (-1=caller thread).
    DWORD dwFlags; // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)

void SetThreadName(const char * threadName)
{
    DWORD dwThreadID = -1;
    THREADNAME_INFO info;
    info.dwType = 0x1000;
    info.szName = threadName;
    info.dwThreadID = dwThreadID;
    info.dwFlags = 0;
    
    __try
    {
        RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*) &info);
    } __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
}

#endif

#ifdef __APPLE__
#include <pthread.h>
void SetThreadName(const char * threadName)
{
    pthread_setname_np(threadName);
}
#endif
