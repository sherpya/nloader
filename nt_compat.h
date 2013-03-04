/*
 * Copyright (c) 2010, Sherpya <sherpya@netfarm.it>, aCaB <acab@0xacab.net>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the copyright holders nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __NT_COMPAT_H
#define __NT_COMPAT_H

#ifdef WIN32
#include <io.h>
#include <ctype.h>
#include <sys/timeb.h>

#define __func__    __FUNCTION__

#ifndef S_ISDIR
#define S_ISDIR(m)      (((m) & S_IFMT) == S_IFDIR)
#endif

#ifndef S_IWUSR
#define S_IWUSR         S_IWRITE
#endif

typedef struct _SYSTEM_INFO
{
    DWORD d1;
    DWORD dwPageSize;
    BYTE padding[(sizeof(PVOID) * 2) + (sizeof(DWORD) * 4) + (sizeof(WORD) * 2)];
} SYSTEM_INFO, *LPSYSTEM_INFO;

DECLSPEC_IMPORT LPVOID WINAPI LoadLibraryA(LPCSTR);
DECLSPEC_IMPORT HANDLE WINAPI GetModuleHandleA(LPCSTR);
DECLSPEC_IMPORT FARPROC WINAPI GetProcAddress(LPVOID, LPCSTR);
DECLSPEC_IMPORT DWORD WINAPI GetLastError(VOID);
DECLSPEC_IMPORT VOID WINAPI SetLastError(DWORD);
DECLSPEC_IMPORT PVOID WINAPI VirtualAlloc(PVOID, DWORD, DWORD, DWORD);
DECLSPEC_IMPORT int WINAPI VirtualFree(PVOID, DWORD, DWORD);
DECLSPEC_IMPORT int WINAPI VirtualProtect(PVOID, DWORD, DWORD, PDWORD);
DECLSPEC_IMPORT VOID WINAPI GetSystemInfo(LPSYSTEM_INFO);
DECLSPEC_IMPORT DWORD WINAPI GetTickCount(VOID);

DECLSPEC_IMPORT HANDLE WINAPI CreateFileA(LPCSTR,DWORD,DWORD,LPVOID,DWORD,DWORD,HANDLE);
DECLSPEC_IMPORT HANDLE WINAPI CreateFileW(LPWSTR,DWORD,DWORD,LPVOID,DWORD,DWORD,HANDLE);
DECLSPEC_IMPORT BOOL WINAPI GetFileSizeEx(HANDLE,PLARGE_INTEGER);
DECLSPEC_IMPORT DWORD WINAPI GetLastError(VOID);
DECLSPEC_IMPORT BOOL WINAPI SetFilePointerEx(HANDLE,LARGE_INTEGER,PLARGE_INTEGER,DWORD);
DECLSPEC_IMPORT BOOL WINAPI ReadFile(HANDLE,LPVOID,DWORD,PDWORD,LPVOID);
DECLSPEC_IMPORT BOOL WINAPI WriteFile(HANDLE,LPCVOID,DWORD,PDWORD,LPVOID);
DECLSPEC_IMPORT BOOL WINAPI CloseHandle(HANDLE);
DECLSPEC_IMPORT BOOL WINAPI FlushFileBuffers(HANDLE);

#define IsValidHandle(handle)   (handle != (HANDLE) -1)
#define FlushFile(handle)       FlushFileBuffers(handle)

struct timeval
{
    long tv_sec;          /* seconds */
    long tv_usec;    /* microseconds */
};

static inline int gettimeofday(struct timeval *tv, void *__restrict tzp)
{
    if (tv)
    {
        struct timeb timebuffer;
        ftime(&timebuffer);
        tv->tv_sec = (long) timebuffer.time;
        tv->tv_usec = 1000 * timebuffer.millitm;
    }

    return 0;
}

static inline int getpagesize(void)
{
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return si.dwPageSize;
}

static inline int uptime(void)
{
    return GetTickCount() * 1000;
}

static inline char *strcasestr(const char *haystack, const char *needle)
{
    char c, sc;
    size_t len;

    if ((c = *needle++) != 0)
    {
        c = tolower((unsigned char)c);
        len = strlen(needle);
        do
        {
            do
            {
                if (!(sc = *haystack++))
                    return NULL;
            } while ((char) tolower((unsigned char) sc) != c);
        } while (_strnicmp(haystack, needle, len));
    haystack--;
    }
    return ((char *) haystack);
}


static inline void *load_library(const char *dllname)
{
    char libpath[260];

    if (!dllname)
        return GetModuleHandleA(NULL);

#ifdef _MSC_VER
    _snprintf(libpath, sizeof(libpath) - 1, ".\\%s", dllname);
    return LoadLibraryA(libpath);
#else
    snprintf(libpath, sizeof(libpath) - 1, "libs\\%s.so", dllname);
    return LoadLibraryA(libpath);
#endif
}


#define dlerror()           (GetLastError() == 127)
#define dlsym(handle, sym)  (SetLastError(0), (FARPROC) GetProcAddress(handle, sym))

#define perror(text)        printf("Error - %s: %d\n", text, GetLastError())
#define strcasecmp          stricmp
#define strncasecmp         strnicmp
#define snprintf            _snprintf

#define S_IWGRP             0
#define S_IWOTH             0

#define PROT_READ           0x1
#define PROT_WRITE          0x2
#define PROT_EXEC           0x4
#define PROT_NONE           0x0

#define MAP_PRIVATE         0
#define MAP_ANON	        0
#define MAP_FIXED           0

#define MAP_FAILED          NULL

#define PAGE_READONLY           0x02
#define PAGE_READWRITE          0x04
#define PAGE_EXECUTE_READ       0x20
#define PAGE_EXECUTE_READWRITE  0x40

static inline DWORD memprot(int prot)
{
    if (prot & PROT_EXEC)
    {
        if (prot & PROT_WRITE)
            return PAGE_EXECUTE_READWRITE;
        else
            return PAGE_EXECUTE_READ;
    }

    if (prot & PROT_WRITE)
        return PAGE_READWRITE;

    return PAGE_READONLY;
}

#define mmap(addr, length, prot, flags, fd, offset) VirtualAlloc(addr, length, 0x3000, memprot(prot))
#define munmap(addr, length) (VirtualFree(addr, length, 0x8000) ? 0 : -1)

static inline int mprotect(void *addr, size_t len, int prot)
{
    DWORD dummy;
    return VirtualProtect(addr, len, memprot(prot), &dummy) ? 0 : -1;
}

#else
#include <unistd.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include <time.h>

#if defined(__linux)
#include <asm/ldt.h>
#include <sys/sysinfo.h>
#elif defined(__FreeBSD__)
#include <machine/segments.h>
#include <machine/sysarch.h>
#elif defined(__APPLE__)
#include <i386/user_ldt.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

#ifndef RTLD_DEEPBIND
#define RTLD_DEEPBIND 0
#endif

#ifndef LIBNLOADER
#define LIBNLOADER "/usr/lib/nloader"
#endif

static inline void *load_library(const char *dllname)
{
    char libpath[512];
    void *handle;

    if (!dllname)
        return dlopen(NULL, RTLD_NOW|RTLD_LOCAL|RTLD_DEEPBIND);

    snprintf(libpath, sizeof(libpath) - 1, LIBNLOADER"/%s.so", dllname);
    handle = dlopen(libpath, RTLD_NOW|RTLD_LOCAL|RTLD_DEEPBIND);

    if (handle)
        return handle;

    snprintf(libpath, sizeof(libpath) - 1, "libs/%s.so", dllname);
    return dlopen(libpath, RTLD_NOW|RTLD_LOCAL|RTLD_DEEPBIND);
}

struct modify_ldt_s
{
    unsigned int  entry_number;
    unsigned long base_addr;
    unsigned int  limit;
    unsigned int  seg_32bit:1;
    unsigned int  contents:2;
    unsigned int  read_exec_only:1;
    unsigned int  limit_in_pages:1;
    unsigned int  seg_not_present:1;
    unsigned int  useable:1;
};

#define LDT_SEL(idx) ((idx) << 3 | 1 << 2 | 3)

#if defined(__linux)
extern int modify_ldt(int func, struct modify_ldt_s *fs_ldt, unsigned long bytecount);
#define TEB_SEL_IDX 17

static inline int uptime(void)
{
    struct sysinfo info;
    if (sysinfo(&info) != -1)
        return info.uptime;
    return 0;
}

#elif defined(__APPLE__) || defined(__FreeBSD__)
// old wine loader in mplayer

#define TEB_SEL_IDX LDT_AUTO_ALLOC

static inline void LDT_EntryToBytes(unsigned long *buffer, const struct modify_ldt_s *content)
{
    *buffer++ = ((content->base_addr & 0x0000ffff) << 16) | (content->limit & 0x0ffff);

    *buffer = (content->base_addr & 0xff000000) |
    ((content->base_addr & 0x00ff0000)>>16) |
    (content->limit & 0xf0000) |
    (content->contents << 10) |
    ((content->read_exec_only == 0) << 9) |
    ((content->seg_32bit != 0) << 22) |
    ((content->limit_in_pages != 0) << 23) |
    0xf000;
}

static inline int modify_ldt(int func, struct modify_ldt_s *fs_ldt, unsigned long bytecount)
{
    int ret;
    unsigned long entry[2];
    LDT_EntryToBytes(entry, fs_ldt);
    ret = i386_set_ldt(fs_ldt->entry_number, (void *) entry, 1);
    return fs_ldt->entry_number = ret;
}

static inline int uptime(void)
{

#if defined(__FreeBSD__)
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != -1)
        return ts.tv_sec;
#elif defined( __APPLE__)
    struct timeval uptime;
    size_t len = sizeof(uptime);
    int mib[2] = { CTL_KERN, KERN_BOOTTIME };
    if (sysctl(mib, 2, &uptime, &len, NULL, 0) == 0)
        return time(NULL) - uptime.tv_sec;
#endif
    return 0;
}

#define MODIFY_LDT_CONTENTS_DATA 0
#else
# error Mboh?
#endif

#define O_BINARY                0
#define IsValidHandle(handle)   (handle != -1)
#define FlushFile(handle)       (fsync(handle) == 0)


#endif

#endif /* __NT_COMPAT_H */
