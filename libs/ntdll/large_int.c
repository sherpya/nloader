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

MODULE(large_int)

LONGLONG NTAPI _alldiv(LONGLONG a, LONGLONG b)
{
    Log("ntdll._alldiv(%lld, %lld) = %lld [ %lld / %lld ]\n", a, b, a / b, a, b);
    return a / b;
}

ULONGLONG NTAPI _aulldiv(ULONGLONG a, ULONGLONG b)
{
    Log("ntdll._aulldiv(%llu, %llu) = %llu [ %llu / %llu ]\n", a, b, a / b, a, b);
    return a / b;
}

LONGLONG NTAPI _allrem(LONGLONG a, LONGLONG b)
{
    Log("ntdll._allrem(%lld, %lld) = %lld [ %lld %% %lld ]\n", a, b, a % b, a, b);
    return a % b;
}

ULONGLONG NTAPI _aullrem(ULONGLONG a, ULONGLONG b)
{
    Log("ntdll._aullrem(%llu, %llu) = %llu [ %llu %% %llu ]\n", a, b, a % b, a, b);
    return a % b;
}

LONGLONG NTAPI _allmul(LONGLONG a, LONGLONG b)
{
    Log("ntdll._allmul(%lld, %lld) = %lld [ %lld * %lld ]\n", a, b, a * b, a, b);
    return a * b;
}

ULONGLONG NTAPI _aullshr(ULONGLONG a, LONG b)
{
    Log("ntll._aullshr(%llu, %d) = %llu [ %llu >> %d ]\n", a, b, a >> b, a, b);
    return a >> b;
}

LONGLONG NTAPI _allshl(LONGLONG a, LONG b)
{
    Log("ntll._allshr(%lld, %d) = %lld [ %llu >> %d ]\n", a, b, a << b, a, b);
    return a << b;
}

LONGLONG NTAPI RtlExtendedIntegerMultiply(LONGLONG a, INT b)
{
    Log("ntll.RtlExtendedIntegerMultiply(%lld, %d) = %lld [ %lld * %d ]\n", a, b, a * b, a, b);
    return a * b;
}

ULONGLONG NTAPI RtlLargeIntegerDivide(ULONGLONG a, ULONGLONG b, ULONGLONG *rem)
{
    ULONGLONG ret = a / b;
    if (rem)
        *rem = a - ret * b;
    Log("ntll.RtlLargeIntegerDivide(%llu, %llu) = %llu, %%llu [ %llu / %llu ]\n", a, b, ret, a - ret * b,  a, b);
    return ret;
}
