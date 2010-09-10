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

#include <stdio.h>
#include <stdlib.h>
#include <ddk/ntddk.h>
#include <ddk/ntifs.h>
#include <ddk/ntdddisk.h>

#define BPX() __asm__ volatile ("int 3")

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

typedef struct _MOUNTDEV_NAME {
    USHORT NameLength;
    WCHAR  Name[1];
} MOUNTDEV_NAME, *PMOUNTDEV_NAME;
#define IOCTL_MOUNTDEV_QUERY_DEVICE_NAME 0x004d0008

typedef struct _STORAGE_HOTPLUG_INFO {
    UINT  Size;
    UCHAR MediaRemovable;
    UCHAR MediaHotplug;
    UCHAR DeviceHotplug;
    UCHAR WriteCacheEnableOverride;
} STORAGE_HOTPLUG_INFO, *PSTORAGE_HOTPLUG_INFO;
#define IOCTL_STORAGE_GET_HOTPLUG_INFO 0x002d0c14

#define FSCTL_QUERY_DEPENDENT_VOLUME 0x000901f0

enum
{
    _StorageDeviceProperty = 0,
    _StorageAdapterProperty,
    _StorageDeviceIdProperty,
    StorageDeviceUniqueIdProperty,
    StorageDeviceWriteCacheProperty,
    StorageMiniportProperty,
    StorageAccessAlignmentProperty,
    StorageDeviceSeekPenaltyProperty,
    StorageDeviceTrimProperty,
    StorageDeviceWriteAggregationProperty
};

// http://msdn.microsoft.com/en-us/library/ff800832(v=VS.85).aspx
static const char bus_types[][32] = { "Invalid", "Scsi", "Atapi", "Ata", "1394", "Ssa", "Fibre", "Usb", "RAID", "iScsi", "Sas", "Sata", "Sd", "Mmc" };
typedef struct __STORAGE_ADAPTER_DESCRIPTOR {
    DWORD   Version;                // 00 - sizeof(_STORAGE_ADAPTER_DESCRIPTOR)
    DWORD   Size;                   // 04 - sizeof(_STORAGE_ADAPTER_DESCRIPTOR)
    DWORD   MaximumTransferLength;  // 08 - 0x20000 for c: 0x10000 for f:
    DWORD   MaximumPhysicalPages;   // 12 - 0x20 for c: - 0x11 for f:
    DWORD   AlignmentMask;          // 16 - 0x01 for c: - 0x00 for f: values: 0 (byte aligned), 1 (word aligned), 3 (DWORD aligned) and 7 (double DWORD aligned)
    BOOLEAN AdapterUsesPio;         // 20 - 0x01 for c: - 0x00 for f:
    BOOLEAN AdapterScansDown;       // 21 - 0x01 for c: - 0x00 for f:
    BOOLEAN CommandQueueing;        // 22 - 0x00 for c: - 0x00 for f:
    BOOLEAN AcceleratedTransfer;    // 23 - 0x00 for c: - 0x00 for f:
    BYTE    BusType;                // 24 - 0x03 for c: - 0x07 for f: (see table) STORAGE_BUS_TYPE
    WORD    BusMajorVersion;        // 25 - 0x01 for c: - 0x02 for f:
    WORD    BusMinorVersion;        // 27 - 0x00 for c: - 0x00 for f:
} _STORAGE_ADAPTER_DESCRIPTOR, *_PSTORAGE_ADAPTER_DESCRIPTOR;

typedef struct _STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR {
    DWORD Version;
    DWORD Size;
    DWORD BytesPerCacheLine;
    DWORD BytesOffsetForCacheAlignment;
    DWORD BytesPerLogicalSector;
    DWORD BytesPerPhysicalSector;
    DWORD BytesOffsetForSectorAlignment;
} STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR, *PSTORAGE_ACCESS_ALIGNMENT_DESCRIPTOR;

int main(int argc, char *argv[])
{
    NTSTATUS res;
    char *outbuf;

    WCHAR drive[] = L"\\??\\c:";

    if (argc > 1)
        drive[4] = argv[1][0];

    OBJECT_ATTRIBUTES ob;
    memset(&ob, 0, sizeof(OBJECT_ATTRIBUTES));
    ob.Length = sizeof(OBJECT_ATTRIBUTES);

    UNICODE_STRING name;
    RtlInitUnicodeString(&name, drive);
    ob.ObjectName = &name;

    HANDLE h;
    IO_STATUS_BLOCK IoStatusBlock;

    //res = NtOpenFile(&h, 0x100080, &ob, &IoStatusBlock, 7, 17);
    res = NtOpenFile(&h, 0x00100003, &ob, &IoStatusBlock, 3, 32);

    if (res)
    {
        wprintf(L"Fail to open %s = 0x%08x\n", drive, (int) res);
        exit(0);
    }

    wprintf(L"Opened %s\n", drive);

    FILE_ALIGNMENT_INFORMATION al = { -1 };
    res = NtQueryInformationFile(h, &IoStatusBlock, &al, sizeof(al), FileAlignmentInformation);
    printf("0x%08x: AlignmentRequirement = %lu\n", (int) res, al.AlignmentRequirement);

    FILE_FS_DEVICE_INFORMATION fsi;
    res = NtQueryVolumeInformationFile(h, &IoStatusBlock, &fsi, sizeof(fsi), FileFsDeviceInformation);
    printf("0x%08x: Volume: DeviceType = 0x%08x - Characteristics = 0x%08x\n", (int) res,
        (int) fsi.DeviceType, (int) fsi.Characteristics);

    DISK_GEOMETRY geo;
    res = NtDeviceIoControlFile(h, NULL, NULL, NULL, &IoStatusBlock, IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0, &geo, sizeof(geo));
    printf("0x%08x: Geo (Type:%d): C = %I64lld - %d/%d/%d\n", (int) res, geo.MediaType, geo.Cylinders.QuadPart,
    (int) geo.TracksPerCylinder, (int) geo.SectorsPerTrack, (int) geo.BytesPerSector);

    GET_LENGTH_INFORMATION gli;
    res = NtDeviceIoControlFile(h, NULL, NULL, NULL, &IoStatusBlock, IOCTL_DISK_GET_LENGTH_INFO, NULL, 0, &gli, sizeof(gli));
    printf("0x%08x: Volume Length %I64lld\n", (int) res, gli.Length.QuadPart);

    PARTITION_INFORMATION_EX piex;
    res = NtDeviceIoControlFile(h, NULL, NULL, NULL, &IoStatusBlock, IOCTL_DISK_GET_PARTITION_INFO_EX, NULL, 0, &piex, sizeof(piex));
    printf("0x%08x: Partinfo: offset: %I64lld - len %I64lld - Hidden = %d\n", (int) res, piex.StartingOffset.QuadPart, piex.PartitionLength.QuadPart,
        (int) piex.Mbr.HiddenSectors);

    STORAGE_DEVICE_NUMBER sdn;
    res = NtDeviceIoControlFile(h, NULL, NULL, NULL, &IoStatusBlock, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0, &sdn, sizeof(sdn));
    printf("0x%08x: Type: 0x%08x - Device %d - Partition %d\n", (int) res, (int) sdn.DeviceType, (int) sdn.DeviceNumber, (int) sdn.PartitionNumber);

    #define DLE_SIZE 14016
    DRIVE_LAYOUT_INFORMATION_EX *dle = calloc(1, DLE_SIZE);
    res = NtDeviceIoControlFile(h, NULL, NULL, NULL, &IoStatusBlock, IOCTL_DISK_GET_DRIVE_LAYOUT_EX, NULL, 0, dle, DLE_SIZE);
    printf("0x%08x: Partition Count %d - Signature 0x%08x\n", (int) res, (int) dle->PartitionCount, (int) dle->Mbr.Signature);
    free(dle);

    #define MDEV_SIZE 524
    MOUNTDEV_NAME *mdev = calloc(1, MDEV_SIZE);
    res = NtDeviceIoControlFile(h, NULL, NULL, NULL, &IoStatusBlock, IOCTL_MOUNTDEV_QUERY_DEVICE_NAME, NULL, 0, mdev, MDEV_SIZE);
    wprintf(L"0x%08x: Device Name: %s\n", (int) res, mdev->Name);
    free(mdev);

    STORAGE_HOTPLUG_INFO hi;
    res = NtDeviceIoControlFile(h, NULL, NULL, NULL, &IoStatusBlock, IOCTL_STORAGE_GET_HOTPLUG_INFO, NULL, 0, &hi, sizeof(hi));
    printf("0x%08x: Removable %d - Hotplug %d - DeviceHotplug %d - WCO %d\n", (int) res, (int) hi.MediaRemovable,
        (int) hi.MediaHotplug, (int) hi.DeviceHotplug, (int) hi.WriteCacheEnableOverride);

    #define DEPVOL_SIZE 268
    outbuf = calloc(1, 268);
    DWORD depvol[2] = { 1, 1 };
    memset(outbuf, 0, DEPVOL_SIZE);
    res = NtFsControlFile(h, NULL, NULL, NULL, &IoStatusBlock, FSCTL_QUERY_DEPENDENT_VOLUME, depvol, sizeof(depvol), outbuf, DEPVOL_SIZE);
    printf("0x%08x: FSCTL_QUERY_DEPENDENT_VOLUME\n", (int) res); // STATUS_INVALID_DEVICE_REQUEST
    free(outbuf);

    STORAGE_PROPERTY_QUERY spq;

    STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR saad;
    spq.PropertyId = StorageAccessAlignmentProperty; // vista+
    spq.QueryType = PropertyStandardQuery;
    memset(&saad, 0, sizeof(saad));
    res = NtDeviceIoControlFile(h, NULL, NULL, NULL, &IoStatusBlock, IOCTL_STORAGE_QUERY_PROPERTY, &spq, sizeof(spq), &saad, sizeof(saad));
    printf("0x%08x: IOCTL_STORAGE_QUERY_PROPERTY Align:\n", (int) res); // STATUS_NOT_IMPLEMENTED

    spq.PropertyId = StorageAdapterProperty;
    spq.QueryType = PropertyStandardQuery;
    spq.AdditionalParameters[0] = 0x0;
    _STORAGE_ADAPTER_DESCRIPTOR sad;
    memset(&sad, 0, sizeof(sad));
    res = NtDeviceIoControlFile(h, NULL, NULL, NULL, &IoStatusBlock, IOCTL_STORAGE_QUERY_PROPERTY, &spq, sizeof(spq), &sad, sizeof(sad));
    if (sad.BusType > 12) sad.BusType = 0;
    printf("0x%08x: IOCTL_STORAGE_QUERY_PROPERTY Adapter: %s\n", (int) res, bus_types[sad.BusType]);

    //BPX();

    NtClose(h);

    return 0;
}
