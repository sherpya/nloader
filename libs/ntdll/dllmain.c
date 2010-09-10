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

#ifdef _WIN32

#include "ntdll.h"
#include "ftable.h"

functable_t ftbl;

#define IMPORT_FUNC(m, x) \
    ftbl.m.x = ( imp_##x ) GetProcAddress(ftbl.m.hLib, Q(x));

#define IMPORT_FUNC_OR_FAIL(m, x) \
    IMPORT_FUNC(m, x); \
    if (!ftbl.m.x) ftbl.m.ok = FALSE;

static BOOL InitFunctionTable(void)
{
    ftbl.nt.hLib = LoadLibraryA("ntdll.dll");
    ftbl.nt.ok = TRUE;
    IMPORT_FUNC_OR_FAIL(nt, NtOpenFile);
    IMPORT_FUNC_OR_FAIL(nt, NtCreateFile);
    IMPORT_FUNC_OR_FAIL(nt, NtReadFile);
    IMPORT_FUNC_OR_FAIL(nt, NtWriteFile);
    IMPORT_FUNC_OR_FAIL(nt, NtClose);
    IMPORT_FUNC_OR_FAIL(nt, NtDeviceIoControlFile);
    IMPORT_FUNC_OR_FAIL(nt, NtQueryInformationFile);
    IMPORT_FUNC_OR_FAIL(nt, NtFsControlFile);
    return TRUE;
}

BOOL APIENTRY DllMain(HINSTANCE hModule, DWORD reason, LPVOID lpReserved)
{
    switch (reason)
    {
        case DLL_PROCESS_ATTACH:
            return InitFunctionTable();
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}

#endif
