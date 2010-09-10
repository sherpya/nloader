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

MODULE(nt)

static void fill_sysinfo(SYSTEM_BASIC_INFORMATION *info)
{
    info->unknown = 0;
    info->KeMaximumIncrement = 0; /* 0x2625a */
    info->PageSize = getpagesize();
    info->MmLowestPhysicalPage = 0x1;
    info->MmHighestPhysicalPage = 0x7fffffff / getpagesize();
    info->MmNumberOfPhysicalPages = info->MmHighestPhysicalPage - info->MmLowestPhysicalPage; /* 0xefee5 */
    info->AllocationGranularity = 0x10000;
    info->LowestUserAddress = (PVOID) 0x10000;
    info->HighestUserAddress = (PVOID) 0x7ffeffff;
    info->ActiveProcessorsAffinityMask = (1 << 1) - 1;
    info->NumberOfProcessors = 1;
}

static void fill_perfinfo(SYSTEM_PERFORMANCE_INFORMATION *spi)
{
    spi->AvailablePages = 0x20000;

/*
    spi->Reserved1 = 0x0;
    spi->Reserved2[0] = 0;
    spi->Reserved2[1] = 0;
    spi->Reserved3 = 0x7fffffff;
    spi->SystemCodePage = 200;
*/
}

NTSTATUS NTAPI NtQuerySystemInformation(SYSTEM_INFORMATION_CLASS SystemInformationClass, PVOID SystemInformation,
    ULONG SystemInformationLength, PULONG ReturnLength)
{
    NTSTATUS result = STATUS_BUFFER_TOO_SMALL;

    Log("ntdll.NtQuerySystemInformation(%s:0x%08x)", strsysinfo(SystemInformationClass), SystemInformationClass);

    switch (SystemInformationClass)
    {
        case SystemBasicInformation:
        {
            if (ReturnLength)
                *ReturnLength = sizeof(SYSTEM_BASIC_INFORMATION);

            if (SystemInformationLength < sizeof(SYSTEM_BASIC_INFORMATION))
                break;

            fill_sysinfo(SystemInformation);
            result = STATUS_SUCCESS;
            break;
        }
        case SystemPerformanceInformation:
        {
            if (ReturnLength)
                *ReturnLength = sizeof(SYSTEM_PERFORMANCE_INFORMATION);

            if (SystemInformationLength < sizeof(SYSTEM_PERFORMANCE_INFORMATION))
                break;

            fill_perfinfo(SystemInformation);
            result = STATUS_SUCCESS;
            break;
        }
        case SystemTimeOfDayInformation:
        {
            SYSTEM_TIMEOFDAY_INFORMATION *sti = SystemInformation;

            if (ReturnLength)
                *ReturnLength = sizeof(SYSTEM_TIMEOFDAY_INFORMATION);

            if (SystemInformationLength < sizeof(SYSTEM_TIMEOFDAY_INFORMATION))
                break;

            memset(sti, 0, sizeof(SYSTEM_TIMEOFDAY_INFORMATION));
            result = STATUS_SUCCESS;
            break;
        }
        default:
            Log(" !!UNIMPLEMENTED!!");
            result = STATUS_NOT_IMPLEMENTED;
    }

    Log("\n");

    return result;
}

NTSTATUS NTAPI NtSetInformationProcess(HANDLE ProcessHandle, PROCESS_INFORMATION_CLASS ProcessInformationClass,
    PVOID ProcessInformation, ULONG ProcessInformationLength)
{
    return STATUS_SUCCESS;
}
FORWARD_FUNCTION(NtSetInformationProcess, ZwSetInformationProcess);

NTSTATUS NTAPI NtTerminateProcess(HANDLE ProcessHandle, NTSTATUS ExitStatus)
{
    Log("ntdll.NtTerminateProcess(%p, 0x%08x)\n", ProcessHandle, ExitStatus);
    exit(ExitStatus);
}

NTSTATUS NTAPI NtDisplayString(PUNICODE_STRING String)
{
    DECLAREVARCONV(StringA);
    CHECK_POINTER(String);

    US2STR(String);
    fputs(StringA, stdout);

    return STATUS_SUCCESS;
}

VOID NTAPI DbgBreakPoint(void)
{
    fprintf(stderr, "ntdll.DbgBreakPoint()\n");
    INT3();
}

NTSTATUS NTAPI NtShutdownSystem(SHUTDOWN_ACTION Action)
{
    fprintf(stderr, "ntdll.NtShutdownSystem(%d)\n", Action);
    exit(0);
}

NTSTATUS NTAPI LdrSetMUICacheType(ULONG Type)
{
    Log("ntdll.LdrSetMUICacheType(0x%08x)\n", Type);
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI NtSerializeBoot(VOID)
{
    Log("ntdll.NtSerializeBoot()\n");
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI NtOpenSection(PHANDLE SectionHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes)
{
    DECLAREVARCONV(ObjectAttributesA);
    OA2STR(ObjectAttributes);
    Log("ntdll.NtOpenSection(\"%s\")\n", ObjectAttributesA);

    return STATUS_ACCESS_DENIED;
}
FORWARD_FUNCTION(NtOpenSection, ZwOpenSection);

NTSTATUS NTAPI LdrGetDllHandle(PWORD pwPath, PVOID Unused, PUNICODE_STRING ModuleFileName, PHANDLE pHModule)
{
    DECLAREVARCONV(ModuleFileNameA);
    CHECK_POINTER(ModuleFileName);

    US2STR(ModuleFileName);
    Log("ntdll.LdrGetDllHandle(\"%s\")\n", ModuleFileNameA);

    return STATUS_INVALID_HANDLE;
}

NTSTATUS NTAPI NtLoadDriver(IN PUNICODE_STRING DriverServiceName)
{
    DECLAREVARCONV(DriverServiceNameA);
    CHECK_POINTER(DriverServiceName);

    US2STR(DriverServiceName);
    Log("ntdll.NtLoadDriver(\"%s\")\n", DriverServiceNameA);

    return STATUS_SUCCESS;
}
FORWARD_FUNCTION(NtLoadDriver, ZwLoadDriver);

NTSTATUS NTAPI NtOpenProcessToken(HANDLE ProcessHandle, ACCESS_MASK DesiredAccess, PHANDLE TokenHandle)
{
    Log("NtOpenProcessToken() - !!UNIMPLEMENTED!!\n");
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS NTAPI NtAdjustPrivilegesToken(HANDLE TokenHandle, BOOLEAN DisableAllPrivileges,
    PVOID /*PTOKEN_PRIVILEGES*/ TokenPrivileges, ULONG PreviousPrivilegesLength,
    PVOID /*PTOKEN_PRIVILEGES*/ PreviousPrivileges, PULONG RequiredLength)
{
    Log("ntdll.NtAdjustPrivilegesToken()\n");
    return STATUS_SUCCESS;
}
