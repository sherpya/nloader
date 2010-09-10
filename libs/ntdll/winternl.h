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

#ifndef _WINTERNL_H
#define _WINTERNL_H

typedef LONG NTSTATUS;
#define STATUS_SUCCESS                  0x00000000
#define STATUS_BUFFER_OVERFLOW          0x80000005
#define STATUS_NO_MORE_ENTRIES          0x8000001A
#define STATUS_UNSUCCESSFUL             0xC0000001
#define STATUS_NOT_IMPLEMENTED          0xC0000002
#define STATUS_INVALID_INFO_CLASS       0xC0000003
#define STATUS_ACCESS_VIOLATION         0xC0000005
#define STATUS_INVALID_HANDLE           0xC0000008
#define STATUS_INVALID_PARAMETER        0xC000000D
#define STATUS_INVALID_DEVICE_REQUEST   0xC0000010
#define STATUS_NO_MEMORY                0xC0000017
#define STATUS_ACCESS_DENIED            0xC0000022
#define STATUS_BUFFER_TOO_SMALL         0xC0000023
#define STATUS_OBJECT_NAME_NOT_FOUND    0xC0000034
#define STATUS_UNKNOWN_REVISION         0xC0000058
#define STATUS_REVISION_MISMATCH        0xC0000059
#define STATUS_INVALID_ACL              0xC0000077
#define STATUS_INVALID_SID              0xC0000078
#define STATUS_ALLOTTED_SPACE_EXCEEDED  0xC0000099
#define STATUS_INSUFFICIENT_RESOURCES   0xC000009A
#define STATUS_MEDIA_WRITE_PROTECTED    0xC00000A2
#define STATUS_DEVICE_NOT_READY         0xC00000A3
#define STATUS_NOT_SUPPORTED            0xC00000BB
#define STATUS_BAD_DESCRIPTOR_FORMAT    0xC00000E7
#define STATUS_NAME_TOO_LONG            0xC0000106
#define STATUS_MESSAGE_NOT_FOUND        0xC0000109
#define STATUS_BAD_COMPRESSION_BUFFER   0xC0000242

#define STATUS_WAIT_0                   0x00000000
#define STATUS_ALERTED                  0x00000101
#define STATUS_TIMEOUT                  0x00000102

#define RTL_QUERY_REGISTRY_NOEXPAND     0x00000010
#define RTL_QUERY_REGISTRY_DIRECT       0x00000020

#define CHECK_POINTER(p) do { if (!p) \
    { Log("ntdll.%s() - !! " #p " == NULL !!\n", __func__); return STATUS_ACCESS_VIOLATION; } } while (0)

#define CHECK_HANDLE(_handle, _type) do { CHECK_POINTER(_handle); if \
    ((_handle->cookie != HANDLE_COOKIE) || (_handle->type != _type)) \
    { Log("ntdll.%s() - !! HANDLE != " #_type " !!\n", __func__); return STATUS_INVALID_PARAMETER; } } while (0)

typedef DWORD EXECUTION_STATE;

typedef DWORD ACCESS_MASK, *PACCESS_MASK;
typedef void *PIO_APC_ROUTINE;

typedef struct _IO_STATUS_BLOCK {
  union {
    NTSTATUS Status;
    PVOID Pointer;
  } u;

  ULONG_PTR Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

typedef enum _OBJECT_WAIT_TYPE {
    WaitAllObject,
    WaitAnyObject
} OBJECT_WAIT_TYPE, *POBJECT_WAIT_TYPE;

/* http://msdn.microsoft.com/en-us/library/cc246805%28PROT.13%29.aspx */
#define CTL_CODE( DeviceType, Function, Method, Access ) ( \
    (DWORD)((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method) )

#define METHOD_BUFFERED                     0
#define METHOD_IN_DIRECT                    1
#define METHOD_OUT_DIRECT                   2
#define METHOD_NEITHER                      3

#define FILE_ANY_ACCESS                     0
#define FILE_SPECIAL_ACCESS                 0
#define FILE_READ_ACCESS                    1
#define FILE_WRITE_ACCESS                   2

#define FILE_DEVICE_DISK                    0x00000007
#define FILE_DEVICE_FILE_SYSTEM             0x00000009
#define FILE_DEVICE_KEYBOARD                0x0000000b
#define FILE_DEVICE_MASS_STORAGE            0x0000002d

#define FSCTL_LOCK_VOLUME                   CTL_CODE(FILE_DEVICE_FILE_SYSTEM,   6, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_UNLOCK_VOLUME                 CTL_CODE(FILE_DEVICE_FILE_SYSTEM,   7, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_DISMOUNT_VOLUME               CTL_CODE(FILE_DEVICE_FILE_SYSTEM,   8, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_GET_RETRIEVAL_POINTERS        CTL_CODE(FILE_DEVICE_FILE_SYSTEM,  28, METHOD_NEITHER,  FILE_ANY_ACCESS)
#define FSCTL_IS_VOLUME_DIRTY               CTL_CODE(FILE_DEVICE_FILE_SYSTEM,  30, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_ALLOW_EXTENDED_DASD_IO        CTL_CODE(FILE_DEVICE_FILE_SYSTEM,  32, METHOD_NEITHER,  FILE_ANY_ACCESS)
#define FSCTL_QUERY_DEPENDENT_VOLUME        CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 124, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_STORAGE_BASE                  FILE_DEVICE_MASS_STORAGE
#define IOCTL_STORAGE_GET_HOTPLUG_INFO      CTL_CODE(IOCTL_STORAGE_BASE, 0x0305, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_STORAGE_GET_DEVICE_NUMBER     CTL_CODE(IOCTL_STORAGE_BASE, 0x0420, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_STORAGE_QUERY_PROPERTY        CTL_CODE(IOCTL_STORAGE_BASE, 0x0500, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_KEYBOARD_SET_INDICATORS       CTL_CODE(FILE_DEVICE_KEYBOARD, 0x0002, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_KEYBOARD_QUERY_INDICATORS     CTL_CODE(FILE_DEVICE_KEYBOARD, 0x0010, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_DISK_BASE                     FILE_DEVICE_DISK
#define IOCTL_DISK_GET_DRIVE_GEOMETRY       CTL_CODE(IOCTL_DISK_BASE, 0x0000, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DISK_GET_PARTITION_INFO       CTL_CODE(IOCTL_DISK_BASE, 0x0001, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_DISK_SET_PARTITION_INFO       CTL_CODE(IOCTL_DISK_BASE, 0x0002, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_DISK_VERIFY                   CTL_CODE(IOCTL_DISK_BASE, 0x0005, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DISK_IS_WRITABLE              CTL_CODE(IOCTL_DISK_BASE, 0x0009, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DISK_GET_PARTITION_INFO_EX    CTL_CODE(IOCTL_DISK_BASE, 0x0012, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DISK_GET_DRIVE_LAYOUT_EX      CTL_CODE(IOCTL_DISK_BASE, 0x0014, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DISK_GET_LENGTH_INFO          CTL_CODE(IOCTL_DISK_BASE, 0x0017, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_DISK_GET_MEDIA_TYPES          CTL_CODE(IOCTL_DISK_BASE, 0x0300, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_MOUNTDEV_BASE                 ((DWORD)'M')
#define IOCTL_MOUNTDEV_QUERY_DEVICE_NAME    CTL_CODE(IOCTL_MOUNTDEV_BASE, 0x0002, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define VOLUME_IS_DIRTY                 1

#define PARTITION_ENTRY_UNUSED          0x00
#define PARTITION_FAT_12                0x01
#define PARTITION_EXTENDED              0x05
#define PARTITION_FAT_16                0x04
#define PARTITION_IFS                   0x07
#define PARTITION_FAT32                 0x0B

/* NtCreateFile/NtOpenFile */
#define FILE_RESERVE_OPFILTER           0x00100000
#define FILE_READ_DATA                  0x00000001
#define FILE_WRITE_DATA                 0x00000002
#define FILE_APPEND_DATA                0x00000004
#define FILE_READ_ATTRIBUTES            0x00000080
#define FILE_WRITE_ATTRIBUTES           0x00000100
#define FILE_READ_EA                    0x00000008
#define FILE_WRITE_EA                   0x00000010
#define GENERIC_READ                    0x80000000
#define GENERIC_WRITE                   0x40000000

/* NtCreateFile/NtOpenFile */
#define FILE_SUPERSEDED                 0
#define FILE_OPENED                     1
#define FILE_CREATED                    2
#define FILE_OVERWRITTEN                3
#define FILE_EXISTS                     4
#define FILE_DOES_NOT_EXIST             5

typedef struct _OBJECT_ATTRIBUTES {
  ULONG Length;
  HANDLE RootDirectory;
  PUNICODE_STRING ObjectName;
  ULONG Attributes;
  PVOID SecurityDescriptor;       /* type SECURITY_DESCRIPTOR */
  PVOID SecurityQualityOfService; /* type SECURITY_QUALITY_OF_SERVICE */
} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;

typedef struct _KEYBOARD_INDICATOR_PARAMETERS {
    USHORT UnitId;
    USHORT LedFlags;
} KEYBOARD_INDICATOR_PARAMETERS, *PKEYBOARD_INDICATOR_PARAMETERS;

typedef NTSTATUS (NTAPI *PRTL_QUERY_REGISTRY_ROUTINE)(PCWSTR ValueName,
                                                       ULONG ValueType,
                                                       PVOID ValueData,
                                                       ULONG ValueLength,
                                                       PVOID Context,
                                                       PVOID EntryContext);

typedef struct _RTL_QUERY_REGISTRY_TABLE
{
    PRTL_QUERY_REGISTRY_ROUTINE QueryRoutine;
    ULONG Flags;
    PWSTR Name;
    PVOID EntryContext;
    ULONG DefaultType;
    PVOID DefaultData;
    ULONG DefaultLength;
} RTL_QUERY_REGISTRY_TABLE, *PRTL_QUERY_REGISTRY_TABLE;

typedef struct _OBJECT_DIRECTORY_INFORMATION {
    UNICODE_STRING Name;
    UNICODE_STRING TypeName;
} OBJECT_DIRECTORY_INFORMATION, *POBJECT_DIRECTORY_INFORMATION;

typedef enum _PROCESSINFOCLASS {
    ProcessBasicInformation = 0,
    ProcessQuotaLimits = 1,
    ProcessIoCounters = 2,
    ProcessVmCounters = 3,
    ProcessTimes = 4,
    ProcessBasePriority = 5,
    ProcessRaisePriority = 6,
    ProcessDebugPort = 7,
    ProcessExceptionPort = 8,
    ProcessAccessToken = 9,
    ProcessLdtInformation = 10,
    ProcessLdtSize = 11,
    ProcessDefaultHardErrorMode = 12,
    ProcessIoPortHandlers = 13,
    ProcessPooledUsageAndLimits = 14,
    ProcessWorkingSetWatch = 15,
    ProcessUserModeIOPL = 16,
    ProcessEnableAlignmentFaultFixup = 17,
    ProcessPriorityClass = 18,
    ProcessWx86Information = 19,
    ProcessHandleCount = 20,
    ProcessAffinityMask = 21,
    ProcessPriorityBoost = 22,
    ProcessDeviceMap = 23,
    ProcessSessionInformation = 24,
    ProcessForegroundInformation = 25,
    ProcessWow64Information = 26,
    ProcessImageFileName = 27,
    ProcessLUIDDeviceMapsEnabled = 28,
    ProcessBreakOnTermination = 29,
    ProcessDebugObjectHandle = 30,
    ProcessDebugFlags = 31,
    ProcessHandleTracing = 32,
    ProcessExecuteFlags = 34,
    MaxProcessInfoClass
} PROCESSINFOCLASS, PROCESS_INFORMATION_CLASS;

typedef enum _KEY_VALUE_INFORMATION_CLASS {
    KeyValueBasicInformation,
    KeyValueFullInformation,
    KeyValuePartialInformation,
    KeyValueFullInformationAlign64,
    KeyValuePartialInformationAlign64
} KEY_VALUE_INFORMATION_CLASS;

typedef struct _KEY_VALUE_FULL_INFORMATION {
    ULONG TitleIndex;
    LONG Type;
    ULONG DataOffset;
    ULONG DataLength;
    ULONG NameLength;
    WCHAR Name[1];
} KEY_VALUE_FULL_INFORMATION, *PKEY_VALUE_FULL_INFORMATION;

typedef struct _KEY_VALUE_PARTIAL_INFORMATION {
  ULONG TitleIndex;
  ULONG Type;
  ULONG DataLength;
  UCHAR Data[1];
} KEY_VALUE_PARTIAL_INFORMATION, *PKEY_VALUE_PARTIAL_INFORMATION;

typedef enum _FILE_INFORMATION_CLASS {
  FileDirectoryInformation = 1,
  FileFullDirectoryInformation,
  FileBothDirectoryInformation,
  FileBasicInformation,
  FileStandardInformation,
  FileInternalInformation,
  FileEaInformation,
  FileAccessInformation,
  FileNameInformation,
  FileRenameInformation,
  FileLinkInformation,
  FileNamesInformation,
  FileDispositionInformation,
  FilePositionInformation,
  FileFullEaInformation,
  FileModeInformation,
  FileAlignmentInformation,
  FileAllInformation,
  FileAllocationInformation,
  FileEndOfFileInformation,
  FileAlternateNameInformation,
  FileStreamInformation,
  FilePipeInformation,
  FilePipeLocalInformation,
  FilePipeRemoteInformation,
  FileMailslotQueryInformation,
  FileMailslotSetInformation,
  FileCompressionInformation,
  FileObjectIdInformation,
  FileCompletionInformation,
  FileMoveClusterInformation,
  FileQuotaInformation,
  FileReparsePointInformation,
  FileNetworkOpenInformation,
  FileAttributeTagInformation,
  FileTrackingInformation,
  FileIdBothDirectoryInformation,
  FileIdFullDirectoryInformation,
  FileValidDataLengthInformation,
  FileShortNameInformation,
  FileMaximumInformation
} FILE_INFORMATION_CLASS, *PFILE_INFORMATION_CLASS;

typedef struct _FILE_ALIGNMENT_INFORMATION {
  ULONG AlignmentRequirement;
} FILE_ALIGNMENT_INFORMATION, *PFILE_ALIGNMENT_INFORMATION;

typedef struct _FILE_BASIC_INFORMATION {
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    ULONG FileAttributes;
} FILE_BASIC_INFORMATION, *PFILE_BASIC_INFORMATION;

typedef struct _FILE_STANDARD_INFORMATION {
    LARGE_INTEGER AllocationSize;
    LARGE_INTEGER EndOfFile;
    ULONG NumberOfLinks;
    BOOLEAN DeletePending;
    BOOLEAN Directory;
} FILE_STANDARD_INFORMATION, *PFILE_STANDARD_INFORMATION;

typedef enum _FSINFOCLASS {
    FileFsVolumeInformation = 1,
    FileFsLabelInformation,
    FileFsSizeInformation,
    FileFsDeviceInformation,
    FileFsAttributeInformation,
    FileFsControlInformation,
    FileFsFullSizeInformation,
    FileFsObjectIdInformation,
    FileFsMaximumInformation
} FS_INFORMATION_CLASS, *PFS_INFORMATION_CLASS;

/* FileFsDeviceInformation = 4 */
typedef struct _FILE_FS_DEVICE_INFORMATION {
    DWORD DeviceType;
    ULONG Characteristics;
} FILE_FS_DEVICE_INFORMATION, *PFILE_FS_DEVICE_INFORMATION;

typedef struct {
    LARGE_INTEGER Length;
} GET_LENGTH_INFORMATION;

typedef enum _MEDIA_TYPE {
    Unknown, F5_1Pt2_512, F3_1Pt44_512, F3_2Pt88_512, F3_20Pt8_512, F3_720_512, F5_360_512,
    F5_320_512, F5_320_1024, F5_180_512, F5_160_512, RemovableMedia, FixedMedia, F3_120M_512,
    F3_640_512, F5_640_512, F5_720_512, F3_1Pt2_512, F3_1Pt23_1024, F5_1Pt23_1024, F3_128Mb_512,
    F3_230Mb_512, F8_256_128
} MEDIA_TYPE, *PMEDIA_TYPE;

typedef struct _DISK_GEOMETRY {
    LARGE_INTEGER       Cylinders;
    MEDIA_TYPE          MediaType;
    DWORD               TracksPerCylinder;
    DWORD               SectorsPerTrack;
    DWORD               BytesPerSector;
} DISK_GEOMETRY, *PDISK_GEOMETRY;

typedef struct _SET_PARTITION_INFORMATION {
    BYTE PartitionType;
} SET_PARTITION_INFORMATION, *PSET_PARTITION_INFORMATION;

typedef struct _VERIFY_INFORMATION {
    LARGE_INTEGER StartingOffset;
    DWORD         Length;
} VERIFY_INFORMATION, *PVERIFY_INFORMATION;

typedef struct _DRIVE_LAYOUT_INFORMATION_MBR {
    ULONG Signature;
} DRIVE_LAYOUT_INFORMATION_MBR, *PDRIVE_LAYOUT_INFORMATION_MBR;

typedef enum _PARTITION_STYLE {
    PARTITION_STYLE_MBR   = 0,
    PARTITION_STYLE_GPT   = 1,
    PARTITION_STYLE_RAW   = 2
} PARTITION_STYLE;

typedef struct _PARTITION_INFORMATION_MBR {
    BYTE    PartitionType;
    BOOLEAN BootIndicator;
    BOOLEAN RecognizedPartition;
    DWORD   HiddenSectors;
} PARTITION_INFORMATION_MBR, *PPARTITION_INFORMATION_MBR;

typedef struct _PARTITION_INFORMATION_GPT {
    GUID    PartitionType;
    GUID    PartitionId;
    DWORD64 Attributes;
    WCHAR   Name[36];
} PARTITION_INFORMATION_GPT, *PPARTITION_INFORMATION_GPT;

typedef struct {
    PARTITION_STYLE PartitionStyle;
    LARGE_INTEGER   StartingOffset;
    LARGE_INTEGER   PartitionLength;
    DWORD           PartitionNumber;
    BOOLEAN         RewritePartition;
    union
    {
        PARTITION_INFORMATION_MBR Mbr;
        PARTITION_INFORMATION_GPT Gpt;
    };
} PARTITION_INFORMATION_EX;

typedef struct _PARTITION_INFORMATION {
    LARGE_INTEGER StartingOffset;
    LARGE_INTEGER PartitionLength;
    DWORD         HiddenSectors;
    DWORD         PartitionNumber;
    BYTE          PartitionType;
    BOOLEAN       BootIndicator;
    BOOLEAN       RecognizedPartition;
    BOOLEAN       RewritePartition;
} PARTITION_INFORMATION, *PPARTITION_INFORMATION;

typedef struct _DRIVE_LAYOUT_INFORMATION_GPT {
    GUID          DiskId;
    LARGE_INTEGER StartingUsableOffset;
    LARGE_INTEGER UsableLength;
    ULONG         MaxPartitionCount;
} DRIVE_LAYOUT_INFORMATION_GPT, *PDRIVE_LAYOUT_INFORMATION_GPT;

typedef struct _DRIVE_LAYOUT_INFORMATION_EX {
    DWORD PartitionStyle;
    DWORD PartitionCount;
    union
    {
        DRIVE_LAYOUT_INFORMATION_MBR Mbr;
        DRIVE_LAYOUT_INFORMATION_GPT Gpt;
    };
    PARTITION_INFORMATION_EX PartitionEntry[1];
} DRIVE_LAYOUT_INFORMATION_EX, *PDRIVE_LAYOUT_INFORMATION_EX;

#define DEVICE_TYPE DWORD
typedef struct _STORAGE_DEVICE_NUMBER {
    DEVICE_TYPE DeviceType;
    ULONG       DeviceNumber;
    ULONG       PartitionNumber;
} STORAGE_DEVICE_NUMBER, *PSTORAGE_DEVICE_NUMBER;

typedef struct _STORAGE_HOTPLUG_INFO {
    UINT  Size;
    BOOLEAN MediaRemovable;
    BOOLEAN MediaHotplug;
    BOOLEAN DeviceHotplug;
    BOOLEAN WriteCacheEnableOverride;
} STORAGE_HOTPLUG_INFO, *PSTORAGE_HOTPLUG_INFO;

typedef enum _STORAGE_PROPERTY_ID {
    StorageDeviceProperty = 0,
    StorageAdapterProperty,
    StorageDeviceIdProperty,
    StorageDeviceUniqueIdProperty,
    StorageDeviceWriteCacheProperty,
    StorageMiniportProperty,
    StorageAccessAlignmentProperty,
    StorageDeviceSeekPenaltyProperty,
    StorageDeviceTrimProperty,
    StorageDeviceWriteAggregationProperty
} STORAGE_PROPERTY_ID, *PSTORAGE_PROPERTY_ID;

typedef enum _STORAGE_QUERY_TYPE {
    PropertyStandardQuery = 0,
    PropertyExistsQuery,
    PropertyMaskQuery,
    PropertyQueryMaxDefined
} STORAGE_QUERY_TYPE, *PSTORAGE_QUERY_TYPE;

typedef struct _STORAGE_PROPERTY_QUERY {
    STORAGE_PROPERTY_ID PropertyId;
    STORAGE_QUERY_TYPE QueryType;
    UCHAR AdditionalParameters[1];
} STORAGE_PROPERTY_QUERY, *PSTORAGE_PROPERTY_QUERY;

typedef enum _STORAGE_BUS_TYPE {
    BusTypeUnknown             = 0x00,
    BusTypeScsi                = 0x01,
    BusTypeAtapi               = 0x02,
    BusTypeAta                 = 0x03,
    BusType1394                = 0x04,
    BusTypeSsa                 = 0x05,
    BusTypeFibre               = 0x06,
    BusTypeUsb                 = 0x07,
    BusTypeRAID                = 0x08,
    BusTypeiScsi               = 0x09,
    BusTypeSas                 = 0x0a,
    BusTypeSata                = 0x0b,
    BusTypeSd                  = 0x0c,
    BusTypeMmc                 = 0x0d,
    BusTypeVirtual             = 0x0e,
    BusTypeFileBackedVirtual   = 0x0f,
    BusTypeMax                 = 0x10,
    BusTypeMaxReserved         = 0x7f
} STORAGE_BUS_TYPE, *PSTORAGE_BUS_TYPE;

typedef struct _STORAGE_ADAPTER_DESCRIPTOR {
    DWORD   Version;
    DWORD   Size;
    DWORD   MaximumTransferLength;
    DWORD   MaximumPhysicalPages;
    DWORD   AlignmentMask;
    BOOLEAN AdapterUsesPio;
    BOOLEAN AdapterScansDown;
    BOOLEAN CommandQueueing;
    BOOLEAN AcceleratedTransfer;
    BYTE    BusType;
    WORD    BusMajorVersion;
    WORD    BusMinorVersion;
} STORAGE_ADAPTER_DESCRIPTOR, *PSTORAGE_ADAPTER_DESCRIPTOR;

typedef struct _STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR {
    DWORD Version;
    DWORD Size;
    DWORD BytesPerCacheLine;
    DWORD BytesOffsetForCacheAlignment;
    DWORD BytesPerLogicalSector;
    DWORD BytesPerPhysicalSector;
    DWORD BytesOffsetForSectorAlignment;
} STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR, *PSTORAGE_ACCESS_ALIGNMENT_DESCRIPTOR;

typedef struct _MOUNTDEV_NAME {
    USHORT NameLength;
    WCHAR  Name[1];
} MOUNTDEV_NAME, *PMOUNTDEV_NAME;

/* --- */

#define SystemBasicInformation 0
#define SystemPerformanceInformation 2
#define SystemTimeOfDayInformation 3
typedef DWORD SYSTEM_INFORMATION_CLASS;

typedef enum _THREADINFOCLASS {
    ThreadBasicInformation,
    ThreadTimes,
    ThreadPriority,
    ThreadBasePriority,
    ThreadAffinityMask,
    ThreadImpersonationToken,
    ThreadDescriptorTableEntry,
    ThreadEnableAlignmentFaultFixup,
    ThreadEventPair_Reusable,
    ThreadQuerySetWin32StartAddress,
    ThreadZeroTlsCell,
    ThreadPerformanceCount,
    ThreadAmILastThread,
    ThreadIdealProcessor,
    ThreadPriorityBoost,
    ThreadSetTlsArrayAddress,
    ThreadIsIoPending,
    ThreadHideFromDebugger,
    MaxThreadInfoClass
} THREADINFOCLASS;

typedef struct _THREAD_BASIC_INFORMATION
{
    NTSTATUS  ExitStatus;
    PVOID     TebBaseAddress;
    CLIENT_ID ClientId;
    ULONG_PTR AffinityMask;
    LONG      Priority;
    LONG      BasePriority;
} THREAD_BASIC_INFORMATION, *PTHREAD_BASIC_INFORMATION;

typedef struct _SYSTEM_BASIC_INFORMATION
{
    DWORD     unknown;                              /* 00 */
    ULONG     KeMaximumIncrement;                   /* 04 */
    ULONG     PageSize;                             /* 08 */
    ULONG     MmNumberOfPhysicalPages;              /* 0c */
    ULONG     MmLowestPhysicalPage;                 /* 10 */
    ULONG     MmHighestPhysicalPage;                /* 14 */
    ULONG_PTR AllocationGranularity;                /* 18 */
    PVOID     LowestUserAddress;                    /* 1c */
    PVOID     HighestUserAddress;                   /* 20 */
    ULONG_PTR ActiveProcessorsAffinityMask;         /* 24 */
    BYTE      NumberOfProcessors;                   /* 28 */
} SYSTEM_BASIC_INFORMATION;

typedef struct _SYSTEM_PERFORMANCE_INFORMATION {
    LARGE_INTEGER IdleTime;                         /* 000 */
    LARGE_INTEGER ReadTransferCount;                /* 008 */
    LARGE_INTEGER WriteTransferCount;               /* 010 */
    LARGE_INTEGER OtherTransferCount;               /* 018 */
    ULONG ReadOperationCount;                       /* 020 */
    ULONG WriteOperationCount;                      /* 024 */
    ULONG OtherOperationCount;                      /* 028 */
    ULONG AvailablePages;                           /* 02c */
    ULONG TotalCommittedPages;                      /* 030 */
    ULONG TotalCommitLimit;                         /* 034 */
    ULONG PeakCommitment;                           /* 038 */
    ULONG PageFaults;                               /* 03c */
    ULONG WriteCopyFaults;                          /* 040 */
    ULONG TransitionFaults;                         /* 044 */
    ULONG Reserved1;                                /* 048 */
    ULONG DemandZeroFaults;                         /* 04c */
    ULONG PagesRead;                                /* 050 */
    ULONG PageReadIos;                              /* 054 */
    ULONG Reserved2[2];                             /* 058 */
    ULONG PagefilePagesWritten;                     /* 060 */
    ULONG PagefilePageWriteIos;                     /* 064 */
    ULONG MappedFilePagesWritten;                   /* 068 */
    ULONG MappedFilePageWriteIos;                   /* 06c */
    ULONG PagedPoolUsage;                           /* 070 */
    ULONG NonPagedPoolUsage;                        /* 074 */
    ULONG PagedPoolAllocs;                          /* 078 */
    ULONG PagedPoolFrees;                           /* 07c */
    ULONG NonPagedPoolAllocs;                       /* 080 */
    ULONG NonPagedPoolFrees;                        /* 084 */
    ULONG TotalFreeSystemPtes;                      /* 088 */
    ULONG SystemCodePage;                           /* 08c */
    ULONG TotalSystemDriverPages;                   /* 090 */
    ULONG TotalSystemCodePages;                     /* 094 */
    ULONG SmallNonPagedLookasideListAllocateHits;   /* 098 */
    ULONG SmallPagedLookasideListAllocateHits;      /* 09c */
    ULONG Reserved3;                                /* 0a0 */
    ULONG MmSystemCachePage;                        /* 0a4 */
    ULONG PagedPoolPage;                            /* 0a8 */
    ULONG SystemDriverPage;                         /* 0ac */
    ULONG FastReadNoWait;                           /* 0b0 */
    ULONG FastReadWait;                             /* 0b4 */
    ULONG FastReadResourceMiss;                     /* 0b8 */
    ULONG FastReadNotPossible;                      /* 0bc */
    ULONG FastMdlReadNoWait;                        /* 0c0 */
    ULONG FastMdlReadWait;                          /* 0c4 */
    ULONG FastMdlReadResourceMiss;                  /* 0c8 */
    ULONG FastMdlReadNotPossible;                   /* 0cc */
    ULONG MapDataNoWait;                            /* 0d0 */
    ULONG MapDataWait;                              /* 0d4 */
    ULONG MapDataNoWaitMiss;                        /* 0d8 */
    ULONG MapDataWaitMiss;                          /* 0dc */
    ULONG PinMappedDataCount;                       /* 0e0 */
    ULONG PinReadNoWait;                            /* 0e4 */
    ULONG PinReadWait;                              /* 0e8 */
    ULONG PinReadNoWaitMiss;                        /* 0ec */
    ULONG PinReadWaitMiss;                          /* 0f0 */
    ULONG CopyReadNoWait;                           /* 0f4 */
    ULONG CopyReadWait;                             /* 0f8 */
    ULONG CopyReadNoWaitMiss;                       /* 0fc */
    ULONG CopyReadWaitMiss;                         /* 100 */
    ULONG MdlReadNoWait;                            /* 104 */
    ULONG MdlReadWait;                              /* 108 */
    ULONG MdlReadNoWaitMiss;                        /* 10c */
    ULONG MdlReadWaitMiss;                          /* 110 */
    ULONG ReadAheadIos;                             /* 114 */
    ULONG LazyWriteIos;                             /* 118 */
    ULONG LazyWritePages;                           /* 11c */
    ULONG DataFlushes;                              /* 120 */
    ULONG DataPages;                                /* 124 */
    ULONG ContextSwitches;                          /* 128 */
    ULONG FirstLevelTbFills;                        /* 12c */
    ULONG SecondLevelTbFills;                       /* 130 */
    ULONG SystemCalls;                              /* 134 */
} SYSTEM_PERFORMANCE_INFORMATION, *PSYSTEM_PERFORMANCE_INFORMATION;

typedef struct _SYSTEM_TIMEOFDAY_INFORMATION {
    LARGE_INTEGER BootTime;
    LARGE_INTEGER SystemTime;
    LARGE_INTEGER TimeZoneBias;
    ULONG CurrentTimeZoneId;
    LARGE_INTEGER Unknown1[2];
} SYSTEM_TIMEOFDAY_INFORMATION, *PSYSTEM_TIMEOFDAY_INFORMATION;

#endif /* _WINTERNL_H */
