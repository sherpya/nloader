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
#include <pthread.h>
#include <sys/time.h>

MODULE(sync)

/* Convert an NT LARGE_INTEGER (100ns ticks) into an absolute timespec
 * suitable for pthread_cond_timedwait (CLOCK_REALTIME based).
 *
 * NT semantics:
 *   - negative value: relative interval, in 100ns units
 *   - positive value: absolute time since 1601 (NT epoch)
 *   - NULL or zero pointer: wait forever (caller handles)
 */
static inline int interval_to_timespec(PLARGE_INTEGER DelayInterval, struct timespec *timeout)
{
    struct timeval now;
    LONGLONG ns_offset;

    if (!DelayInterval || !DelayInterval->QuadPart)
        return 0;

    gettimeofday(&now, NULL);

    if (DelayInterval->QuadPart < 0)
    {
        /* relative, 100ns units → ns */
        ns_offset = -DelayInterval->QuadPart * 100;
    }
    else
    {
        /* absolute NT time. Convert to ns since unix epoch then subtract now. */
        LONGLONG unix_100ns = DelayInterval->QuadPart - TICKS_1601_TO_1970;
        LONGLONG now_100ns  = (LONGLONG) now.tv_sec * 10000000LL + (LONGLONG) now.tv_usec * 10LL;
        ns_offset = (unix_100ns - now_100ns) * 100;
        if (ns_offset < 0)
            ns_offset = 0;
    }

    timeout->tv_sec  = now.tv_sec + ns_offset / 1000000000LL;
    timeout->tv_nsec = now.tv_usec * 1000 + ns_offset % 1000000000LL;
    if (timeout->tv_nsec >= 1000000000L)
    {
        timeout->tv_sec  += 1;
        timeout->tv_nsec -= 1000000000L;
    }

    return 1;
}

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
            struct timespec deadline;
            int has_timeout = interval_to_timespec(Timeout, &deadline);

            /* Single mutex protects both the predicate (signaled) and the
             * condvar wait — avoids the lost-wakeup race the old code had. */
            pthread_mutex_lock(&Handle->event.state_mutex);

            while (!Handle->event.signaled)
            {
                int ret;
                if (has_timeout)
                    ret = pthread_cond_timedwait(&Handle->event.cond, &Handle->event.state_mutex, &deadline);
                else
                    ret = pthread_cond_wait(&Handle->event.cond, &Handle->event.state_mutex);

                if (ret == ETIMEDOUT)
                {
                    result = STATUS_TIMEOUT;
                    break;
                }
                if (ret != 0)
                {
                    result = STATUS_UNSUCCESSFUL;
                    break;
                }
                /* spurious wakeup: re-check predicate */
            }

            if (result == STATUS_SUCCESS && Handle->event.type == SynchronizationEvent)
                Handle->event.signaled = 0;

            pthread_mutex_unlock(&Handle->event.state_mutex);
            return result;
        }
        case HANDLE_TH:
        {
            pthread_join(Handle->thread.tid, NULL);
            return STATUS_SUCCESS;
        }
    }

    return STATUS_NOT_IMPLEMENTED;
}
FORWARD_FUNCTION(NtWaitForSingleObject, ZwWaitForSingleObject);

NTSTATUS NTAPI NtWaitForMultipleObjects(ULONG ObjectCount, PHANDLE ObjectsArray,
    OBJECT_WAIT_TYPE WaitType, BOOLEAN Alertable, PLARGE_INTEGER Timeout)
{
    Log("ntdll.NtWaitForMultipleObjects(%u, %s)\n", ObjectCount,
        WaitType == WaitAllObject ? "WaitAllObject" : "WaitAnyObject");

    if (!ObjectCount || !ObjectsArray)
        return STATUS_INVALID_PARAMETER;

    if (WaitType == WaitAllObject)
    {
        /* Sequential wait. Acceptable approximation: real Windows would
         * atomically wait for all. For autochk's usage (events + thread
         * handles) the order doesn't matter. */
        for (ULONG i = 0; i < ObjectCount; i++)
        {
            NTSTATUS s = NtWaitForSingleObject(ObjectsArray[i], Alertable, Timeout);
            if (s != STATUS_SUCCESS)
                return s;
        }
        return STATUS_SUCCESS;
    }

    /* WaitAny: poll each object with a zero timeout, then sleep briefly.
     * Sufficient for the keyboard-poll pattern; not efficient but correct. */
    {
        LARGE_INTEGER zero = { .QuadPart = -1LL };  /* 100ns, effectively poll */
        struct timespec deadline = { 0 };
        int has_deadline = interval_to_timespec(Timeout, &deadline);

        for (;;)
        {
            for (ULONG i = 0; i < ObjectCount; i++)
            {
                HANDLE h = ObjectsArray[i];
                if (!h) continue;
                if (h->type == HANDLE_EV)
                {
                    pthread_mutex_lock(&h->event.state_mutex);
                    if (h->event.signaled)
                    {
                        if (h->event.type == SynchronizationEvent)
                            h->event.signaled = 0;
                        pthread_mutex_unlock(&h->event.state_mutex);
                        return STATUS_WAIT_0 + i;
                    }
                    pthread_mutex_unlock(&h->event.state_mutex);
                }
                else
                {
                    /* Non-event handle: defer to single-object wait with poll. */
                    NTSTATUS s = NtWaitForSingleObject(h, Alertable, &zero);
                    if (s == STATUS_SUCCESS)
                        return STATUS_WAIT_0 + i;
                }
            }

            if (has_deadline)
            {
                struct timespec now;
                clock_gettime(CLOCK_REALTIME, &now);
                if (now.tv_sec > deadline.tv_sec ||
                    (now.tv_sec == deadline.tv_sec && now.tv_nsec >= deadline.tv_nsec))
                    return STATUS_TIMEOUT;
            }
            else if (Timeout && !Timeout->QuadPart)
                return STATUS_TIMEOUT;

            /* sleep 10ms then retry */
            struct timespec slp = { 0, 10 * 1000 * 1000 };
            nanosleep(&slp, NULL);
        }
    }
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

    pthread_mutex_init(&(*EventHandle)->event.state_mutex, NULL);
    pthread_cond_init(&(*EventHandle)->event.cond, NULL);

    Log("ntdll.NtCreateEvent(\"%s\", type=%d, init=%d)\n", EventA, EventType, InitialState);

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

    pthread_mutex_lock(&EventHandle->event.state_mutex);
    if (PreviousState)
        *PreviousState = EventHandle->event.signaled;
    EventHandle->event.signaled = 1;
    if (EventHandle->event.type == NotificationEvent)
        pthread_cond_broadcast(&EventHandle->event.cond);
    else
        pthread_cond_signal(&EventHandle->event.cond);
    pthread_mutex_unlock(&EventHandle->event.state_mutex);

    Log("ntdll.NtSetEvent(\"%s\")\n", strhandle(EventHandle));

    return STATUS_SUCCESS;
}

NTSTATUS NTAPI NtResetEvent(HANDLE EventHandle, PLONG PreviousState)
{
    CHECK_HANDLE(EventHandle, HANDLE_EV);

    Log("ntdll.NtReSetEvent(\"%s\")\n", strhandle(EventHandle));

    pthread_mutex_lock(&EventHandle->event.state_mutex);
    if (PreviousState)
        *PreviousState = EventHandle->event.signaled;
    EventHandle->event.signaled = 0;
    pthread_mutex_unlock(&EventHandle->event.state_mutex);

    return STATUS_SUCCESS;
}

NTSTATUS NTAPI NtDelayExecution(BOOLEAN Alertable, PLARGE_INTEGER DelayInterval)
{
    Log("ntdll.NtDelayExecution(%d, %lld)\n", Alertable, DelayInterval ? DelayInterval->QuadPart : 0);

    if (!DelayInterval || !DelayInterval->QuadPart)
        return STATUS_SUCCESS;

    if (DelayInterval->QuadPart < 0)
    {
        /* relative interval, 100ns units */
        LONGLONG ns = -DelayInterval->QuadPart * 100;
        struct timespec slp = { ns / 1000000000LL, ns % 1000000000LL };
        nanosleep(&slp, NULL);
    }
    /* absolute time: not implemented (no consumer in our guests) */
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI NtClearEvent(HANDLE EventHandle)
{
    CHECK_HANDLE(EventHandle, HANDLE_EV);

    Log("ntdll.NtClearEvent(\"%s\")\n", strhandle(EventHandle));
    return NtResetEvent(EventHandle, NULL);
}
FORWARD_FUNCTION(NtClearEvent, ZwClearEvent);
