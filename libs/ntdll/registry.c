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

MODULE(registry)

#define REG_PREFIX "\\Registry\\Machine\\"

typedef struct _RegKey
{
    const char *path;
    void *data;
    DWORD len;
    DWORD type;
} RegKey;

static RegKey RegKeys[] =
{
    { "Software\\Microsoft\\Windows NT\\CurrentVersion\\SystemRoot", systemroot, sizeof(systemroot), REG_SZ },

    { "Hardware\\Description\\System\\Identifier", L"AT/AT COMPATIBLE", sizeof(L"AT/AT COMPATIBLE"), REG_SZ },

    { "System\\Setup\\SystemPartition", L"\\Device\\HarddiskVolume1", sizeof(L"\\Device\\HarddiskVolume1"), REG_SZ },

    { "System\\ControlSet000\\Control\\SystemStartOptions", L"FASTDETECT", sizeof(L"FASTDETECT"), REG_SZ },
    { "System\\CurrentControlSet\\Control\\SystemStartOptions", L"FASTDETECT", sizeof(L"FASTDETECT"), REG_SZ },

    { "Session Manager\\BootExecute", L"autocheck autochk *\0\0", sizeof(L"autocheck autochk *\0\0"), REG_MULTI_SZ },
    { "System\\ControlSet000\\Control\\Session Manager\\BootExecute", L"autocheck autochk *\0\0",
        sizeof(L"autocheck autochk *\0\0"), REG_MULTI_SZ },
    { "System\\CurrentControlSet\\Control\\Session Manager\\BootExecute", L"autocheck autochk *\0\0",
        sizeof(L"autocheck autochk *\0\0"), REG_MULTI_SZ },

    { "System\\ControlSet000\\Control\\Session Manager\\Memory Management\\PagingFiles", L"c:\\pagefile.sys\0\0",
        sizeof(L"c:\\pagefile.sys\0\0"), REG_MULTI_SZ },

    { NULL, NULL, 0 }
};

// FIXME: handle REG_DWORD
static RegKey *RegistryLookup(const char *path)
{
    int i = 0;

    if (!strncasecmp(path, REG_PREFIX, sizeof(REG_PREFIX) - 1))
        path += sizeof(REG_PREFIX) - 1;

    while (RegKeys[i].path)
    {
        if (!strcasecmp(path, RegKeys[i].path))
            return &RegKeys[i];
        i++;
    }
    return NULL;
}

NTSTATUS NTAPI NtOpenKey(PHANDLE retkey, ACCESS_MASK access, POBJECT_ATTRIBUTES ObjectAttributes)
{
    DECLAREVARCONV(ObjectAttributesA);
    CHECK_POINTER(retkey);
    CHECK_POINTER(ObjectAttributes);
    CHECK_POINTER(ObjectAttributes->ObjectName);

    OA2STR(ObjectAttributes);

    Log("ntdll.NtOpenKey(\"%s\")\n", ObjectAttributesA);

    __CreateHandle(*retkey, HANDLE_REG, ObjectAttributesA);

    return STATUS_SUCCESS;
}
FORWARD_FUNCTION(NtOpenKey, ZwOpenKey);

NTSTATUS NTAPI NtQueryValueKey(HANDLE KeyHandle, PUNICODE_STRING ValueName,
    KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass, PVOID KeyValueInformation, ULONG Length, PULONG ResultLength)
{
    DECLAREVARCONV(ValueNameA);
    RegKey *value;
    char path[512];
    size_t vlen;

    CHECK_HANDLE(KeyHandle, HANDLE_REG);
    CHECK_POINTER(ValueName);

    US2STR(ValueName);
    Log("ntdll.NtQueryValueKey(\"%s\", \"%s\", %d) - ", strhandle(KeyHandle), ValueNameA, KeyValueInformationClass);

    snprintf(path, sizeof(path), "%s\\%s", KeyHandle->name, ValueNameA);

    value = RegistryLookup(path);

    if (!value)
    {
        Log("Not Found\n");
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    Log("OK\n");

    vlen = value->len;
    switch (KeyValueInformationClass)
    {
        case KeyValueFullInformation:
        {
            KEY_VALUE_FULL_INFORMATION *k = KeyValueInformation;

            vlen += sizeof(KEY_VALUE_FULL_INFORMATION);

            if (ResultLength)
                *ResultLength = vlen;

            if (Length < vlen)
                return STATUS_BUFFER_TOO_SMALL;

            k->TitleIndex = 0;
            k->Type = value->type;
            k->DataOffset = FIELD_OFFSET(KEY_VALUE_FULL_INFORMATION, Name);
            k->DataLength = k->NameLength = value->len;
            memcpy(k->Name, value->data, value->len);

            return STATUS_SUCCESS;
        }
        case KeyValuePartialInformation:
        {
            KEY_VALUE_PARTIAL_INFORMATION *k = KeyValueInformation;

            vlen += sizeof(KEY_VALUE_PARTIAL_INFORMATION);

            if (ResultLength)
                *ResultLength = vlen;

            if (Length < vlen)
                return STATUS_BUFFER_TOO_SMALL;

            k->TitleIndex = 0;
            k->Type = value->type;
            k->DataLength = value->len;
            memcpy(k->Data, value->data, value->len);

            return STATUS_SUCCESS;
        }
        default:
            return STATUS_NOT_IMPLEMENTED;
    }
}
FORWARD_FUNCTION(NtQueryValueKey, ZwQueryValueKey);

NTSTATUS NTAPI NtSetValueKey(HANDLE KeyHandle, PUNICODE_STRING ValueName, ULONG TitleIndex, ULONG Type, PVOID Data, ULONG DataSize)
{
    DECLAREVARCONV(ValueNameA);
    DECLAREVARCONV(DataA);

    CHECK_HANDLE(KeyHandle, HANDLE_REG);
    CHECK_POINTER(ValueName);

    US2STR(ValueName);
    RtlUnicodeToOemN(DataA, _CONVBUF, NULL, (LPCWSTR) Data, DataSize);

    if (Type == REG_MULTI_SZ)
    {
        unsigned int i;
        for (i = 0; i < DataSize / sizeof(WCHAR); i++)
            if (!ValueNameA[i]) ValueNameA[i] = '|';
    }

    Log("ntdll.NtSetValueKey(\"%s\", \"%s\", \"%s\", %d)\n", strhandle(KeyHandle), ValueNameA, DataA, Type);

    return STATUS_SUCCESS;
}
FORWARD_FUNCTION(NtSetValueKey, ZwSetValueKey);

NTSTATUS NTAPI RtlQueryRegistryValues(ULONG RelativeTo, PCWSTR Path, PRTL_QUERY_REGISTRY_TABLE QueryTable, PVOID Context, PVOID Environment)
{
    NTSTATUS result = STATUS_OBJECT_NAME_NOT_FOUND;
    DECLAREVARCONV(PathA);
    DECLAREVARCONV(key);
    char path[512];
    RegKey *value;

    CHECK_POINTER(Path);
    CHECK_POINTER(QueryTable);

    WSTR2STR(Path);
    Log("ntdll.RtlQueryRegistryValues(\"%s\"): keys -> ", PathA);

    for (; QueryTable->QueryRoutine != NULL || QueryTable->Name != NULL; ++QueryTable)
    {
        WSTR2STREX(QueryTable->Name, key);
        snprintf(path, sizeof(path), "%s\\%s", PathA, key);
        value = RegistryLookup(path);

        Log("%s (", path);

        if (!value)
        {
            Log("Not found)");
            continue;
        }

        if (QueryTable->Flags & RTL_QUERY_REGISTRY_DIRECT)
        {
            PUNICODE_STRING str = QueryTable->EntryContext;
            if (str->MaximumLength < value->len)
                result = STATUS_BUFFER_TOO_SMALL;
            else
            {
                memcpy(str->Buffer, value->data, value->len);
                str->Length = value->len;
                result = STATUS_SUCCESS;
            }
            Log("OK)");
            break;
        }
        Log(")");
    }

    Log("\n");

    return result;
}

NTSTATUS NTAPI RtlWriteRegistryValue(IN ULONG RelativeTo, IN PCWSTR Path, IN PCWSTR ValueName,
    IN ULONG ValueType, IN PVOID ValueData, IN ULONG ValueLength)
{
    DECLAREVARCONV(PathA);
    DECLAREVARCONV(ValueNameA);
    WSTR2STR(Path);
    WSTR2STR(ValueName);
    Log("ntdll.RtlWriteRegistryValue(\"%s\\%s\")\n", PathA, ValueNameA);
    return STATUS_SUCCESS;
}
