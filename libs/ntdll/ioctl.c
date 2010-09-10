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

#include "ntdll.h"

MODULE(ioctl)

NTSTATUS NTAPI NtQueryInformationFile(HANDLE FileHandle, PIO_STATUS_BLOCK IoStatusBlock, PVOID FileInformation,
    ULONG Length, FILE_INFORMATION_CLASS FileInformationClass)
{
    CHECK_HANDLE(FileHandle, HANDLE_FILE);
    CHECK_POINTER(IoStatusBlock);

#ifdef REDIR_SYSCALL
    {
        NTSTATUS res = ftbl.nt.NtQueryInformationFile(FileHandle->file.fh, IoStatusBlock, FileInformation, Length, FileInformationClass);
        Log("ntdll.NtQueryInformationFile(\"%s\", %s:0x%08x) = 0x%08x\n", strhandle(FileHandle), strfileinfo(FileInformationClass),
            FileInformationClass, res);
        return res;
    }
#endif

    Log("ntdll.NtQueryInformationFile(\"%s\", %s:0x%08x)", strhandle(FileHandle), strfileinfo(FileInformationClass),
        FileInformationClass);

    switch (FileInformationClass)
    {
        case FileAlignmentInformation:
        {
            FILE_ALIGNMENT_INFORMATION *fi = FileInformation;

            if (Length < sizeof(FILE_ALIGNMENT_INFORMATION))
            {
                IoStatusBlock->u.Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            fi->AlignmentRequirement = 0;

            IoStatusBlock->Information = sizeof(FILE_ALIGNMENT_INFORMATION);
            IoStatusBlock->u.Status = STATUS_SUCCESS;
            break;
        }
        case FileBasicInformation:
        {
            FILE_BASIC_INFORMATION *fi = FileInformation;

            if (Length < sizeof(FILE_BASIC_INFORMATION))
            {
                IoStatusBlock->u.Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            if (S_ISDIR(FileHandle->file.st.st_mode))
                fi->FileAttributes = FILE_ATTRIBUTE_DIRECTORY;
            else fi->FileAttributes = FILE_ATTRIBUTE_ARCHIVE;

            if (!(FileHandle->file.st.st_mode & (S_IWUSR | S_IWGRP | S_IWOTH)))
                fi->FileAttributes |= FILE_ATTRIBUTE_READONLY;

            RtlSecondsSince1970ToTime((DWORD) FileHandle->file.st.st_mtime, &fi->CreationTime);
            RtlSecondsSince1970ToTime((DWORD) FileHandle->file.st.st_mtime, &fi->LastWriteTime);
            RtlSecondsSince1970ToTime((DWORD) FileHandle->file.st.st_ctime, &fi->ChangeTime);
            RtlSecondsSince1970ToTime((DWORD) FileHandle->file.st.st_atime, &fi->LastAccessTime);

            IoStatusBlock->Information = sizeof(FILE_BASIC_INFORMATION);
            IoStatusBlock->u.Status = STATUS_SUCCESS;
            break;
        }
        case FileStandardInformation:
        {
            FILE_STANDARD_INFORMATION *fi = FileInformation;

            if (Length < sizeof(FILE_STANDARD_INFORMATION))
            {
                IoStatusBlock->u.Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            if ((fi->Directory = S_ISDIR(FileHandle->file.st.st_mode)))
            {
                fi->AllocationSize.QuadPart = 0;
                fi->EndOfFile.QuadPart      = 0;
                fi->NumberOfLinks           = 1;
            }
            else
            {
                fi->EndOfFile.QuadPart      = FileHandle->file.st.st_size;
#ifdef _WIN32
                fi->AllocationSize.QuadPart = fi->EndOfFile.QuadPart;
#else
                fi->AllocationSize.QuadPart = (ULONGLONG) FileHandle->file.st.st_blocks * 512;
#endif
                fi->NumberOfLinks           = FileHandle->file.st.st_nlink;
            }

            IoStatusBlock->Information = sizeof(FILE_STANDARD_INFORMATION);
            IoStatusBlock->u.Status = STATUS_SUCCESS;
            break;
        }

        default:
            Log(" !!UNIMPLEMENTED!!");
            IoStatusBlock->u.Status = STATUS_NOT_IMPLEMENTED;
    }

    Log("\n");

    return IoStatusBlock->u.Status;
}

NTSTATUS NTAPI NtSetInformationFile(HANDLE FileHandle, PIO_STATUS_BLOCK IoStatusBlock, PVOID FileInformation,
    ULONG Length, FILE_INFORMATION_CLASS FileInformationClass)
{
    CHECK_HANDLE(FileHandle, HANDLE_FILE);
    CHECK_POINTER(IoStatusBlock);

    Log("ntdll.NtSetInformationFile(\"%s\", %s:0x%08x)", strhandle(FileHandle), strfileinfo(FileInformationClass),
        FileInformationClass);

    switch (FileInformationClass)
    {
        case FileBasicInformation:
        {
            if (Length < sizeof(FILE_BASIC_INFORMATION))
            {
                IoStatusBlock->u.Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            /* FILE_BASIC_INFORMATION *fi = FileInformation; */
            /* looks like it passes to me data I've filled in NtQuerySystemInformation,
               but with different FileAttributes */

            IoStatusBlock->u.Status = STATUS_SUCCESS;
            break;
        }

        default:
            Log(" !!UNIMPLEMENTED!!");
            IoStatusBlock->u.Status = STATUS_NOT_IMPLEMENTED;
    }

    Log("\n");

    return IoStatusBlock->u.Status;
}

NTSTATUS NTAPI NtQueryVolumeInformationFile(HANDLE FileHandle, PIO_STATUS_BLOCK IoStatusBlock,
    PVOID FileSystemInformation, ULONG Length, FS_INFORMATION_CLASS FileSystemInformationClass)
{
    CHECK_HANDLE(FileHandle, HANDLE_FILE);
    CHECK_POINTER(IoStatusBlock);

    Log("ntdll.NtQueryVolumeInformationFile(\"%s\", %s:0x%08x)", strhandle(FileHandle),
        strfsinfo(FileSystemInformationClass), FileSystemInformationClass);

    switch (FileSystemInformationClass)
    {
        case FileFsDeviceInformation:
        {
            FILE_FS_DEVICE_INFORMATION *fsi = FileSystemInformation;

            if (Length < sizeof(FILE_FS_DEVICE_INFORMATION))
            {
                IoStatusBlock->u.Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            fsi->DeviceType = FILE_DEVICE_DISK;
            fsi->Characteristics = 0;

            IoStatusBlock->Information = sizeof(FILE_FS_DEVICE_INFORMATION);
            IoStatusBlock->u.Status = STATUS_SUCCESS;
            break;
        }

        default:
            Log(" !!UNIMPLEMENTED!!");
            IoStatusBlock->u.Status = STATUS_NOT_IMPLEMENTED;
    }

    Log("\n");

    return IoStatusBlock->u.Status;
}

#define CTL_CHECK_SIZE(size) \
    if (OutputBufferLength < size) return (IoStatusBlock->u.Status = STATUS_BUFFER_TOO_SMALL)
#define CTL_CHECK_SIZE_TYPE(type) CTL_CHECK_SIZE(sizeof(type))

NTSTATUS NTAPI NtFsControlFile(HANDLE FileHandle, HANDLE Event, PIO_APC_ROUTINE ApcRoutine,
    PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, ULONG FsControlCode, PVOID InputBuffer,
    ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength)
{
    CHECK_HANDLE(FileHandle, HANDLE_FILE);
    CHECK_POINTER(IoStatusBlock);

#ifdef REDIR_SYSCALL
    {
        NTSTATUS res = ftbl.nt.NtFsControlFile(FileHandle->file.fh, Event, ApcRoutine, ApcContext,
            IoStatusBlock, FsControlCode, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength);

        Log("ntdll.NtFsControlFile(handle:%s, %s:0x%08x, %d) = 0x%08x\n", strhandle(FileHandle), strctl(FsControlCode),
            FsControlCode, OutputBufferLength, res);

        return res;
    }
#endif

    Log("ntdll.NtFsControlFile(handle:%s, %s:0x%08x, %d)\n", strhandle(FileHandle), strctl(FsControlCode),
        FsControlCode, OutputBufferLength);

    switch (FsControlCode)
    {
        case FSCTL_LOCK_VOLUME:             /* 0x00090018 */
        case FSCTL_UNLOCK_VOLUME:           /* 0x0009001c */
        case FSCTL_DISMOUNT_VOLUME:         /* 0x00090020 */
            IoStatusBlock->u.Status = STATUS_SUCCESS;
            break;
        /* pagedefrag for file allocation */
        case FSCTL_GET_RETRIEVAL_POINTERS:  /* 0x00090073 */
            IoStatusBlock->u.Status = STATUS_NOT_IMPLEMENTED;
            break;
        case FSCTL_IS_VOLUME_DIRTY:         /* 0x00090078 */
        {
            ULONG *dirty = OutputBuffer;
            CTL_CHECK_SIZE_TYPE(ULONG);
            *dirty = VOLUME_IS_DIRTY;
            IoStatusBlock->Information = sizeof(ULONG);
            IoStatusBlock->u.Status = STATUS_SUCCESS;
            break;
        }
        case FSCTL_ALLOW_EXTENDED_DASD_IO:  /* 0x00090083 */
            IoStatusBlock->u.Status = STATUS_SUCCESS;
            break;
        case FSCTL_QUERY_DEPENDENT_VOLUME:  /* 0x000901f0 */
        default:
            IoStatusBlock->u.Status = STATUS_INVALID_DEVICE_REQUEST;
            break;
    }

    return IoStatusBlock->u.Status;
}

static NTSTATUS StorageQueryProperty(HANDLE FileHandle, PIO_STATUS_BLOCK IoStatusBlock, PSTORAGE_PROPERTY_QUERY spq,
    PVOID OutputBuffer, ULONG OutputBufferLength)
{
    // http://msdn.microsoft.com/en-us/library/ff800839(VS.85).aspx
    Log("ntdll.StorageQueryProperty(PropertyId = %s:%d, QueryType = %s:%d, %d)\n", strspqid(spq->PropertyId), spq->PropertyId,
        strspqtype(spq->QueryType), spq->QueryType, OutputBufferLength);

    IoStatusBlock->u.Status = STATUS_NOT_IMPLEMENTED;

    if (spq->QueryType != PropertyStandardQuery)
        return IoStatusBlock->u.Status;

    switch (spq->PropertyId)
    {
        case StorageAdapterProperty:
        {
            STORAGE_ADAPTER_DESCRIPTOR *sad = OutputBuffer;

            CTL_CHECK_SIZE_TYPE(STORAGE_ADAPTER_DESCRIPTOR);

            sad->Version = sad->Size = sizeof(STORAGE_ADAPTER_DESCRIPTOR);
            sad->MaximumTransferLength = 0x10000;   // 0x20000 on my hd
            sad->MaximumPhysicalPages = 0x11;       // 0x20 on my hd
            sad->AlignmentMask = 0;                 // 1 on my hs - 0 (byte aligned), 1 (word aligned), 3 (DWORD aligned) and 7 (double DWORD aligned)
            sad->AdapterUsesPio = FALSE;            // TRUE on my hd
            sad->AdapterScansDown = FALSE;          // TRUE on my hd
            sad->CommandQueueing = FALSE;
            sad->AcceleratedTransfer = FALSE;
            sad->BusType = BusTypeUsb;              // BusTypeAta on my hd
            sad->BusMajorVersion = 1;
            sad->BusMinorVersion = 0;

            IoStatusBlock->Information = sizeof(STORAGE_ADAPTER_DESCRIPTOR);
            IoStatusBlock->u.Status = STATUS_SUCCESS;
            break;
        }
        case StorageAccessAlignmentProperty: // FIXME: run on win7 and fill values
        {
            /* STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR *saad = OutputBuffer; */
            CTL_CHECK_SIZE_TYPE(STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR);
            IoStatusBlock->u.Status = STATUS_NOT_IMPLEMENTED;
            break;
        }
        default:
            break;
    }

    return IoStatusBlock->u.Status;
}

NTSTATUS NTAPI NtDeviceIoControlFile(HANDLE FileHandle, HANDLE Event, PIO_APC_ROUTINE ApcRoutine,
    PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, ULONG IoControlCode, PVOID InputBuffer,
    ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength)
{
    CHECK_HANDLE(FileHandle, HANDLE_FILE);
    CHECK_POINTER(IoStatusBlock);


#ifdef REDIR_SYSCALL
    {
        NTSTATUS res = ftbl.nt.NtDeviceIoControlFile(FileHandle->file.fh, Event, ApcRoutine, ApcContext,
            IoStatusBlock, IoControlCode, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength);
        Log("ntdll.NtDeviceIoControlFile(handle:%s, %s:0x%08x, %d) = 0x%08x\n", strhandle(FileHandle),
            strctl(IoControlCode), IoControlCode, OutputBufferLength, res);
        return res;
    }
#endif

    Log("ntdll.NtDeviceIoControlFile(handle:%s, %s:0x%08x, %d)\n", strhandle(FileHandle),
        strctl(IoControlCode), IoControlCode, OutputBufferLength);

    IoStatusBlock->u.Status = STATUS_NOT_IMPLEMENTED;

    switch (IoControlCode)
    {
        case IOCTL_STORAGE_GET_DEVICE_NUMBER:   /* 0x002d1080 */
        {
            STORAGE_DEVICE_NUMBER *sdn = OutputBuffer;
            CTL_CHECK_SIZE_TYPE(STORAGE_DEVICE_NUMBER);

            sdn->DeviceType = FILE_DEVICE_DISK;
            sdn->DeviceNumber = 1;
            sdn->PartitionNumber = 1;
            IoStatusBlock->Information = sizeof(STORAGE_DEVICE_NUMBER);
            IoStatusBlock->u.Status = STATUS_SUCCESS;
            break;
        }
        case IOCTL_STORAGE_GET_HOTPLUG_INFO:    /* 0x002d0c14 */
        {
            STORAGE_HOTPLUG_INFO *shi = OutputBuffer;
            CTL_CHECK_SIZE_TYPE(STORAGE_HOTPLUG_INFO);

            shi->Size = sizeof(STORAGE_HOTPLUG_INFO);
            shi->MediaRemovable = TRUE;             /* FALSE = non removable   - TRUE removable */
            shi->MediaHotplug = FALSE;              /* FALSE = lockable        - TRUE = non lockable */
            shi->DeviceHotplug = TRUE;              /* FALSE = non hotplug dev - TRUE = hotplug dev */
            shi->WriteCacheEnableOverride = FALSE;  /* always FALSE (reserved) */

            IoStatusBlock->Information = sizeof(STORAGE_HOTPLUG_INFO);
            IoStatusBlock->u.Status = STATUS_SUCCESS;
            break;
        }
        case IOCTL_STORAGE_QUERY_PROPERTY:      /* 0x002d1400 */
            return StorageQueryProperty(FileHandle, IoStatusBlock, InputBuffer, OutputBuffer, OutputBufferLength);
        case IOCTL_KEYBOARD_SET_INDICATORS:     /* 0x000b0008 */
        {
            IoStatusBlock->u.Status = STATUS_SUCCESS;
            break;
        }
        case IOCTL_KEYBOARD_QUERY_INDICATORS:   /* 0x000b0040 */
        {
            KEYBOARD_INDICATOR_PARAMETERS *kip = OutputBuffer;
            CTL_CHECK_SIZE_TYPE(KEYBOARD_INDICATOR_PARAMETERS);

            kip->UnitId = 0;
            kip->LedFlags = 0;

            IoStatusBlock->Information = sizeof(KEYBOARD_INDICATOR_PARAMETERS);
            IoStatusBlock->u.Status = STATUS_SUCCESS;
            break;
        }
        case IOCTL_DISK_IS_WRITABLE:            /* 0x00070024 */
        {
            IoStatusBlock->u.Status = STATUS_SUCCESS; /* it will check for STATUS_MEDIA_WRITE_PROTECTED */
            break;
        }
        case IOCTL_DISK_VERIFY:                 /* 0x00070014 */
        {
            VERIFY_INFORMATION *vi = InputBuffer;
            Log("ntdll.NtDeviceIoControlFile(IOCTL_DISK_VERIFY, %llu, %d)\n", vi->StartingOffset.QuadPart, vi->Length);

            IoStatusBlock->Information = 0;
            IoStatusBlock->u.Status = STATUS_SUCCESS;
            break;
        }
        case IOCTL_DISK_GET_PARTITION_INFO:     /* 0x00074004 */
        {
            PARTITION_INFORMATION_EX piex;
            PARTITION_INFORMATION *pinfo = OutputBuffer;
            CTL_CHECK_SIZE_TYPE(PARTITION_INFORMATION);

            NtDeviceIoControlFile(FileHandle, NULL, NULL, NULL, IoStatusBlock, IOCTL_DISK_GET_PARTITION_INFO_EX, NULL, 0, &piex, sizeof(piex));

            pinfo->StartingOffset.QuadPart = piex.StartingOffset.QuadPart;
            pinfo->PartitionLength.QuadPart = piex.PartitionLength.QuadPart;
            pinfo->HiddenSectors =  piex.Mbr.HiddenSectors;
            pinfo->PartitionNumber = piex.PartitionNumber;
            pinfo->PartitionType = piex.Mbr.PartitionType;
            pinfo->BootIndicator = piex.Mbr.BootIndicator;
            pinfo->RecognizedPartition = piex.Mbr.RecognizedPartition;
            pinfo->RewritePartition = piex.RewritePartition;

            IoStatusBlock->Information = sizeof(PARTITION_INFORMATION);
            IoStatusBlock->u.Status = STATUS_SUCCESS;
            break;
        }
        case IOCTL_DISK_SET_PARTITION_INFO:     /* 0x0007c008 */
        {
            SET_PARTITION_INFORMATION *pinfo = InputBuffer;

            Log("ntdll.NtDeviceIoControlFile(IOCTL_DISK_SET_PARTITION_INFO, PartitionType = %d)\n", pinfo->PartitionType);

            IoStatusBlock->Information = 0;
            IoStatusBlock->u.Status = STATUS_SUCCESS;
            break;
        }
        case IOCTL_DISK_GET_PARTITION_INFO_EX:  /* 0x00070048 */
        {
            DISK_GEOMETRY geo;
            PARTITION_INFORMATION_EX *piex = OutputBuffer;
            CTL_CHECK_SIZE_TYPE(PARTITION_INFORMATION_EX);

            NtDeviceIoControlFile(FileHandle, NULL, NULL, NULL, IoStatusBlock, IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0, &geo, sizeof(geo));

            piex->PartitionStyle = PARTITION_STYLE_MBR;

            piex->PartitionNumber = 1;
            piex->RewritePartition = FALSE;

            piex->Mbr.PartitionType = PARTITION_ENTRY_UNUSED;
            piex->Mbr.BootIndicator = TRUE;
            piex->Mbr.RecognizedPartition = FALSE;
            piex->Mbr.HiddenSectors = 63;

            piex->StartingOffset.QuadPart = geo.BytesPerSector * piex->Mbr.HiddenSectors;
            piex->PartitionLength.QuadPart = FileHandle->file.st.st_size;

            IoStatusBlock->Information = sizeof(PARTITION_INFORMATION_EX);
            IoStatusBlock->u.Status = STATUS_SUCCESS;
            break;
        }
        case IOCTL_DISK_GET_DRIVE_LAYOUT_EX:    /* 0x00070050 */
        {
            DRIVE_LAYOUT_INFORMATION_EX *dlex = OutputBuffer;
            CTL_CHECK_SIZE(sizeof(DRIVE_LAYOUT_INFORMATION_EX) + sizeof(PARTITION_INFORMATION_EX));

            // dlex->Mbr has 1 field, but looks like autochk want to use also other bytes
            memset(dlex, 0, sizeof(DRIVE_LAYOUT_INFORMATION_EX));
            dlex->PartitionStyle = PARTITION_STYLE_MBR;
            dlex->PartitionCount = 4;
            dlex->Mbr.Signature = 0;

            NtDeviceIoControlFile(FileHandle, NULL, NULL, NULL, IoStatusBlock, IOCTL_DISK_GET_PARTITION_INFO_EX,
                NULL, 0, &dlex->PartitionEntry[0], sizeof(PARTITION_INFORMATION_EX));

            memset(&dlex->PartitionEntry[1], 0, sizeof(PARTITION_INFORMATION_EX) * 3);

            IoStatusBlock->Information = sizeof(DRIVE_LAYOUT_INFORMATION_EX) + sizeof(PARTITION_INFORMATION_EX);
            IoStatusBlock->u.Status = STATUS_SUCCESS;
            break;
        }
        case IOCTL_DISK_GET_LENGTH_INFO:        /* 0x0007405c */
        {
            GET_LENGTH_INFORMATION *gli = OutputBuffer;
            CTL_CHECK_SIZE_TYPE(GET_LENGTH_INFORMATION);

            gli->Length.QuadPart = FileHandle->file.st.st_size;

            IoStatusBlock->Information = sizeof(GET_LENGTH_INFORMATION);
            IoStatusBlock->u.Status = STATUS_SUCCESS;
            break;
        }
        case IOCTL_DISK_GET_DRIVE_GEOMETRY:     /* 0x00070000 */
        {
            DISK_GEOMETRY *geo = OutputBuffer;
            CTL_CHECK_SIZE_TYPE(DISK_GEOMETRY);

            geo->MediaType = RemovableMedia;
            geo->TracksPerCylinder = 255;
            geo->SectorsPerTrack = 63;
            geo->BytesPerSector = 512;

            geo->Cylinders.QuadPart = 1 + FileHandle->file.st.st_size / (geo->TracksPerCylinder * geo->SectorsPerTrack * geo->BytesPerSector);

            if (geo->Cylinders.QuadPart < 2)
            {
                fprintf(stderr, "ntdll.NtDeviceIoControlFile(IOCTL_DISK_GET_DRIVE_GEOMETRY, Cylinders = %llu) - TOO SMALL!!\n", geo->Cylinders.QuadPart);
                return (IoStatusBlock->u.Status = STATUS_INVALID_PARAMETER);
            }

            Log("ntdll.NtDeviceIoControlFile(IOCTL_DISK_GET_DRIVE_GEOMETRY, Cylinders = %llu)\n", geo->Cylinders.QuadPart);

            IoStatusBlock->Information = sizeof(DISK_GEOMETRY);
            IoStatusBlock->u.Status = STATUS_SUCCESS;
            break;
        }
        case IOCTL_MOUNTDEV_QUERY_DEVICE_NAME:  /* 0x004d0008 */
        {
            MOUNTDEV_NAME *mdev = OutputBuffer;
            SIZE_T datalen = sizeof(MOUNTDEV_NAME) + sizeof(systempartition);
            CTL_CHECK_SIZE(datalen);

            memcpy(mdev->Name, systempartition, sizeof(systempartition));
            mdev->NameLength = sizeof(systempartition);

            IoStatusBlock->Information = datalen;
            IoStatusBlock->u.Status = STATUS_SUCCESS;
            break;
        }
        case IOCTL_DISK_GET_MEDIA_TYPES:        /* 0x00070c00 */
            return (IoStatusBlock->u.Status = STATUS_NOT_IMPLEMENTED);
        default:
            break;
    }

    if (IoStatusBlock->u.Status == STATUS_NOT_IMPLEMENTED)
    {
        fprintf(stderr, "ntdll.NtDeviceIoControlFile(%s:0x%08x, %d) !!UNIMPLEMENTED!!\n", strctl(IoControlCode), IoControlCode, OutputBufferLength);
        abort();
    }

    return IoStatusBlock->u.Status;
}
