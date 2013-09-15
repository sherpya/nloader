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

#ifndef _NTDLL_H
#define _NTDLL_H

#ifdef _MSC_VER
#define stat  _stati64
#define fstat _fstati64
typedef long long _off_t;

#define _OFF_T_DEFINED
#include <sys/stat.h>
#include <sys/types.h>

#endif

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef THREADED
#include <pthread.h>
#endif

typedef enum _EVENT_TYPE {
    NotificationEvent,
    SynchronizationEvent
} EVENT_TYPE, *PEVENT_TYPE;

struct _HANDLE;
typedef struct _HANDLE
{
    int  cookie;
    char type;
    union
    {
        struct
        {
#ifdef _WIN32
            struct _HANDLE *fh;
#else
            int fh;
#endif
            int mode;
            struct stat st;
        } file;

        struct
        {
            EVENT_TYPE type;
            int signaled;
#ifdef THREADED
            pthread_mutex_t state_mutex;
            pthread_mutex_t cond_mutex;
            pthread_cond_t cond;
#endif
        } event;

        struct
        {
#ifdef THREADED
            pthread_t tid;
#endif
            int ExitStatus;
            void *StartAddress;
            void *StartParameter;
        } thread;

        struct
        {
            void *ProcessHandle;
            int DesiredAccess;
        } token;

        void *data;
    };

    char    desc[512];
    char    name[512];

} *HANDLE, **PHANDLE;

#define HANDLE_COOKIE 0xfafadada

#define __CreateHandle(_handle, _type, _name)                                   \
    do                                                                          \
    {                                                                           \
        (_handle) = RtlAllocateHeap(HANDLE_HEAP, HEAP_ZERO_MEMORY, sizeof(struct _HANDLE));    \
        (_handle)->cookie = HANDLE_COOKIE;                                      \
        (_handle)->type = _type;                                                \
        strncpy((_handle)->name, _name, sizeof((_handle)->name));               \
        snprintf((_handle)->desc, sizeof((_handle)->desc), "%c:%s (by %s)",     \
        _type, _name, __func__);                                                \
    } while (0)

#define HANDLE_FILE 'F'
#define HANDLE_LINK 'L'
#define HANDLE_DIR  'D'
#define HANDLE_REG  'R'
#define HANDLE_EV   'E'
#define HANDLE_TH   'T'
#define HANDLE_TOK  'K'

#define HANDLE_DEFINED
#include "../../nt_structs.h"
#include "winternl.h"
#ifdef _WIN32
#include "ftable.h"
#endif

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

#define widetoa(x)      (x & 0xff)
#define widetol(x)      tolower(widetoa(x))
#define widetou(x)      toupper(widetoa(x))
#define wcmp(a, b, ci)  (ci ? widetol(a) == widetol(b) : a == b)
#define wtoi(x)         (widetoa(x) - '0')

#define _CONVBUF 1024

#define DECLAREVARCONV(var)     char var[_CONVBUF] = "(nil)"
#define WSTR2STREX(src, dest)   RtlUnicodeToOemN(dest, _CONVBUF, NULL, src, rpl_wcslen(src) * sizeof(WCHAR))
#define WSTR2STR(var)           WSTR2STREX(var, var ## A)
#define US2STREX(src, dest)     RtlUnicodeToOemN(dest, _CONVBUF, NULL, src->Buffer, src->Length)
#define US2STR(var)             US2STREX(var, var ## A)
#define OA2STREX(src, dest)     RtlUnicodeToOemN(dest, _CONVBUF, NULL, src->ObjectName->Buffer, src->ObjectName->Length)
#define OA2STR(var)             OA2STREX(var, var ## A)

static inline void spam_wchar(const WCHAR *str)
{
    fprintf(stderr, "\n--@%p--->", str);

    if (str)
        while (*str) fputc(*str++ & 0xff, stderr);
    else
        fprintf(stderr, "(null)");

    fprintf(stderr, "<---\n");
}

static inline void munge(char *text)
{
    char *e = text;
    while (*e)
    {
        if ((*e == '\r') || (*e == '\n'))
            *e = '.';
        e++;
    }
}

/* consts */
//#define dirinfo_name    L"Volume{12345678-1234-11dd-1234-123456789012}"
#define dirinfo_name    L"Volume{X}"
#define dirinfo_type    L"SymbolicLink"
#define version_sp2     L"Service Pack 2"
#define systemroot      L"c:\\windows"
#define systempartition L"\\Device\\Harddisk1"
#define devicelink      systempartition

#define GENERIC_READ    0x80000000
#define GENERIC_WRITE   0x40000000
#define GENERIC_EXECUTE 0x20000000
#define GENERIC_ALL     0x10000000

#define CREATE_NEW          1
#define CREATE_ALWAYS       2
#define OPEN_EXISTING       3
#define OPEN_ALWAYS         4
#define TRUNCATE_EXISTING   5

#define FILE_USE_FILE_POINTER_POSITION  0xfffffffffffffffeLL

#define NORMALIZE(x,addr)   {if(x) x=(PVOID)((ULONG_PTR)(x)+(ULONG_PTR)(addr));}
#define RTL_USER_PROCESS_PARAMETERS_NORMALIZED 0x01

#define PAGE_NOACCESS           0x01
#define PAGE_READONLY           0x02
#define PAGE_READWRITE          0x04
#define PAGE_WRITECOPY          0x08
#define PAGE_EXECUTE            0x10
#define PAGE_EXECUTE_READ       0x20
#define PAGE_EXECUTE_READWRITE  0x40
#define PAGE_EXECUTE_WRITECOPY  0x80

#define PAGE_GUARD              0x100
#define PAGE_NOCACHE            0x200
#define PAGE_WRITECOMBINE       0x400

#define MEM_COMMIT              0x00001000
#define MEM_RESERVE             0x00002000
#define MEM_DECOMMIT            0x00004000
#define MEM_RELEASE             0x00008000
#define MEM_FREE                0x00010000
#define MEM_PRIVATE             0x00020000
#define MEM_MAPPED              0x00040000
#define MEM_RESET               0x00080000
#define MEM_TOP_DOWN            0x00100000
#define MEM_WRITE_WATCH         0x00200000
#define MEM_PHYSICAL            0x00400000
#define MEM_LARGE_PAGES         0x20000000
#define MEM_4MB_PAGES           0x80000000

typedef enum _SHUTDOWN_ACTION
{
    ShutdownNoReboot,
    ShutdownReboot,
    ShutdownPowerOff
} SHUTDOWN_ACTION;

typedef DWORD (NTAPI *ThreadProc)(LPVOID);

/* heap */
PVOID NTAPI RtlAllocateHeap(PVOID HeapHandle, ULONG Flags, SIZE_T Size);
PVOID NTAPI RtlReAllocateHeap(PVOID HeapHandle, ULONG Flags, PVOID MemoryPointer, SIZE_T Size);
SIZE_T NTAPI RtlSizeHeap(PVOID HeapHandle, ULONG Flags, LPCVOID MemoryPointer);
BOOLEAN NTAPI RtlFreeHeap(PVOID HeapHandle, ULONG Flags, PVOID MemoryPointer);

/* crt */
SIZE_T CDECL rpl_wcslen(LPCWSTR str);
LPWSTR CDECL rpl_wcscat(LPWSTR dest, LPCWSTR src);

/* rtl */
#define TICKSPERSEC         10000000ULL
#define TICKS_1601_TO_1970  116444736000000000LL
NTSTATUS NTAPI RtlUnicodeToOemN(PCHAR OemString, ULONG MaxBytesInOemString, PULONG BytesInOemString, PCWCH UnicodeString, ULONG BytesInUnicodeString);
VOID NTAPI RtlInitUnicodeString(PUNICODE_STRING DestinationString, LPCWSTR SourceString);
VOID NTAPI RtlSecondsSince1970ToTime(DWORD Seconds, LARGE_INTEGER *Time);
VOID NTAPI RtlFillMemoryUlong(PVOID Destination, ULONG Length, ULONG Fill);

/* time.c */
NTSTATUS NTAPI NtQuerySystemTime(PLARGE_INTEGER Time);

/* str */
char *strhandle(HANDLE Handle);
const char *strctl(ULONG ControlCode);
const char *strsysinfo(SYSTEM_INFORMATION_CLASS infoclass);
const char *strfileinfo(FILE_INFORMATION_CLASS infoclass);
const char *strfsinfo(FS_INFORMATION_CLASS infoclass);
const char *strthinfo(THREADINFOCLASS infoclass);
const char *strspqid(STORAGE_PROPERTY_ID PropertyId);
const char *strspqtype(STORAGE_QUERY_TYPE QueryType);

/* resources */
#define IMAGE_RESOURCE_NAME_IS_STRING       0x80000000
#define IMAGE_RESOURCE_DATA_IS_DIRECTORY    0x80000000

typedef struct _IMAGE_RESOURCE_DIRECTORY_ENTRY {
    union {
        struct {
            unsigned NameOffset:31;
            unsigned NameIsString:1;
        };
        DWORD Name;
        struct {
            WORD Id;
            WORD __pad;
        };
    };
    union {
        DWORD OffsetToData;
        struct {
            unsigned OffsetToDirectory:31;
            unsigned DataIsDirectory:1;
        };
    };
} IMAGE_RESOURCE_DIRECTORY_ENTRY,*PIMAGE_RESOURCE_DIRECTORY_ENTRY;

typedef struct _IMAGE_RESOURCE_DATA_ENTRY {
    DWORD OffsetToData;
    DWORD Size;
    DWORD CodePage;
    DWORD Reserved;
} IMAGE_RESOURCE_DATA_ENTRY,*PIMAGE_RESOURCE_DATA_ENTRY;

typedef struct _IMAGE_RESOURCE_DIRECTORY {
    DWORD Characteristics;
    DWORD TimeDateStamp;
    WORD  MajorVersion;
    WORD  MinorVersion;
    WORD  NumberOfNamedEntries;
    WORD  NumberOfIdEntries;
    IMAGE_RESOURCE_DIRECTORY_ENTRY DirectoryEntries[1];
} IMAGE_RESOURCE_DIRECTORY,*PIMAGE_RESOURCE_DIRECTORY;

typedef struct tagMESSAGE_RESOURCE_ENTRY
{
    WORD Length;
    WORD Flags;
    BYTE Text[1];
} MESSAGE_RESOURCE_ENTRY,*PMESSAGE_RESOURCE_ENTRY;
#define MESSAGE_RESOURCE_UNICODE 0x0001

typedef struct tagMESSAGE_RESOURCE_BLOCK {
    DWORD LowId;
    DWORD HighId;
    DWORD OffsetToEntries;
} MESSAGE_RESOURCE_BLOCK,*PMESSAGE_RESOURCE_BLOCK;

typedef struct tagMESSAGE_RESOURCE_DATA {
    DWORD NumberOfBlocks;
    MESSAGE_RESOURCE_BLOCK Blocks[1];
} MESSAGE_RESOURCE_DATA,*PMESSAGE_RESOURCE_DATA;

#define KEY_MAKE    0
#define KEY_BREAK   1

typedef struct tagKEYBOARD_INPUT_DATA {
    USHORT UnitId;
    USHORT MakeCode;
    USHORT Flags;
    USHORT Reserved;
    ULONG ExtraInformation;
} KEYBOARD_INPUT_DATA, *PKEYBOARD_INPUT_DATA;

#endif /* _NTDLL_H */
