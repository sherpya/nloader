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

char *strhandle(HANDLE Handle)
{
    if (!Handle)
        return "!! NULL HANDLE !!";
    if (Handle->cookie == HANDLE_COOKIE)
        return Handle->desc;
    return "Not my own";
}

const char *strctl(ULONG ControlCode)
{
    switch (ControlCode)
    {
        case FSCTL_LOCK_VOLUME:
            return "FSCTL_LOCK_VOLUME";
        case FSCTL_UNLOCK_VOLUME:
            return "FSCTL_UNLOCK_VOLUME";
        case FSCTL_DISMOUNT_VOLUME:
            return "FSCTL_DISMOUNT_VOLUME";
        case FSCTL_GET_RETRIEVAL_POINTERS:
            return "FSCTL_GET_RETRIEVAL_POINTERS";
        case FSCTL_IS_VOLUME_DIRTY:
            return "FSCTL_IS_VOLUME_DIRTY";
        case FSCTL_QUERY_DEPENDENT_VOLUME:
            return "FSCTL_QUERY_DEPENDENT_VOLUME";
        case FSCTL_ALLOW_EXTENDED_DASD_IO:
            return "FSCTL_ALLOW_EXTENDED_DASD_IO";
        case IOCTL_STORAGE_GET_HOTPLUG_INFO:
            return "IOCTL_STORAGE_GET_HOTPLUG_INFO";
        case IOCTL_STORAGE_GET_DEVICE_NUMBER:
            return "IOCTL_STORAGE_GET_DEVICE_NUMBER";
        case IOCTL_STORAGE_QUERY_PROPERTY:
            return "IOCTL_STORAGE_QUERY_PROPERTY";
        case IOCTL_KEYBOARD_SET_INDICATORS:
            return "IOCTL_KEYBOARD_SET_INDICATORS";
        case IOCTL_KEYBOARD_QUERY_INDICATORS:
            return "IOCTL_KEYBOARD_QUERY_INDICATORS";
        case IOCTL_DISK_GET_PARTITION_INFO:
            return "IOCTL_DISK_GET_PARTITION_INFO";
        case IOCTL_DISK_SET_PARTITION_INFO:
            return "IOCTL_DISK_SET_PARTITION_INFO";
        case IOCTL_DISK_GET_DRIVE_LAYOUT:
            return "IOCTL_DISK_GET_DRIVE_LAYOUT";
        case IOCTL_DISK_GET_DRIVE_GEOMETRY:
            return "IOCTL_DISK_GET_DRIVE_GEOMETRY";
        case IOCTL_DISK_IS_WRITABLE:
            return "IOCTL_DISK_IS_WRITABLE";
        case IOCTL_DISK_GET_PARTITION_INFO_EX:
            return "IOCTL_DISK_GET_PARTITION_INFO_EX";
        case IOCTL_DISK_VERIFY:
            return "IOCTL_DISK_VERIFY";
        case IOCTL_DISK_GET_DRIVE_LAYOUT_EX:
            return "IOCTL_DISK_GET_DRIVE_LAYOUT_EX";
        case IOCTL_DISK_GET_LENGTH_INFO:
            return "IOCTL_DISK_GET_LENGTH_INFO";
        case IOCTL_DISK_GET_MEDIA_TYPES:
            return "IOCTL_DISK_GET_MEDIA_TYPES";
        case IOCTL_MOUNTDEV_QUERY_DEVICE_NAME:
            return "IOCTL_MOUNTDEV_QUERY_DEVICE_NAME";
        default:
            return "CTL_UNKNOWN";
    }
}

const char *strsysinfo(SYSTEM_INFORMATION_CLASS infoclass)
{
    switch (infoclass)
    {
        case SystemBasicInformation:
            return "SystemBasicInformation";
        case SystemPerformanceInformation:
            return "SystemPerformanceInformation";
        case SystemTimeOfDayInformation:
            return "SystemTimeOfDayInformation";
        default:
            return "Unknown_System_Info_Class";
    }
}

const char *strfileinfo(FILE_INFORMATION_CLASS infoclass)
{
    switch (infoclass)
    {
        case FileAlignmentInformation:
            return "FileAlignmentInformation";
        case FileBasicInformation:
            return "FileBasicInformation";
        case FileStandardInformation:
            return "FileStandardInformation";
        default:
            return "Unknown_File_Info_Class";
    }
}

const char *strfsinfo(FS_INFORMATION_CLASS infoclass)
{
    switch (infoclass)
    {
        case FileFsDeviceInformation:
            return "FileFsDeviceInformation";
        default:
            return "Unknown_Fs_Info_Class";
    }
}

const char *strthinfo(THREADINFOCLASS infoclass)
{
    switch (infoclass)
    {
        case ThreadBasicInformation:
            return "ThreadBasicInformation";
        default:
            return "Unknown_Thread_Info_Class";
    }
}

const char *strspqid(STORAGE_PROPERTY_ID PropertyId)
{
    switch (PropertyId)
    {
        case StorageDeviceProperty:
            return "StorageDeviceProperty";
        case StorageAdapterProperty:
            return "StorageAdapterProperty";
        case StorageDeviceIdProperty:
            return "StorageDeviceIdProperty";
        case StorageDeviceUniqueIdProperty:
            return "StorageDeviceUniqueIdProperty";
        case StorageDeviceWriteCacheProperty:
            return "StorageDeviceWriteCacheProperty";
        case StorageMiniportProperty:
            return "StorageMiniportProperty";
        case StorageAccessAlignmentProperty:
            return "StorageAccessAlignmentProperty";
        case StorageDeviceSeekPenaltyProperty:
            return "StorageDeviceSeekPenaltyProperty";
        case StorageDeviceTrimProperty:
            return "StorageDeviceTrimProperty";
        default:
            return "Unknown_Storage_PropertyId";
    }
}

const char *strspqtype(STORAGE_QUERY_TYPE QueryType)
{
    switch (QueryType)
    {
        case PropertyStandardQuery:
            return "PropertyStandardQuery";
        case PropertyExistsQuery:
            return "PropertyExistsQuery";
        case PropertyMaskQuery:
            return "PropertyMaskQuery";
        case PropertyQueryMaxDefined:
            return "PropertyQueryMaxDefined";
        default:
            return "Unknown_Storage_QueryType";
    }
}
