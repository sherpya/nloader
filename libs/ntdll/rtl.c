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
#include <stdarg.h>
#include <ctype.h>
#include "upcasetable.h"

MODULE(rtl)

PRTL_USER_PROCESS_PARAMETERS NTAPI RtlNormalizeProcessParams(PRTL_USER_PROCESS_PARAMETERS Params)
{
    if (Params && !(Params->Flags & RTL_USER_PROCESS_PARAMETERS_NORMALIZED))
    {
        NORMALIZE(Params->CurrentDirectory.DosPath.Buffer, Params);
        NORMALIZE(Params->DllPath.Buffer, Params);
        NORMALIZE(Params->ImagePathName.Buffer, Params);
        NORMALIZE(Params->CommandLine.Buffer, Params);
        NORMALIZE(Params->WindowTitle.Buffer, Params);
        NORMALIZE(Params->Desktop.Buffer, Params);
        NORMALIZE(Params->ShellInfo.Buffer, Params);
        NORMALIZE(Params->RuntimeInfo.Buffer, Params);
        Params->Flags |= RTL_USER_PROCESS_PARAMETERS_NORMALIZED;
    }
    return Params;
}

NTSTATUS NTAPI RtlUnicodeStringToAnsiString(PANSI_STRING DestinationString,
    PUNICODE_STRING SourceString, BOOLEAN AllocateDestinationString)
{
    int i;
    USHORT len = SourceString->Length / sizeof(WCHAR);
    USHORT alloc = SourceString->MaximumLength / sizeof(WCHAR);

    if (AllocateDestinationString)
        DestinationString->Buffer = RtlAllocateHeap(HANDLE_HEAP, 0, alloc);

    DestinationString->Length = len;
    DestinationString->MaximumLength = alloc;

    for (i = 0; i < len; i++)
        DestinationString->Buffer[i] = widetoa(SourceString->Buffer[i]);
    DestinationString->Buffer[i] = 0;

    Log("ntdll.RtlUnicodeStringToAnsiString(Alloc=%p) = \"%s\"\n", DestinationString->Buffer,
        DestinationString->Buffer);

    return STATUS_SUCCESS;
}

NTSTATUS NTAPI RtlAnsiStringToUnicodeString(PUNICODE_STRING DestinationString,
    PCANSI_STRING SourceString, BOOLEAN AllocateDestinationString)
{
    int i;
    USHORT len = SourceString->Length;
    USHORT alloc = SourceString->MaximumLength * sizeof(WCHAR);

    if (AllocateDestinationString)
        DestinationString->Buffer = RtlAllocateHeap(HANDLE_HEAP, 0, alloc);

    DestinationString->Length = len * sizeof(WCHAR);
    DestinationString->MaximumLength = alloc;

    for (i = 0; i < len; i++)
        DestinationString->Buffer[i] = SourceString->Buffer[i];
    DestinationString->Buffer[i] = 0;

    Log("ntdll.RtlAnsiStringToUnicodeString(\"%s\", %d)\n", SourceString->Buffer,
        AllocateDestinationString);

    return STATUS_SUCCESS;
}

WCHAR NTAPI RtlAnsiCharToUnicodeChar(PUCHAR *SourceCharacter)
{
    WCHAR unichar = L' ';
    if ((**SourceCharacter & 0x80) == 0)
        unichar = **SourceCharacter;

    Log("ntdll.RtlAnsiCharToUnicodeChar('%c')\n", **SourceCharacter);
    return unichar;
}

VOID NTAPI RtlFreeAnsiString(PANSI_STRING AnsiString)
{
    if (RtlSizeHeap(HANDLE_HEAP, 0, AnsiString->Buffer) != AnsiString->MaximumLength)
        Log("ntdll.RtlFreeAnsiString(%p) -> PTR SIZE MISMATCH\n", AnsiString->Buffer);
    else
    {
        Log("ntdll.RtlFreeAnsiString(%p)\n", AnsiString->Buffer);
        RtlFreeHeap(HANDLE_HEAP, 0, AnsiString->Buffer);
    }
}

NTSTATUS NTAPI RtlUpcaseUnicodeString(PUNICODE_STRING DestinationString,
    PCUNICODE_STRING SourceString, BOOLEAN AllocateDestinationString)
{
    USHORT i, len = SourceString->Length;

    if (AllocateDestinationString)
    {
        DestinationString->MaximumLength = len;
        if (!(DestinationString->Buffer = RtlAllocateHeap(HANDLE_HEAP, 0, len)))
                return STATUS_NO_MEMORY;
    }
    else if (len > DestinationString->MaximumLength)
        return STATUS_BUFFER_OVERFLOW;

    for (i = 0; i < len / sizeof(WCHAR); i++)
        DestinationString->Buffer[i] = UpcaseTable[SourceString->Buffer[i]];

    DestinationString->Length = len;

    Log("ntdll.RtlUpcaseUnicodeString(%p, %p, %d)\n", SourceString->Buffer, DestinationString->Buffer, AllocateDestinationString);

    return STATUS_SUCCESS;
}

NTSTATUS NTAPI RtlFindMessage(HMODULE hmod, ULONG type, ULONG lang, ULONG msg_id, const MESSAGE_RESOURCE_ENTRY **ret)
{
    ULONG_PTR root;
    struct pe_image_optional_hdr32 *pe_opt;
    const IMAGE_RESOURCE_DIRECTORY *rd;
    const IMAGE_RESOURCE_DATA_ENTRY *rde;
    DWORD pe_off;
    unsigned int level = 1, i = 0, len;

    CHECK_POINTER(ret);

    Log("ntdll.RtlFindMessage(LANG=%d, ID=%d) = ", lang, msg_id);

    *ret = NULL;
    pe_off = hmod + (*(DWORD *)(hmod + 0x3c));
    pe_opt = (struct pe_image_optional_hdr32 *) (pe_off + sizeof(struct pe_image_file_hdr));

    if (pe_opt->DataDirectory[2].Size == 0)
    {
        Log("Not Resource block found\n");
        return STATUS_MESSAGE_NOT_FOUND;
    }

    root = hmod + pe_opt->DataDirectory[2].VirtualAddress;
    rd = (const IMAGE_RESOURCE_DIRECTORY *) root;
    rde = NULL;

    /* RT_MESSAGETABLE -> 1 (NAME) -> 1033 (LANG) -> messagetable */
    len = rd->NumberOfNamedEntries + rd->NumberOfIdEntries;
    do
    {
        const IMAGE_RESOURCE_DIRECTORY_ENTRY *cur = &rd->DirectoryEntries[i++];

        if (!cur->NameIsString && cur->DataIsDirectory)
        {
            if ( ((level == 1) && (cur->Id == type)) ||
                 ((level == 2) && (cur->Id == 1)) )
            {
                rd = (const IMAGE_RESOURCE_DIRECTORY *) (root + cur->OffsetToDirectory);
                i = 0, len = rd->NumberOfNamedEntries + rd->NumberOfIdEntries;
                level++;
                continue;
            }
        }
        else if ((level == 3) && (!lang || (cur->Id == lang)))
        {
            rde = (const IMAGE_RESOURCE_DATA_ENTRY *) (root + cur->OffsetToData);
            break;
        }
    }
    while ((i < len) && (level < 4));

    if (rde)
    {
        const MESSAGE_RESOURCE_DATA *data = (const MESSAGE_RESOURCE_DATA *) (hmod + rde->OffsetToData); /* offset from hmod here? argh */
        const MESSAGE_RESOURCE_BLOCK *block = data->Blocks;

        for (i = 0; i < data->NumberOfBlocks; i++, block++)
        {
            if (msg_id >= block->LowId && msg_id <= block->HighId)
            {
                const MESSAGE_RESOURCE_ENTRY *entry;
                DECLAREVARCONV(message);

                entry = (const MESSAGE_RESOURCE_ENTRY *)((ULONG_PTR) data + block->OffsetToEntries);
                for (i = msg_id - block->LowId; i > 0; i--)
                    entry = (const MESSAGE_RESOURCE_ENTRY *)((ULONG_PTR) entry + entry->Length);
                *ret = entry;

                if ((*ret)->Flags & MESSAGE_RESOURCE_UNICODE)
                    RtlUnicodeToOemN(message, sizeof(message), NULL, (PCWCH) (*ret)->Text, (*ret)->Length);
                else
                    memcpy(message, (*ret)->Text, MIN((*ret)->Length, sizeof(message)));
                munge(message);
                Log("%p \"%s\"\n", (*ret)->Text, message);
                return STATUS_SUCCESS;
            }
        }
    }

    Log("Not found\n");

    return STATUS_MESSAGE_NOT_FOUND;
}

NTSTATUS NTAPI RtlMultiByteToUnicodeN(PWCH UnicodeString, ULONG MaxBytesInUnicodeString,
    PULONG BytesInUnicodeString, const CHAR *MultiByteString, ULONG BytesInMultiByteString)
{
    int i;
    int end = MIN(MaxBytesInUnicodeString / sizeof(WCHAR), BytesInMultiByteString);

    Log("ntdll.RtlMultiByteToUnicodeN(\"%s\", %d)\n", MultiByteString, end);

    if (end == 0)
    {
        UnicodeString[0] = 0;
        return STATUS_SUCCESS;
    }

    for (i = 0; i < end; i++)
        UnicodeString[i] = MultiByteString[i];

    if (BytesInUnicodeString)
        *BytesInUnicodeString = i * sizeof(WCHAR);

    return STATUS_SUCCESS;
}
FORWARD_FUNCTION(RtlMultiByteToUnicodeN, RtlOemToUnicodeN);

NTSTATUS NTAPI RtlUnicodeToOemN(PCHAR OemString, ULONG MaxBytesInOemString,
    PULONG BytesInOemString, PCWCH UnicodeString, ULONG BytesInUnicodeString)
{
    int i;
    int end = MIN(BytesInUnicodeString / sizeof(WCHAR), MaxBytesInOemString);

    if (!UnicodeString)
        return STATUS_ACCESS_VIOLATION;

    if (end == 0)
    {
        OemString[0] = 0;
        return STATUS_SUCCESS;
    }

    for (i = 0; i <= end; i++)
        OemString[i] = widetoa(UnicodeString[i]);

    if (BytesInOemString)
        *BytesInOemString = i;

    return STATUS_SUCCESS;
}
FORWARD_FUNCTION(RtlUnicodeToOemN, RtlUnicodeToMultiByteN);

VOID NTAPI RtlInitUnicodeString(PUNICODE_STRING DestinationString, LPCWSTR SourceString)
{
    DECLAREVARCONV(SourceStringA);
    WSTR2STR(SourceString);

    if ((DestinationString->Buffer = (PWSTR) SourceString))
    {
        DestinationString->Length = rpl_wcslen(SourceString) * sizeof(WCHAR);
        DestinationString->MaximumLength = DestinationString->Length + sizeof(WCHAR);
    }
    else
        DestinationString->Length = DestinationString->MaximumLength = 0;

    Log("ntdll.RtlInitUnicodeString(%p, \"%s\") -> %p\n", SourceString, SourceStringA, DestinationString);
}

VOID NTAPI RtlInitAnsiString(PANSI_STRING DestinationString, LPCSTR SourceString)
{
    if ((DestinationString->Buffer = (PSTR) SourceString))
    {
        DestinationString->Length = strlen(SourceString);
        DestinationString->MaximumLength = DestinationString->Length + sizeof(CHAR);
    }
    else
        DestinationString->Length = DestinationString->MaximumLength = 0;

    Log("ntdll.RtlInitAnsiString(\"%s\") -> %p\n", SourceString, DestinationString->Buffer);
}

NTSTATUS NTAPI RtlInitAnsiStringEx(PANSI_STRING DestinationString, LPCSTR SourceString)
{
    if (SourceString)
    {
        size_t len = strlen(SourceString);

        if ((len + 1) > 0xffff)
            return STATUS_NAME_TOO_LONG;

        DestinationString->Buffer = (PSTR) SourceString;
        DestinationString->Length = len;
        DestinationString->MaximumLength = len + 1;
    }
    else
    {
        DestinationString->Buffer = NULL;
        DestinationString->Length = DestinationString->MaximumLength = 0;
    }

    Log("ntdll.RtlInitAnsiStringEx(\"%s\") -> %p\n", SourceString, DestinationString->Buffer);

    return STATUS_SUCCESS;
}

VOID NTAPI RtlFreeUnicodeString(PUNICODE_STRING UnicodeString)
{
    if (RtlSizeHeap(HANDLE_HEAP, 0, UnicodeString->Buffer) != UnicodeString->MaximumLength)
        Log("ntdll.RtlFreeUnicodeString(%p) -> PTR SIZE MISMATCH\n", UnicodeString->Buffer);
    else
    {
        Log("ntdll.RtlFreeUnicodeString(%p)\n", UnicodeString->Buffer);
        RtlFreeHeap(HANDLE_HEAP, 0, UnicodeString->Buffer);
    }
}

BOOLEAN NTAPI RtlPrefixUnicodeString(PUNICODE_STRING String1, PUNICODE_STRING String2, BOOLEAN CaseInSensitive)
{
    int i;
    BOOLEAN res = TRUE;
    DECLAREVARCONV(String1A);
    DECLAREVARCONV(String2A);

    if (String1->Length <= String2->Length)
    {
        int len = String1->Length / sizeof(WCHAR);
        for (i = 0; res && (i < len); i++)
            res = wcmp(String1->Buffer[i], String2->Buffer[i], CaseInSensitive);
    }
    else
        res = FALSE;

    US2STR(String1);
    US2STR(String2);

    Log("ntdll.RtlPrefixUnicodeString(\"%s\", \"%s\", %d)=%d\n", String1A, String2A, CaseInSensitive, res);

    return res;
}

BOOLEAN NTAPI RtlEqualUnicodeString(PCUNICODE_STRING String1, PCUNICODE_STRING String2, BOOLEAN CaseInSensitive)
{
    BOOLEAN res;
    DECLAREVARCONV(String1A);
    DECLAREVARCONV(String2A);

    US2STR(String1);
    US2STR(String2);

    res = CaseInSensitive ? !strcasecmp(String1A, String2A) : !strcmp(String1A, String2A);

    Log("ntdll.RtlEqualUnicodeString(\"%s\", \"%s\", %d)=%d\n", String1A, String2A, CaseInSensitive, res);

    return res;
}

static void *get_arg(int no, va_list *args, BOOLEAN ArgumentIsArray)
{
    void *sel;

    if (ArgumentIsArray)
        return args[no];

    do
    {
        sel = va_arg(*args, void *);
        no--;
    } while (no > 0);
    return sel;
}

NTSTATUS NTAPI RtlFormatMessage(LPWSTR Message, UCHAR MaxWidth, BOOLEAN IgnoreInserts, BOOLEAN Ansi,
    BOOLEAN ArgumentIsArray, va_list *Arguments, LPWSTR Buffer, ULONG BufferSize, PULONG ReturnLength)
{
    // FIXME: no check for BufferSize
    WCHAR c, *msg = Message;
    size_t eos = 0, count = 0;
    DECLAREVARCONV(BufferA);

    CHECK_POINTER(Message);

    if (Ansi)
    {
        fprintf(stderr, "RtlFormatMessage() - TODO Ansi\n");
        abort();
    }

    if (IgnoreInserts)
    {
        fprintf(stderr, "RtlFormatMessage() - TODO IgnoreInserts\n");
        abort();
    }

    while ((c = *msg++) && !eos)
    {
        if (c != L'%')
        {
            Buffer[count++] = c;
            continue;
        }

        c = *msg++;

        switch (c)
        {
            case '0':
                eos = 1;
                break;
            case 'r':
                Buffer[count++] = L'\r';
#ifndef _WIN32
                // clear the line on unix
                rpl_wcscat(&Buffer[count], L"\x1b[K");
                count += 3;
#endif
                break;
            case 'n':
                Buffer[count++] = L'\n';
                break;
            case 't':
                Buffer[count++] = L'\t';
                break;
            case 'b':
                Buffer[count++] = L' ';
                break;
            case '%':
                Buffer[count++] = L'%';
                break;
            case '1': case '2': case '3': case '4': case '5':
            case '6': case '7': case '8': case '9':
            {
                int argnum = wtoi(c);
                WCHAR *arg;
                if (isdigit(msg[1]))
                {
                    msg++;
                    argnum = (argnum * 10) + wtoi(*msg);
                }
                argnum--;
                arg = get_arg(argnum, Arguments, ArgumentIsArray);
                rpl_wcscat(&Buffer[count], arg);
                count += rpl_wcslen(arg);
                break;
            }
        }
    }
    Buffer[count] = 0;

    if (ReturnLength)
        *ReturnLength = count;

    WSTR2STR(Buffer);
    munge(BufferA);

    Log("ntdll.RtlFormatMessage(Message=%p, Buffer=%p) \"%s\"\n", Message, Buffer, BufferA);

    return STATUS_SUCCESS;
}

BOOLEAN NTAPI RtlDosPathNameToNtPathName_U(PCWSTR dos_path, PUNICODE_STRING ntpath, PWSTR* file_part, CURDIR* cd)
{
    size_t dplen_inb = rpl_wcslen(dos_path) * sizeof(WCHAR);
    DECLAREVARCONV(dos_pathA);
    DECLAREVARCONV(ntpathA);

    RtlInitUnicodeString(ntpath, NULL);
    ntpath->Buffer = RtlAllocateHeap(HANDLE_HEAP, 0, dplen_inb + sizeof(WCHAR));
    memcpy(ntpath->Buffer, dos_path, dplen_inb + sizeof(WCHAR));
    ntpath->Buffer[1] = L'?';
    ntpath->Length = dplen_inb;
    ntpath->MaximumLength = dplen_inb + sizeof(WCHAR);

    WSTR2STR(dos_path);
    US2STR(ntpath);

    Log("ntdll.RtlDosPathNameToNtPathName_U(\"%s\") = \"%s\"\n", dos_pathA, ntpathA);

    return TRUE;
}

NTSTATUS NTAPI RtlExpandEnvironmentStrings_U(PVOID Environment, PUNICODE_STRING SourceString,
    PUNICODE_STRING DestinationString, PULONG DestinationBufferLength)
{
    DECLAREVARCONV(SourceStringA);
    CHECK_POINTER(SourceString);
    CHECK_POINTER(DestinationString);

    US2STR(SourceString);

    Log("ntdll.RtlExpandEnvironmentStrings_U(\"%s\")\n", SourceStringA);

    if (!strcasecmp(SourceStringA, "%SystemRoot%"))
    {
        memcpy(DestinationString->Buffer, systemroot, sizeof(systemroot));
        DestinationString->Length = sizeof(systemroot) - sizeof(WCHAR);
    }
    else
    {
        memcpy(DestinationString->Buffer, SourceString->Buffer, SourceString->Length);
        DestinationString->Length = SourceString->Length;
    }

    if (DestinationBufferLength)
        *DestinationBufferLength = DestinationString->Length;
    return STATUS_SUCCESS;
}

VOID NTAPI RtlFillMemoryUlong(PVOID Destination, ULONG Length, ULONG Fill)
{
   PULONG Dest  = Destination;
   ULONG  Count = Length / sizeof(ULONG);

   while (Count > 0)
   {
      *Dest = Fill;
      Dest++;
      Count--;
   }
}

VOID NTAPI RtlSecondsSince1970ToTime(DWORD Seconds, LARGE_INTEGER *Time)
{
    ULONGLONG secs = Seconds * (ULONGLONG) TICKSPERSEC + TICKS_1601_TO_1970;
    Time->u.LowPart  = (DWORD) secs;
    Time->u.HighPart = (DWORD) (secs >> 32);
}

NTSTATUS NTAPI RtlAdjustPrivilege(IN ULONG Privilege, IN BOOLEAN Enable, IN BOOLEAN CurrentThread, OUT PBOOLEAN Enabled)
{
    Log("ntdll.RtlAdjustPrivilege(0x%08x, %d)\n", Privilege, Enable);
    *Enabled = Enable;
    return STATUS_SUCCESS;
}
