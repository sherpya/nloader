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

/*
 * Placeholder bcd.dll. Logs every call and returns STATUS_OBJECT_NAME_NOT_FOUND
 * for element lookups so the caller falls through to its default. Pattern
 * mirrors libs/ntdll/registry.c — populate the (GUID, ElementType) → data
 * table below as autochk's BCD usage surfaces in the smoke logs.
 *
 * Real BCD parsing (loading \Boot\BCD as a hive) is out of scope: nloader is
 * not booting Windows, so the values it serves can be synthesized.
 */

#include "../ntdll/ntdll.h"

MODULE(bcd)

#define HANDLE_BCDS 'B'  /* BCD store  */
#define HANDLE_BCDO 'O'  /* BCD object */

static HANDLE bcd_alloc_handle(char type, const char *name)
{
    HANDLE h = calloc(1, sizeof(struct _HANDLE));
    if (!h) return NULL;
    h->cookie = HANDLE_COOKIE;
    h->type = type;
    strncpy(h->name, name, sizeof(h->name) - 1);
    snprintf(h->desc, sizeof(h->desc), "%c:%s (by bcd)", type, name);
    return h;
}

static void format_guid(char *out, size_t outlen, const GUID *g)
{
    snprintf(out, outlen,
        "{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
        (unsigned long) g->Data1, g->Data2, g->Data3,
        g->Data4[0], g->Data4[1], g->Data4[2], g->Data4[3],
        g->Data4[4], g->Data4[5], g->Data4[6], g->Data4[7]);
}

/* autochk x86_64 (Win10+) calls BcdOpenStore with three args: (PCWSTR szFile,
 * ULONG flags, PHANDLE phStore). The MSDN proto is two-arg, but the binary
 * places phStore in R8 — we match the actual ABI of the caller, not the doc. */
/* szFile is logged as a raw pointer rather than converted: bcd.dll.so is
 * loaded with RTLD_LOCAL|RTLD_DEEPBIND, so RtlUnicodeToOemN/rpl_wcslen from
 * libs/ntdll.dll.so are not resolvable here. autochk passes szFile=NULL
 * anyway; expand to a real conversion if a caller starts using a path. */
NTSTATUS WINAPI BcdOpenStore(PCWSTR szFile, ULONG flags, PHANDLE phStore)
{
    char namebuf[64];

    CHECK_POINTER(phStore);

    if (szFile)
        snprintf(namebuf, sizeof(namebuf), "<file@%p>", (void *) szFile);
    else
        snprintf(namebuf, sizeof(namebuf), "<system>");

    Log("bcd.BcdOpenStore(%s, 0x%lx)\n", namebuf, (unsigned long) flags);

    *phStore = bcd_alloc_handle(HANDLE_BCDS, namebuf);
    return *phStore ? STATUS_SUCCESS : STATUS_NO_MEMORY;
}

NTSTATUS WINAPI BcdCloseStore(HANDLE hStore)
{
    CHECK_HANDLE(hStore, HANDLE_BCDS);
    Log("bcd.BcdCloseStore(\"%s\")\n", hStore->name);
    free(hStore);
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI BcdForciblyUnloadStore(HANDLE hStore)
{
    CHECK_HANDLE(hStore, HANDLE_BCDS);
    Log("bcd.BcdForciblyUnloadStore(\"%s\")\n", hStore->name);
    free(hStore);
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI BcdOpenObject(HANDLE hStore, GUID *Identifier, PHANDLE phObject)
{
    char guid[40] = "(nil)";

    CHECK_HANDLE(hStore, HANDLE_BCDS);
    CHECK_POINTER(Identifier);
    CHECK_POINTER(phObject);

    format_guid(guid, sizeof(guid), Identifier);
    Log("bcd.BcdOpenObject(\"%s\", %s)\n", hStore->name, guid);

    *phObject = bcd_alloc_handle(HANDLE_BCDO, guid);
    return *phObject ? STATUS_SUCCESS : STATUS_NO_MEMORY;
}

NTSTATUS WINAPI BcdCloseObject(HANDLE hObject)
{
    CHECK_HANDLE(hObject, HANDLE_BCDO);
    Log("bcd.BcdCloseObject(\"%s\")\n", hObject->name);
    free(hObject);
    return STATUS_SUCCESS;
}

/* Element type encoding: top byte = format (0x1=device, 0x2=string, 0x3=objid,
 * 0x4=objidlist, 0x5=int, 0x6=bool, 0x7=intlist), next byte = class
 * (0x1=Library, 0x2=Application, 0x4=OEM), low 16 = subtype. So 0x1600006C is
 * OS Loader Boolean #0x6C. autochk reads it as a 16-bit value and only checks
 * `v != 0`; returning NOT_FOUND keeps the caller's default (0 / FALSE). */
NTSTATUS WINAPI BcdGetElementData(HANDLE hObject, ULONG ElementType,
                                   PVOID Buffer, PULONG BufferSize)
{
    CHECK_HANDLE(hObject, HANDLE_BCDO);
    CHECK_POINTER(BufferSize);

    Log("bcd.BcdGetElementData(\"%s\", 0x%08lx, size=%lu) - Not Found\n",
        hObject->name, (unsigned long) ElementType,
        (unsigned long) *BufferSize);

    return STATUS_OBJECT_NAME_NOT_FOUND;
}
