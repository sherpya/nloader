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
#include <errno.h>

MODULE(sync)

#ifdef THREADED
#include <pthread.h>
#include <sys/time.h>

static inline int interval_to_timespec(PLARGE_INTEGER DelayInterval, struct timespec *timeout, int relative)
{
    LARGE_INTEGER when, delay;

    if (!DelayInterval)
        return 0;

    delay.QuadPart = DelayInterval->QuadPart;

    NtQuerySystemTime(&when);

    // Negative value means delay relative to current
    if (DelayInterval->QuadPart < 0)
    {
        if (relative)
            when.QuadPart = -DelayInterval->QuadPart;
        else
            when.QuadPart -= DelayInterval->QuadPart;
    }
    else
    {
        if (relative)
            when.QuadPart = DelayInterval->QuadPart - when.QuadPart;
        else
            when.QuadPart = DelayInterval->QuadPart;
    }

    timeout->tv_sec  = when.QuadPart / 1000000;
    timeout->tv_nsec = when.QuadPart % 1000000;

    //Log("sync: %lld -> Rel: %s sec: %lu nsec: %lu\n", DelayInterval->QuadPart, relative ? "yes" : "no", timeout->tv_sec, timeout->tv_nsec);

    return 1;
}
#endif

NTSTATUS NTAPI NtWaitForSingleObject(HANDLE Handle, BOOLEAN Alertable, PLARGE_INTEGER Timeout)
{
    CHECK_POINTER(Handle);

    Log("ntdll.NtWaitForSingleObject(\"%s\", %llu)\n", strhandle(Handle), Timeout ? Timeout->QuadPart : 0);

    switch (Handle->type)
    {
        case HANDLE_FILE:
            return STATUS_SUCCESS;
        case HANDLE_EV:
        {
            NTSTATUS result = STATUS_SUCCESS;

#ifdef THREADED
            struct timespec timeout;

            interval_to_timespec(Timeout, &timeout, 0);
            pthread_mutex_lock(&Handle->event.cond_mutex);

            while (1)
            {
                int ret;
                pthread_mutex_lock(&Handle->event.state_mutex);

                if(Handle->event.signaled)
                {
                    result = STATUS_SUCCESS;
                    break;
                }
                pthread_mutex_unlock(&Handle->event.state_mutex);

                if (Timeout && Timeout->QuadPart)
                {
                    struct timeval before, after;
                    gettimeofday(&before, NULL);
                    ret = pthread_cond_timedwait(&Handle->event.cond, &Handle->event.cond_mutex, &timeout);
                    gettimeofday(&after, NULL);
                    timeout.tv_sec  -= after.tv_sec - before.tv_sec;
                    timeout.tv_nsec -= (after.tv_usec - before.tv_usec) * 1000;
                }
                else
                    ret = pthread_cond_wait(&Handle->event.cond, &Handle->event.cond_mutex);

                switch (ret)
                {
                    case 0:
                        continue;
                    case ETIMEDOUT:
                        result = STATUS_TIMEOUT;
                        break;
                    default:/* EINVAL, EPERM, ENOTLOCKED, etc. */
                        result = STATUS_UNSUCCESSFUL;
                }
                break;
            }
            pthread_mutex_unlock(&Handle->event.cond_mutex);
#else   /* THREADED */
            if(Handle->event.signaled)  { /* I SHOULD BE DEALOCKING HERE */ }
#endif  /* THREADED */
            if(result == STATUS_SUCCESS)
            {
                if(Handle->event.type == SynchronizationEvent)
                    Handle->event.signaled = 0;
#ifdef THREADED
                pthread_mutex_unlock(&Handle->event.state_mutex);
#endif
            }
            return result;
        }
        case HANDLE_TH:
        {
#ifdef THREADED
            pthread_join(Handle->thread.tid, NULL);
#endif
            return STATUS_SUCCESS;
        }
    }

    return STATUS_NOT_IMPLEMENTED;
}
FORWARD_FUNCTION(NtWaitForSingleObject, ZwWaitForSingleObject);

NTSTATUS NTAPI NtWaitForMultipleObjects(ULONG ObjectCount, PHANDLE ObjectsArray,
    OBJECT_WAIT_TYPE WaitType, BOOLEAN Alertable, PLARGE_INTEGER Timeout)
{
    // FIXME: in threaded mode a call to:
    // NtWaitForSingleObject(ObjectsArray[0], Alertable, Timeout)
    // the programs creates an event and associates it to NtReadFile call
    // this event is triggered when something is ready
    // native programs use this method to e.g. read from keyboard

    if ((ObjectCount == 1) || (ObjectCount = 2)) // keyboards
        return STATUS_TIMEOUT;

    /* pagedefrag */
    if (ObjectCount == 16)
        return STATUS_TIMEOUT;

    fprintf(stderr, "ntdll.NtWaitForMultipleObjects() is only implemented for the keyboard stuff\n");
    abort();

    return STATUS_UNSUCCESSFUL; /* vc please... abort is noreturn */
}
FORWARD_FUNCTION(NtWaitForMultipleObjects, ZwWaitForMultipleObjects);

NTSTATUS NTAPI NtCreateEvent(PHANDLE EventHandle, ACCESS_MASK DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes, EVENT_TYPE EventType, BOOLEAN InitialState)
{
    char EventA[_CONVBUF] = "-Event-";
    CHECK_POINTER(EventHandle);

    if (ObjectAttributes && ObjectAttributes->ObjectName)
        RtlUnicodeToOemN(EventA, sizeof(EventA), NULL,
            ObjectAttributes->ObjectName->Buffer, ObjectAttributes->ObjectName->Length);

    __CreateHandle(*EventHandle, HANDLE_EV, EventA);

    (*EventHandle)->event.type = EventType;
    (*EventHandle)->event.signaled = InitialState;

#ifdef THREADED
    pthread_mutex_init(&(*EventHandle)->event.state_mutex, NULL);
    pthread_mutex_init(&(*EventHandle)->event.cond_mutex, NULL);
    pthread_cond_init(&(*EventHandle)->event.cond, NULL);
#endif

    Log("ntdll.NtCreateEvent(\"%s\")\n", EventA);

    return STATUS_SUCCESS;
}

NTSTATUS NTAPI NtOpenEvent(PHANDLE EventHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes)
{
    DECLAREVARCONV(ObjectAttributesA);

    CHECK_POINTER(EventHandle);
    CHECK_POINTER(ObjectAttributes);

    OA2STR(ObjectAttributes);

    Log("ntdll.NtOpenEvent(\"%s\")\n", ObjectAttributesA);
    return STATUS_NOT_IMPLEMENTED;
}
FORWARD_FUNCTION(NtOpenEvent, ZwOpenEvent);

NTSTATUS NTAPI NtSetEvent(HANDLE EventHandle, PLONG PreviousState)
{
    CHECK_HANDLE(EventHandle, HANDLE_EV);

#ifdef THREADED
    pthread_mutex_lock(&EventHandle->event.state_mutex);
#endif
    if (PreviousState)
        *PreviousState = EventHandle->event.signaled;
    EventHandle->event.signaled = 1;
#ifdef THREADED
    pthread_cond_signal(&EventHandle->event.cond);
    pthread_mutex_unlock(&EventHandle->event.state_mutex);
#endif

    Log("ntdll.NtSetEvent(\"%s\")\n", strhandle(EventHandle));

    return STATUS_SUCCESS;
}

NTSTATUS NTAPI NtResetEvent(HANDLE EventHandle, PLONG PreviousState)
{
    CHECK_HANDLE(EventHandle, HANDLE_EV);

    Log("ntdll.NtReSetEvent(\"%s\")\n", strhandle(EventHandle));

#ifdef THREADED
    pthread_mutex_lock(&EventHandle->event.state_mutex);
#endif
    if (PreviousState)
        *PreviousState = EventHandle->event.signaled;
    EventHandle->event.signaled = 0;
#ifdef THREADED
    pthread_mutex_unlock(&EventHandle->event.state_mutex);
#endif

    return STATUS_SUCCESS;
}

NTSTATUS NTAPI NtDelayExecution(BOOLEAN Alertable, PLARGE_INTEGER DelayInterval)
{
    Log("ntdll.NtDelayExecution(%d, %llu)\n", Alertable, DelayInterval ? DelayInterval->QuadPart : 0);
#if 0
    struct timespec timeout;
    if (!interval_to_timespec(DelayInterval, &timeout, 1))
        return STATUS_SUCCESS;

    nanosleep(&timeout, NULL);
#endif
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI NtClearEvent(HANDLE EventHandle)
{
    CHECK_HANDLE(EventHandle, HANDLE_EV);

    Log("ntdll.NtClearEvent(\"%s\")\n", strhandle(EventHandle));
    return NtResetEvent(EventHandle, NULL);
}
FORWARD_FUNCTION(NtClearEvent, ZwClearEvent);
