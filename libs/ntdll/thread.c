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

MODULE(thread)

/* TLS in a shared object loaded at runtime: "initial-exec" generates code that
 * works for both static-linked and dlopen'd cases on glibc. */
__thread __attribute__((tls_model("initial-exec")))
HANDLE CurrentThreadHandle = NULL;

static void *thread_start(void *arg)
{
    HANDLE th = arg;
    ThreadProc proc;
    CurrentThreadHandle = th;
    proc = th->thread.StartAddress;
    th->thread.ExitStatus = proc(th->thread.StartParameter);
    return (void *) (uintptr_t) th->thread.ExitStatus;
}

static HANDLE GetThreadHandle(HANDLE ThreadHandle)
{
    if (ThreadHandle == NtCurrentThread())
        return CurrentThreadHandle;
    else
        return ThreadHandle;
}

NTSTATUS NTAPI NtTerminateThread(HANDLE ThreadHandle, NTSTATUS ExitStatus)
{
    ThreadHandle = GetThreadHandle(ThreadHandle);

    Log("ntdll.NtTerminateThread(\"%s\", 0x%08x)\n", strhandle(ThreadHandle), ExitStatus);

    if (ThreadHandle)
        ThreadHandle->thread.ExitStatus = ExitStatus;

    pthread_exit(ThreadHandle ? &ThreadHandle->thread.ExitStatus : NULL);
    return STATUS_SUCCESS;  /* unreachable */
}
FORWARD_FUNCTION(NtTerminateThread, ZwTerminateThread);

NTSTATUS NTAPI RtlCreateUserThread(HANDLE ProcessHandle, /*PSECURITY_DESCRIPTOR*/ PVOID SecurityDescriptor,
    BOOLEAN CreateSuspended, ULONG StackZeroBits, PULONG StackReserved, PULONG StackCommit,
    PVOID StartAddress, PVOID StartParameter, PHANDLE ThreadHandle, PCLIENT_ID ClientID)
{
    char desc[1024];
    HANDLE th;

    CHECK_POINTER(ProcessHandle);
    CHECK_POINTER(StartAddress);
    CHECK_POINTER(ThreadHandle);

    snprintf(desc, sizeof(desc), "ThreadProc @%p - param @%p", StartAddress, StartParameter);

    __CreateHandle(th, HANDLE_TH, desc);
    th->thread.StartAddress = StartAddress;
    th->thread.StartParameter = StartParameter;
    th->thread.ExitStatus = -1;

    if (pthread_create(&th->thread.tid, NULL, thread_start, (void *) th))
    {
        RtlFreeHeap(GetProcessHeap(), 0, th);
        return STATUS_UNSUCCESSFUL;
    }
    *ThreadHandle = th;

    return STATUS_SUCCESS;
}

NTSTATUS NTAPI NtQueryInformationThread(HANDLE ThreadHandle, THREADINFOCLASS ThreadInformationClass,
    PVOID ThreadInformation, ULONG ThreadInformationLength, PULONG ReturnLength)
{
    NTSTATUS result = STATUS_BUFFER_TOO_SMALL;

    ThreadHandle = GetThreadHandle(ThreadHandle);

    Log("ntdll.NtQueryInformationThread(%s:0x%08x)", strthinfo(ThreadInformationClass), ThreadInformationClass);

    switch (ThreadInformationClass)
    {
        case ThreadBasicInformation:
        {
            THREAD_BASIC_INFORMATION *thinfo = ThreadInformation;

            if (ReturnLength)
                *ReturnLength = sizeof(THREAD_BASIC_INFORMATION);

            thinfo->ExitStatus = ThreadHandle ? ThreadHandle->thread.ExitStatus : 0;
            thinfo->TebBaseAddress = NtCurrentTeb();
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

NTSTATUS NTAPI NtSetThreadExecutionState(EXECUTION_STATE esFlags, EXECUTION_STATE *PreviousFlags)
{
    CHECK_POINTER(PreviousFlags);

    Log("ntdll.NtSetThreadExecutionState(0x%08x)\n", esFlags);

    *PreviousFlags = esFlags;
    return STATUS_SUCCESS;
}

/* critical sections — Windows critical sections are recursive on the same
 * thread, so we use PTHREAD_MUTEX_RECURSIVE. */

NTSTATUS NTAPI RtlInitializeCriticalSection(PRTL_CRITICAL_SECTION CriticalSection)
{
    pthread_mutexattr_t attr;
    pthread_mutex_t *m;

    CHECK_POINTER(CriticalSection);
    Log("ntdll.RtlInitializeCriticalSection(%p)\n", CriticalSection);
    memset(CriticalSection, 0, sizeof(RTL_CRITICAL_SECTION));

    m = RtlAllocateHeap(GetProcessHeap(), 0, sizeof(pthread_mutex_t));
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(m, &attr);
    pthread_mutexattr_destroy(&attr);
    CriticalSection->LockSemaphore = (HANDLE) m;
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI RtlDeleteCriticalSection(PRTL_CRITICAL_SECTION CriticalSection)
{
    CHECK_POINTER(CriticalSection);
    Log("ntdll.RtlDeleteCriticalSection(%p)\n", CriticalSection);
    pthread_mutex_destroy((pthread_mutex_t *) CriticalSection->LockSemaphore);
    RtlFreeHeap(GetProcessHeap(), 0, CriticalSection->LockSemaphore);
    CriticalSection->LockSemaphore = NULL;
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI RtlEnterCriticalSection(PRTL_CRITICAL_SECTION CriticalSection)
{
    Log("ntdll.RtlEnterCriticalSection(%p)\n", CriticalSection);
    pthread_mutex_lock((pthread_mutex_t *) CriticalSection->LockSemaphore);
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI RtlLeaveCriticalSection(PRTL_CRITICAL_SECTION CriticalSection)
{
    Log("ntdll.RtlLeaveCriticalSection(%p)\n", CriticalSection);
    pthread_mutex_unlock((pthread_mutex_t *) CriticalSection->LockSemaphore);
    return STATUS_SUCCESS;
}
