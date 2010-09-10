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

MODULE(mm)

// FIXME: untested
NTSTATUS NTAPI NtAllocateVirtualMemory(HANDLE ProcessHandle, PVOID *BaseAddress,
    ULONG_PTR ZeroBits, PSIZE_T RegionSize, ULONG AllocationType, ULONG Protect)
{
    int prot, flags = MAP_PRIVATE | MAP_ANON;
    SIZE_T size;
    PVOID address;

    if (!*RegionSize)
        return STATUS_INVALID_PARAMETER;

    switch (Protect)
    {
        case PAGE_READONLY:
            prot = PROT_READ;
            break;
        case PAGE_READWRITE:
            prot = PROT_READ | PROT_WRITE;
            break;
        case PAGE_EXECUTE:
            prot = PROT_EXEC;
            break;
        default:
            fprintf(stderr, "ntdll.NtAllocateVirtualMemory() unknown protection 0x%8x\n", Protect);
            abort();
    }

    size = ROUND_TO_PAGES(*RegionSize);
    address = *BaseAddress ? PAGE_ALIGN(*BaseAddress) : NULL;

    if (address) flags |= MAP_FIXED;
    address = mmap(address, size, prot, flags, -1, 0);
    *RegionSize = size;

    Log("ntdll.NtAllocateVirtualMemory(%d, %p, 0x%08x, %d, 0x%08x, 0x%08x) = %p/%d\n", (int) ProcessHandle,
        *BaseAddress, ZeroBits, *RegionSize, AllocationType, Protect, address, size);

    if (address)
    {
        *BaseAddress = address;
        return STATUS_SUCCESS;
    }

    return STATUS_INSUFFICIENT_RESOURCES;
}
FORWARD_FUNCTION(NtAllocateVirtualMemory, ZwAllocateVirtualMemory);

NTSTATUS NTAPI NtFreeVirtualMemory(HANDLE ProcessHandle, PVOID *BaseAddress, PSIZE_T RegionSize, ULONG FreeType)
{
    Log("ntdll.NtFreeVirtualMemory(%d, %p, %d, 0x%08x)\n", (int) ProcessHandle, *BaseAddress, *RegionSize, FreeType);

    if (munmap(*BaseAddress, ROUND_TO_PAGES(*RegionSize)) < 0)
        return STATUS_UNSUCCESSFUL;
    else
        return STATUS_SUCCESS;
}
FORWARD_FUNCTION(NtFreeVirtualMemory, ZwFreeVirtualMemory);
