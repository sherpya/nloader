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
#include <time.h>

#ifndef _WIN32
#include <sys/time.h>
#endif

MODULE(time)

NTSTATUS NTAPI NtQuerySystemTime(PLARGE_INTEGER Time)
{
    struct timeval now;
    CHECK_POINTER(Time);

    gettimeofday(&now, NULL);
    Time->QuadPart = now.tv_sec * TICKSPERSEC + TICKS_1601_TO_1970;
    Time->QuadPart += now.tv_usec * 10;
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI RtlSystemTimeToLocalTime(IN PLARGE_INTEGER SystemTime, OUT PLARGE_INTEGER LocalTime)
{
    int64_t bias;
    time_t now = time(NULL);

#ifdef _WIN32
    struct tm *tnow = gmtime(&now);
    time_t lnow = mktime(tnow);
    bias = now - lnow;
#else
    struct tm *t = localtime(&now);
    bias = t ? t->tm_gmtoff : 0;
#endif

    bias *= TICKSPERSEC;
    bias = -bias;

    LocalTime->QuadPart = SystemTime->QuadPart - bias;
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI NtQueryPerformanceCounter(PLARGE_INTEGER PerformanceCounter, PLARGE_INTEGER PerformanceFrequency)
{
    LARGE_INTEGER now;
    uint64_t server_start_time = 0;

    CHECK_POINTER(PerformanceCounter);

    if (!PerformanceCounter)
        return STATUS_ACCESS_VIOLATION;

    server_start_time = uptime() * TICKSPERSEC + TICKS_1601_TO_1970;

    /* convert a counter that increments at a rate of 10 MHz
     * to one of 1.193182 MHz, with some care for arithmetic
     * overflow and good accuracy (21/176 = 0.11931818) */

    NtQuerySystemTime(&now);
    PerformanceCounter->QuadPart = ((now.QuadPart - server_start_time) * 21) / 176;

    if (PerformanceFrequency)
        PerformanceFrequency->QuadPart = 1193182;

    return STATUS_SUCCESS;
}
