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
#include <assert.h>

MODULE(heap)

#ifdef DEBUG_HEAP
#define Debug(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)
#else
#define Debug(...)
#endif

#define MAX_HEAP_ALLOC  1073741824
#define HEAPALLOC_MAGIC 0xFEFAF0F0
#define HEAPFREE_MAGIC (~HEAPALLOC_MAGIC)

#define HEAP_OFFMAGIC(x) ((uint32_t *) x)[-2]
#define HEAP_OFFSIZE(x)  ((uint32_t *) x)[-1]

PVOID NTAPI RtlCreateHeap(ULONG Flags, PVOID HeapBase, SIZE_T ReserveSize, SIZE_T CommitSize, PVOID Lock, /*PRTL_HEAP_PARAMETERS */ LPVOID Parameters)
{
    Log("ntdll.RtlCreateHeap(0x%08x, %p, %d, %d, %p, %p)\n", (int) Flags, HeapBase, ReserveSize, CommitSize, Lock, Parameters);
    return HANDLE_HEAP;
}

PVOID NTAPI RtlDestroyHeap(PVOID HeapHandle)
{
    Log("ntdll.RtlDestroyHeap(%p)\n", HeapHandle);
    return NULL;
}

PVOID NTAPI RtlAllocateHeap(PVOID HeapHandle, ULONG Flags, SIZE_T Size)
{
    PVOID MemoryPointer;

    assert(Size < MAX_HEAP_ALLOC);

    MemoryPointer = malloc(Size + (sizeof(uint32_t) * 2));
    Debug("ntdll.RtlAllocateHeap(%p, 0x%08x, %d)=%p -> ", HeapHandle, (int) Flags, Size, MemoryPointer);

    assert(MemoryPointer);

    MemoryPointer = (PVOID)((uint32_t *) MemoryPointer + 2);
    HEAP_OFFMAGIC(MemoryPointer) = HEAPALLOC_MAGIC;
    HEAP_OFFSIZE(MemoryPointer) = Size;

    if (Flags & HEAP_ZERO_MEMORY)
        memset(MemoryPointer, 0, Size);

    Debug("%p\n", MemoryPointer);
    return MemoryPointer;
}

PVOID NTAPI RtlReAllocateHeap(PVOID HeapHandle, ULONG Flags, PVOID MemoryPointer, SIZE_T Size)
{
    SIZE_T OldSize;

    assert(Size < MAX_HEAP_ALLOC);

    if (!MemoryPointer)
        return RtlAllocateHeap(HeapHandle, Flags, Size);

    OldSize = HEAP_OFFSIZE(MemoryPointer);
    MemoryPointer = realloc((PVOID) HEAP_OFFSIZE(MemoryPointer), Size + (sizeof(uint32_t) * 2));

    assert(MemoryPointer);

    MemoryPointer = (PVOID)((uint32_t *) MemoryPointer + 2);
    HEAP_OFFMAGIC(MemoryPointer) = HEAPALLOC_MAGIC;
    HEAP_OFFSIZE(MemoryPointer) = Size;

    Log(" -> %p\n", MemoryPointer);

    if ((Size > OldSize) && (Flags & HEAP_ZERO_MEMORY))
        memset((char *) MemoryPointer + OldSize, 0, Size - OldSize);

    return MemoryPointer;
}

SIZE_T NTAPI RtlSizeHeap(PVOID HeapHandle, ULONG Flags, LPCVOID MemoryPointer)
{
    assert(HEAP_OFFMAGIC(MemoryPointer) == HEAPALLOC_MAGIC);

    Debug("ntdll.RtlSizeHeap(%p, 0x%08x, %p) = %d\n", HeapHandle, (int) Flags, MemoryPointer, HEAP_OFFSIZE(MemoryPointer));
    return HEAP_OFFSIZE(MemoryPointer);
}

BOOLEAN NTAPI RtlFreeHeap(PVOID HeapHandle, ULONG Flags, PVOID MemoryPointer)
{
    assert(HEAP_OFFMAGIC(MemoryPointer) == HEAPALLOC_MAGIC);

    Debug("ntdll.RtlFreeHeap(%p, 0x%08x, %p)\n", HeapHandle, (int) Flags, MemoryPointer);

    HEAP_OFFMAGIC(MemoryPointer) = HEAPFREE_MAGIC;
    MemoryPointer = (PVOID)((uint32_t *) MemoryPointer - 2);
    free(MemoryPointer);

    return TRUE;
}
