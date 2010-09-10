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

#ifndef __NT_STRUCTS_H
#define __NT_STRUCTS_H

#if !defined(_WIN32) && (__WCHAR_MAX__ != 65535U)
# error compile with -fshort-wchar
#endif

#if !defined(_WIN32) && (defined(REDIR_IO) || defined(REDIR_SYSCALL))
#error Redir only available on win32
#endif

#include <stdint.h>

#ifdef __GNUC__
#define ATTRIBUTE_USED          __attribute__((used))
#define ATTRIBUTE_ALIGNED(x)    __attribute__((aligned(x)))
#define BPX() { if (getenv("BPX")) __asm__ volatile ("int $3"); }
#define INT3() __asm__ volatile ("int $3")
#else
#define ATTRIBUTE_USED
#define ATTRIBUTE_ALIGNED(x)
#define inline __inline
#define BPX() { if (getenv("BPX")) __asm { int 3 }; }
#define INT3() __asm { int 3 }
#endif

#ifdef __GNUC__
#define WINAPI __attribute__((stdcall))
#define CDECL  __attribute__((cdecl))
#else
#define WINAPI _stdcall
#define CDECL  _cdecl
#endif

#define APIENTRY WINAPI
#define NTAPI WINAPI

#define DECLSPEC_IMPORT __declspec(dllimport)
typedef int (WINAPI *FARPROC)(void);

#define FIELD_OFFSET(Type, Field)   ((LONG) (&(((Type *) 0)->Field)))
#define PAGE_SIZE                   getpagesize()
#define PAGE_ALIGN(Va)              ((PVOID) ((ULONG_PTR)(Va) & ~(PAGE_SIZE - 1)))
#define ROUND_TO_PAGES(Size)        ((ULONG_PTR) (((ULONG_PTR) Size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1)))

#define MAXLONG     0x7fffffff
#define MAXULONG    0xffffffff

#define VOLUME_PREFIX   L"Volume{"

#define Q(x)                #x

#if defined(_WIN32) || defined(__APPLE__)
#define MANGLE(func)        Q(_ ## func)
#else
#define MANGLE(func)        #func
#endif

#ifdef _WIN32 // via .def
#define FORWARD_FUNCTION(have, need)
#else
#define FORWARD_FUNCTION(have, need)            \
    asm(                                        \
    ".text                              \n\t"   \
    ".globl "Q(need)"                   \n\t"   \
    Q(need)":                           \n\t"   \
    "jmp "MANGLE(have)"")
#endif

#define IN
#define OUT
#define OPTIONAL

#define HANDLE_HEAP             (HANDLE)(0x1337)
#define INVALID_HANDLE_VALUE    (HANDLE)(-1)

#define NtCurrentProcess()      ((HANDLE)(LONG_PTR)-1)
#define NtCurrentThread()       ((HANDLE)(LONG_PTR)-2)

#define DLL_PROCESS_DETACH  0
#define DLL_PROCESS_ATTACH  1
#define DLL_THREAD_ATTACH   2
#define DLL_THREAD_DETACH   3

#define MODULE(name) static ATTRIBUTE_USED const char __module__[] = #name;
#define Log(format, ...) NtCurrentTeb()->LogModule(__module__, format, ##__VA_ARGS__)

typedef void (*LogModuleFunc)(const char *module, const char *format, ...);

#define TRUE  1
#define FALSE 0

struct pe_image_file_hdr {
    uint32_t Magic;
    uint16_t Machine;
    uint16_t NumberOfSections;
    uint32_t TimeDateStamp;
    uint32_t PointerToSymbolTable;
    uint32_t NumberOfSymbols;
    uint16_t SizeOfOptionalHeader;
    uint16_t Characteristics;
};


struct pe_image_optional_hdr32 {
    uint16_t Magic;
    uint8_t  MajorLinkerVersion;
    uint8_t  MinorLinkerVersion;
    uint32_t SizeOfCode;
    uint32_t SizeOfInitializedData;
    uint32_t SizeOfUninitializedData;
    uint32_t AddressOfEntryPoint;
    uint32_t BaseOfCode;
    uint32_t BaseOfData;
    uint32_t ImageBase;
    uint32_t SectionAlignment;
    uint32_t FileAlignment;
    uint16_t MajorOperatingSystemVersion;
    uint16_t MinorOperatingSystemVersion;
    uint16_t MajorImageVersion;
    uint16_t MinorImageVersion;
    uint16_t MajorSubsystemVersion;
    uint16_t MinorSubsystemVersion;
    uint32_t Win32VersionValue;
    uint32_t SizeOfImage;
    uint32_t SizeOfHeaders;
    uint32_t CheckSum;
    uint16_t Subsystem;
    uint16_t DllCharacteristics;
    uint32_t SizeOfStackReserve;
    uint32_t SizeOfStackCommit;
    uint32_t SizeOfHeapReserve;
    uint32_t SizeOfHeapCommit;
    uint32_t LoaderFlags;
    uint32_t NumberOfRvaAndSizes;
    struct pe_image_data_dir {
        uint32_t VirtualAddress;
	uint32_t Size;
    } DataDirectory[16];
};

struct pe_image_section_hdr {
    uint8_t Name[8];
    uint32_t VirtualSize;
    uint32_t VirtualAddress;
    uint32_t SizeOfRawData;
    uint32_t PointerToRawData;
    uint32_t PointerToRelocations;
    uint32_t PointerToLinenumbers;
    uint16_t NumberOfRelocations;
    uint16_t NumberOfLinenumbers;
    uint32_t Characteristics;
};

struct IMAGE_IMPORT {
    uint32_t OrigThunk;
    uint32_t Time;
    uint32_t Fwd;
    uint32_t DllName;
    uint32_t Thunk;
};

#define PEALIGN(o,a) (((a))?(((o)/(a))*(a)):(o))
#define PESALIGN(o,a) (((a))?(((o)/(a)+((o)%(a)!=0))*(a)):(o))

#define CLI_ISCONTAINED(bb, bb_size, sb, sb_size)       \
  ((bb_size) > 0 && (sb_size) > 0 && (size_t)(sb_size) <= (size_t)(bb_size) \
   && (sb) >= (bb) && ((sb) + (sb_size)) <= ((bb) + (bb_size)) && ((sb) + (sb_size)) > (bb) && (sb) < ((bb) + (bb_size)))


#define ROUND_DOWN(n, align) \
    (((ULONG)n) & ~((align) - 1l))

#define ROUND_UP(n, align) \
    ROUND_DOWN(((ULONG)n) + (align) - 1, (align))

/* --------------------------------------------------------------------------------------------------------------------------------
   Bunch of crap from WINE
   To be simplified/reduced ASAP!!!
   -------------------------------------------------------------------------------------------------------------------------------- */

typedef void VOID, *PVOID, *LPVOID;
typedef const void *LPCVOID;
#ifndef HANDLE_DEFINED
typedef PVOID HANDLE, *PHANDLE;
#endif

#ifdef __WCHAR_TYPE__
typedef __WCHAR_TYPE__ WCHAR;
#else
typedef unsigned short WCHAR;
#endif

typedef ATTRIBUTE_ALIGNED(8) int64_t INT64, *PINT64;
typedef ATTRIBUTE_ALIGNED(8) uint64_t UINT64, *PUINT64;

typedef intptr_t LONG_PTR;
typedef uintptr_t ULONG_PTR;

typedef ULONG_PTR SIZE_T, *PSIZE_T;
typedef LONG_PTR SSIZE_T, *PSSIZE_T;

typedef INT64 LONGLONG, *PLONGLONG;
typedef UINT64 ULONGLONG, *PULONGLONG;

typedef UINT64 DWORD64, *PDWORD64;

typedef uint16_t WORD, *PWORD;
typedef uint32_t DWORD, *PDWORD;
typedef uint32_t ULONG, *PULONG;
typedef int32_t LONG, *PLONG;
typedef uint16_t USHORT;

typedef uint8_t BYTE, *PBYTE;
typedef uint8_t UCHAR, *PUCHAR;
typedef BYTE BOOLEAN, *PBOOLEAN;
typedef int BOOL, *PBOOL;

typedef char CHAR, CCHAR, *PCCHAR;
typedef CHAR *PCHAR, *LPCH, *PCH, *NPSTR, *LPSTR, *PSTR;
typedef const CHAR *PCCH, *LPCSTR;

typedef WCHAR *PWCHAR, *LPWCH, *PWCH, *NWPSTR, *LPWSTR, *PWSTR, *PCWSTR;
typedef const WCHAR *PCWCH;
typedef PWSTR *PZPWSTR;
typedef const WCHAR *LPCWSTR;

typedef int32_t INT, *PINT;
typedef uint32_t UINT, *PUINT;

typedef ULONG_PTR HMODULE;
typedef PVOID HINSTANCE;
typedef PVOID HWND;

typedef union _LARGE_INTEGER {
    struct {
        DWORD    LowPart;
        LONG     HighPart;
    } u;
    LONGLONG QuadPart;
} LARGE_INTEGER, *PLARGE_INTEGER;

typedef union _ULARGE_INTEGER {
    struct {
        DWORD    LowPart;
        DWORD    HighPart;
    } u;
    ULONGLONG QuadPart;
} ULARGE_INTEGER, *PULARGE_INTEGER;

#define FILE_ATTRIBUTE_READONLY            0x00000001
#define FILE_ATTRIBUTE_HIDDEN              0x00000002
#define FILE_ATTRIBUTE_SYSTEM              0x00000004
#define FILE_ATTRIBUTE_DIRECTORY           0x00000010
#define FILE_ATTRIBUTE_ARCHIVE             0x00000020
#define FILE_ATTRIBUTE_DEVICE              0x00000040
#define FILE_ATTRIBUTE_NORMAL              0x00000080
#define FILE_ATTRIBUTE_TEMPORARY           0x00000100
#define FILE_ATTRIBUTE_SPARSE_FILE         0x00000200
#define FILE_ATTRIBUTE_REPARSE_POINT       0x00000400
#define FILE_ATTRIBUTE_COMPRESSED          0x00000800
#define FILE_ATTRIBUTE_OFFLINE             0x00001000
#define FILE_ATTRIBUTE_NOT_CONTENT_INDEXED 0x00002000
#define FILE_ATTRIBUTE_ENCRYPTED           0x00004000

typedef struct _GUID
{
    ULONG Data1;
    WORD Data2;
    WORD Data3;
    UCHAR Data4[8];
} GUID, *PGUID;

typedef struct _CLIENT_ID
{
   HANDLE UniqueProcess;
   HANDLE UniqueThread;
} CLIENT_ID, *PCLIENT_ID;

typedef struct _UNICODE_STRING {
  USHORT Length;
  USHORT MaximumLength;
  PWSTR  Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef const UNICODE_STRING *PCUNICODE_STRING;

typedef struct _STRING {
  USHORT Length;
  USHORT MaximumLength;
  PCHAR Buffer;
} STRING, *PSTRING;

typedef STRING ANSI_STRING;
typedef PSTRING PANSI_STRING;
typedef const ANSI_STRING *PCANSI_STRING;

typedef struct tagRTL_BITMAP {
    ULONG  SizeOfBitMap;
    PULONG Buffer;
} RTL_BITMAP, *PRTL_BITMAP;

typedef struct _RTL_BITMAP_RUN
{
    ULONG StartingIndex;
    ULONG NumberOfBits;
} RTL_BITMAP_RUN, *PRTL_BITMAP_RUN;

typedef struct RTL_DRIVE_LETTER_CURDIR
{
    USHORT              Flags;
    USHORT              Length;
    ULONG               TimeStamp;
    UNICODE_STRING      DosPath;
} RTL_DRIVE_LETTER_CURDIR, *PRTL_DRIVE_LETTER_CURDIR;

typedef struct _LIST_ENTRY {
  struct _LIST_ENTRY *Flink;
  struct _LIST_ENTRY *Blink;
} LIST_ENTRY, *PLIST_ENTRY;


#define InitializeListHead(le)  (void)((le)->Flink = (le)->Blink = (le))
#define InsertHeadList(le,e)    do { PLIST_ENTRY f = (le)->Flink; (e)->Flink = f; (e)->Blink = (le); f->Blink = (e); (le)->Flink = (e); } while (0)
#define InsertTailList(le,e)    do { PLIST_ENTRY b = (le)->Blink; (e)->Flink = (le); (e)->Blink = b; b->Flink = (e); (le)->Blink = (e); } while (0)
#define IsListEmpty(le)         ((le)->Flink == (le))
#define RemoveEntryList(e)      do { PLIST_ENTRY f = (e)->Flink, b = (e)->Blink; f->Blink = b; b->Flink = f; (e)->Flink = (e)->Blink = NULL; } while (0)

typedef struct _RTL_CRITICAL_SECTION_DEBUG
{
  WORD   Type;
  WORD   CreatorBackTraceIndex;
  struct _RTL_CRITICAL_SECTION *CriticalSection;
  LIST_ENTRY ProcessLocksList;
  DWORD EntryCount;
  DWORD ContentionCount;
  DWORD Spare[ 2 ];
} RTL_CRITICAL_SECTION_DEBUG, *PRTL_CRITICAL_SECTION_DEBUG, RTL_RESOURCE_DEBUG, *PRTL_RESOURCE_DEBUG;


typedef struct _RTL_CRITICAL_SECTION {
    PRTL_CRITICAL_SECTION_DEBUG DebugInfo;
    LONG LockCount;
    LONG RecursionCount;
    HANDLE OwningThread;
    HANDLE LockSemaphore;
    ULONG_PTR SpinCount;
}  RTL_CRITICAL_SECTION, *PRTL_CRITICAL_SECTION;


typedef struct _CURDIR
{
    UNICODE_STRING DosPath;
    PVOID Handle;
} CURDIR, *PCURDIR;


typedef struct _PEB_LDR_DATA
{
    ULONG               Length;
    BOOLEAN             Initialized;
    PVOID               SsHandle;
    LIST_ENTRY          InLoadOrderModuleList;
    LIST_ENTRY          InMemoryOrderModuleList;
    LIST_ENTRY          InInitializationOrderModuleList;
} PEB_LDR_DATA, *PPEB_LDR_DATA;

typedef struct _RTL_ACTIVATION_CONTEXT_STACK_FRAME
{
    struct _RTL_ACTIVATION_CONTEXT_STACK_FRAME *Previous;
    struct _ACTIVATION_CONTEXT                 *ActivationContext;
    ULONG                                       Flags;
} RTL_ACTIVATION_CONTEXT_STACK_FRAME, *PRTL_ACTIVATION_CONTEXT_STACK_FRAME;

typedef struct _ACTIVATION_CONTEXT_STACK
{
    ULONG                               Flags;
    ULONG                               NextCookieSequenceNumber;
    RTL_ACTIVATION_CONTEXT_STACK_FRAME *ActiveFrame;
    LIST_ENTRY                          FrameListCache;
} ACTIVATION_CONTEXT_STACK, *PACTIVATION_CONTEXT_STACK;

typedef struct _GDI_TEB_BATCH
{
    ULONG  Offset;
    ULONG  HDC;
    ULONG  Buffer[310];
} GDI_TEB_BATCH;

typedef struct _KSYSTEM_TIME
{
     ULONG LowPart;
     LONG High1Time;
     LONG High2Time;
} KSYSTEM_TIME, *PKSYSTEM_TIME;

typedef enum _NT_PRODUCT_TYPE
{
    NtProductWinNt      = 1,
    NtProductLanManNt   = 2,
    NtProductServer     = 3
} NT_PRODUCT_TYPE;

typedef enum _ALTERNATIVE_ARCHITECTURE_TYPE
{
    StandardDesign = 0,
    NEC98x86 = 1,
    EndAlternatives = 2
} ALTERNATIVE_ARCHITECTURE_TYPE;

typedef struct _KUSER_SHARED_DATA
{
    ULONG TickCountLowDeprecated;
    ULONG TickCountMultiplier;
    volatile KSYSTEM_TIME InterruptTime;
    volatile KSYSTEM_TIME SystemTime;
    volatile KSYSTEM_TIME TimeZoneBias;
    WORD ImageNumberLow;
    WORD ImageNumberHigh;
    WCHAR NtSystemRoot[260];
    ULONG MaxStackTraceDepth;
    ULONG CryptoExponent;
    ULONG TimeZoneId;
    ULONG LargePageMinimum;
    ULONG Reserved2[7];
    NT_PRODUCT_TYPE NtProductType;
    UCHAR ProductTypeIsValid;
    ULONG NtMajorVersion;
    ULONG NtMinorVersion;
    UCHAR ProcessorFeatures[64];
    ULONG Reserved1;
    ULONG Reserved3;
    ULONG TimeSlip;
    ALTERNATIVE_ARCHITECTURE_TYPE AlternativeArchitecture;
    LARGE_INTEGER SystemExpirationDate;
    ULONG SuiteMask;
    UCHAR KdDebuggerEnabled;
    ULONG ActiveConsoleId;
    ULONG DismountCount;
    ULONG ComPlusPackage;
    ULONG LastSystemRITEventTickCount;
    ULONG NumberOfPhysicalPages;
    UCHAR SafeBootMode;
    ULONG TraceLogging;
    UINT64 TestRetInstruction;
    ULONG SystemCall;
    ULONG SystemCallReturn;
    UINT64 SystemCallPad[3];
    union
    {
        struct _KSYSTEM_TIME TickCount;
        UINT64 TickCountQuad;
    };
    ULONG   Cookie;
    ULONG Wow64SharedInformation[16];
} KUSER_SHARED_DATA, *PKUSER_SHARED_DATA;
#define USER_SHARED_DATA    0x7ffe0000

#define PARAMS_ALREADY_NORMALIZED 1
typedef struct _RTL_USER_PROCESS_PARAMETERS
{
    ULONG               AllocationSize;
    ULONG               Size;
    ULONG               Flags;
    ULONG               DebugFlags;
    HANDLE              ConsoleHandle;
    ULONG               ConsoleFlags;
    HANDLE              hStdInput;
    HANDLE              hStdOutput;
    HANDLE              hStdError;
    CURDIR              CurrentDirectory;
    UNICODE_STRING      DllPath;
    UNICODE_STRING      ImagePathName;
    UNICODE_STRING      CommandLine;
    PWSTR               Environment;
    ULONG               dwX;
    ULONG               dwY;
    ULONG               dwXSize;
    ULONG               dwYSize;
    ULONG               dwXCountChars;
    ULONG               dwYCountChars;
    ULONG               dwFillAttribute;
    ULONG               dwFlags;
    ULONG               wShowWindow;
    UNICODE_STRING      WindowTitle;
    UNICODE_STRING      Desktop;
    UNICODE_STRING      ShellInfo;
    UNICODE_STRING      RuntimeInfo;
    RTL_DRIVE_LETTER_CURDIR DLCurrentDirectory[32];
} RTL_USER_PROCESS_PARAMETERS, *PRTL_USER_PROCESS_PARAMETERS;

typedef struct _PEB
{
    BOOLEAN                      InheritedAddressSpace;
    BOOLEAN                      ReadImageFileExecOptions;
    BOOLEAN                      BeingDebugged;
    union
    {
        BYTE BitField;
        struct
        {
            BOOLEAN ImageUsesLargePages:1;
            BOOLEAN SpareBits:7;
        };
    };
    HANDLE                       Mutant;
    HMODULE                      ImageBaseAddress;
    PPEB_LDR_DATA                LdrData;
    PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
    PVOID                        SubSystemData;
    HANDLE                       ProcessHeap;
    PRTL_CRITICAL_SECTION        FastPebLock;
    PVOID                        AtlThunkSListPtr;
    PVOID                        SparePtr2;
    ULONG                        EnvironmentUpdateCount;
    PVOID                        KernelCallbackTable;
    ULONG                        Reserved[2];
    PVOID /*PPEB_FREE_BLOCK*/    FreeList;
    ULONG                        TlsExpansionCounter;
    PRTL_BITMAP                  TlsBitmap;
    ULONG                        TlsBitmapBits[2];

    PVOID                        ReadOnlySharedMemoryBase;
    PVOID                        ReadOnlySharedMemoryHeap;
    PVOID                       *ReadOnlyStaticServerData;
    PVOID                        AnsiCodePageData;
    PVOID                        OemCodePageData;
    PVOID                        UnicodeCaseTableData;
    ULONG                        NumberOfProcessors;
    ULONG                        NtGlobalFlag;
    LARGE_INTEGER                CriticalSectionTimeout;
    SIZE_T                       HeapSegmentReserve;
    SIZE_T                       HeapSegmentCommit;
    SIZE_T                       HeapDeCommitTotalFreeThreshold;
    SIZE_T                       HeapDeCommitFreeBlockThreshold;
    ULONG                        NumberOfHeaps;
    ULONG                        MaximumNumberOfHeaps;
    PVOID                       *ProcessHeaps;
    PVOID                        GdiSharedHandleTable;
    PVOID                        ProcessStarterHelper;
    PVOID                        GdiDCAttributeList;
    PRTL_CRITICAL_SECTION        LoaderLock;
    ULONG                        OSMajorVersion;
    ULONG                        OSMinorVersion;
    USHORT                       OSBuildNumber;
    USHORT                       OSCSDVersion;
    ULONG                        OSPlatformId;
    ULONG                        ImageSubSystem;
    ULONG                        ImageSubSystemMajorVersion;
    ULONG                        ImageSubSystemMinorVersion;
    ULONG_PTR                    ImageProcessAffinityMask;
    HANDLE                       GdiHandleBuffer[34];
    PVOID                        PostProcessInitRoutine;
    PRTL_BITMAP                  TlsExpansionBitmap;
    ULONG                        TlsExpansionBitmapBits[32];
    ULONG                        SessionId;
    ULARGE_INTEGER               AppCompatFlags;
    ULARGE_INTEGER               AppCompatFlagsUser;
    PVOID                        ShimData;
    PVOID                        AppCompatInfo;
    UNICODE_STRING               CSDVersion;
    PVOID                        ActivationContextData;
    PVOID                        ProcessAssemblyStorageMap;
    PVOID                        SystemDefaultActivationData;
    PVOID                        SystemAssemblyStorageMap;
    SIZE_T                       MinimumStackCommit;
    PVOID                       *FlsCallback;
    LIST_ENTRY                   FlsListHead;
    PRTL_BITMAP                  FlsBitmap;
    ULONG                        FlsBitmapBits[4];
    ULONG                        FlsHighIndex;
} PEB, *PPEB;

typedef struct _NT_TIB
{
     struct _EXCEPTION_REGISTRATION_RECORD *ExceptionList;
     PVOID StackBase;
     PVOID StackLimit;
     PVOID SubSystemTib;
     union
     {
          PVOID FiberData;
          ULONG Version;
     };
     PVOID ArbitraryUserPointer;
     struct _NT_TIB *Self;
} NT_TIB, *PNT_TIB;

typedef struct _TEB {
    NT_TIB                       NtTib;
    PVOID                        EnvironmentPointer;
    CLIENT_ID                    ClientId;
    PVOID                        ActiveRpcHandle;
    PVOID                        ThreadLocalStoragePointer;
    PEB                          *Peb;
    ULONG                        LastErrorValue;
    ULONG                        CountOfOwnedCriticalSections;
    PVOID                        CsrClientThread;
    PVOID                        Win32ThreadInfo;
    ULONG                        User32Reserved[26];                // used for user32 private data in Wine
    PKUSER_SHARED_DATA           SharedUserData;
    LogModuleFunc                LogModule;
    ULONG                        UserReserved[3];                   // was ULONG[5], added SharedUserData and LogModule, beware PVOID vs ULONG*/
    PVOID                        WOW32Reserved;
    ULONG                        CurrentLocale;
    ULONG                        FpSoftwareStatusRegister;
    PVOID                        SystemReserved1[54];               // used for kernel32 private data in Wine
    LONG                         ExceptionCode;
    PACTIVATION_CONTEXT_STACK    ActivationContextStack;
    BYTE                         SpareBytes1[36];
    ULONG                        TxFsContext;
    GDI_TEB_BATCH                GdiTebBatch;
    CLIENT_ID                    RealClientId;
    HANDLE                       GdiCachedProcessHandle;
    ULONG                        GdiClientPID;
    ULONG                        GdiClientTID;
    PVOID                        GdiThreadLocaleInfo;
    ULONG_PTR                    Win32ClientInfo[62];
    PVOID                        glDispachTable[233];
    PVOID                        glReserved1[29];
    PVOID                        glReserved2;
    PVOID                        glSectionInfo;
    PVOID                        glSection;
    PVOID                        glTable;
    PVOID                        glCurrentRC;
    PVOID                        glContext;
    ULONG                        LastStatusValue;
    UNICODE_STRING               StaticUnicodeString;
    WCHAR                        StaticUnicodeBuffer[261];
    PVOID                        DeallocationStack;
    PVOID                        TlsSlots[64];
    LIST_ENTRY                   TlsLinks;
    PVOID                        Vdm;
    PVOID                        ReservedForNtRpc;
    PVOID                        DbgSsReserved[2];
    ULONG                        HardErrorMode;
    PVOID                        Instrumentation[9]; // 14 - guid
    GUID                         ActivityId;
    PVOID                        SubProcessTag;
    PVOID                        EtwLocalData;
    PVOID                        EtwTraceData;
    PVOID                        WinSockData;
    ULONG                        GdiBatchCount;
    BOOLEAN                      InDbgPrint;
    BOOLEAN                      FreeStackOnTermination;
    BOOLEAN                      HasFiberData;
    BOOLEAN                      IdealProcessor;
    ULONG                        GuaranteedStackBytes; // Should be zero
    PVOID                        ReservedForPerfLog;
    PVOID                        ReservedForOle;
    ULONG                        WaitingOnLoaderLock;
    PVOID                        SavedPriorityState;
    PVOID                        SoftPatchPtr1;
    PVOID                        ThreadPoolData;
    PVOID                       *TlsExpansionSlots;
    ULONG                        ImpersonationLocale;
    ULONG                        IsImpersonating;
    PVOID                        NlsCache;
    PVOID                        pShimData;
    ULONG                        HeapVirtualAffinity;
    PVOID                        CurrentTransactionHandle;
    PVOID                        ActiveFrame;
    PVOID                        FlsData;
    BOOLEAN                      SafeThunkCall;
    BOOLEAN                      BooleanSpare[3];
} TEB, *PTEB;

#define HEAP_ZERO_MEMORY    0x00000008

enum
{
    REG_NONE = 0,
    REG_SZ,
    REG_EXPAND_SZ,
    REG_BINARY,
    REG_DWORD,
    REG_DWORD_BIG_ENDIAN,
    REG_LINK,
    REG_MULTI_SZ,
    REG_RESOURCE_LIST
};

static inline TEB *NtCurrentTeb(void)
{
#ifdef __GNUC__
    TEB *teb;
    asm ("movl %%fs:0x18, %0" : "=r" (teb));
    return teb;
#else
    __asm mov eax, fs:[0x18];
#endif
}

TEB *NtCurrentTeb(VOID);
#define NtCurrentPeb() NtCurrentTeb()->Peb

#include "nt_compat.h"

#endif
