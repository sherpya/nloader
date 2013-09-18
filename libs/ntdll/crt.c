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
#include <ctype.h>

MODULE(crt)

FORWARD_FUNCTION(memcmp, rpl_memcmp);
FORWARD_FUNCTION(memcpy, rpl_memcpy);
FORWARD_FUNCTION(memset, rpl_memset);
FORWARD_FUNCTION(memmove, rpl_memmove);
FORWARD_FUNCTION(atoll, rpl__atoi64);
FORWARD_FUNCTION(atoi, rpl_atoi);
FORWARD_FUNCTION(qsort, rpl_qsort);
FORWARD_FUNCTION(strcasecmp, rpl__stricmp);
FORWARD_FUNCTION(strncasecmp, rpl__strnicmp);
FORWARD_FUNCTION(strncpy, rpl_strncpy);
FORWARD_FUNCTION(strpbrk, rpl_strpbrk);
FORWARD_FUNCTION(strspn, rpl_strspn);

// long is always 32bit on win32
LONG CDECL rpl_atol(const char *nptr)
{
    return atol(nptr);
}

SIZE_T CDECL rpl_wcslen(LPCWSTR str)
{
    const WCHAR *s = str;

    if (!str)
        return 0;

#ifdef DEBUG_CRT
    spam_wchar(str);
#endif
    while (*s)
        s++;
    return (s - str);
}

LPWSTR CDECL rpl_wcscat(LPWSTR dest, LPCWSTR src)
{
    DECLAREVARCONV(srcA);
    DECLAREVARCONV(destA);
    WCHAR *d;

    if (!src)
        return dest;

    WSTR2STR(dest);
    WSTR2STR(src);

    Log("ntdll.wcscat(\"%s\", \"%s\")\n", destA, srcA);

    d = dest;

/*    while (*d++); d--; */ // FIXME: why not?
    while ((*d++ = *src++));

    return dest;
}

int CDECL rpl_wcscmp(LPCWSTR s1, LPCWSTR s2)
{
    DECLAREVARCONV(s1A);
    DECLAREVARCONV(s2A);
    WSTR2STR(s1);
    WSTR2STR(s2);

    Log("ntdll._wcscmp(\"%s\", \"%s\")\n", s1A, s2A);

    while (*s1 == *s2)
    {
        if (!*s1++) return 0;
        s2++;
    }
    return (*s1 < *s2) ? -1 : 1;
}

int CDECL rpl_wcsncmp(LPCWSTR s1, LPCWSTR s2, SIZE_T n)
{
    DECLAREVARCONV(s1A);
    DECLAREVARCONV(s2A);
    WSTR2STR(s1);
    WSTR2STR(s2);

    Log("ntdll.wcsncmp(\"%s\", \"%s\", %d)\n", s1A, s2A, n);

    do
    {
        if (!(n && *s1))
            return 0;
    }
    while (n-- && (*s1++ == *s2++));
    return (*s1 < *s2) ? -1 : 1;
}

int CDECL rpl__wcsicmp(LPCWSTR s1, LPCWSTR s2)
{
    DECLAREVARCONV(s1A);
    DECLAREVARCONV(s2A);
    WSTR2STR(s1);
    WSTR2STR(s2);

    Log("ntdll._wcsicmp(\"%s\", \"%s\")\n", s1A, s2A);

    while (wcmp(*s1, *s2, TRUE))
    {
        if (!*s1++)
            return 0;
        s2++;
    }
    return widetol(*s1) < widetol(*s2) ? -1 : 1;
}

int CDECL rpl__wcsnicmp(LPCWSTR s1, LPCWSTR s2, SIZE_T n)
{
    DECLAREVARCONV(s1A);
    DECLAREVARCONV(s2A);
    WSTR2STR(s1);
    WSTR2STR(s2);

    Log("ntdll._wcsnicmp(\"%s\", \"%s\", %d)\n", s1A, s2A, n);

    while (wcmp(*s1, *s2, TRUE))
    {
        n--;
        if (!(n && *s1++))
            return 0;
        s2++;
    }
    return widetol(*s1) < widetol(*s2) ? -1 : 1;
}

LPWSTR CDECL rpl_wcsstr(LPCWSTR haystack, LPCWSTR needle)
{
    char *res;
    DECLAREVARCONV(haystackA);
    DECLAREVARCONV(needleA);

    if (!rpl_wcslen(needle))
        return (LPWSTR) haystack;

    WSTR2STR(haystack);
    WSTR2STR(needle);

    if (!(res = strstr(haystackA, needleA)))
        return NULL;

    return (LPWSTR) (haystack + (needleA - haystackA));
}

LPWSTR CDECL rpl_wcscpy(LPWSTR dest, LPCWSTR src)
{
    WCHAR *s = dest;
    while (*src)
        *s++ = *src++;
    *s = 0;
    return dest;
}

LPWSTR CDECL rpl_wcsrchr(LPCWSTR s, WCHAR c)
{
    const WCHAR *p = NULL;
    DECLAREVARCONV(sA);
    WSTR2STR(s);

    Log("ntdll.wcsrchr(\"%s\", \"%c\")\n", sA, widetoa(c));

    do
    {
        if (*s == c)
            p = s;
    } while (*s++);

    return (WCHAR *) p;
}

LPWSTR CDECL rpl_wcschr(LPCWSTR str, WCHAR c)
{
    WCHAR *s = (WCHAR *) str;
    DECLAREVARCONV(strA);
    WSTR2STR(str);

    Log("ntdll.wcschr(\"%s\", \"%c\")\n", strA, widetoa(c));

    while (*s)
    {
        if (*s == c)
            return s;
        s++;
    }
    return NULL;
}

LPWSTR CDECL rpl__wcsupr(LPWSTR str)
{
    WCHAR *s = str;
    while (*s)
    {
        *s = widetou(*s);
        s++;
    }
    return str;
}

LPWSTR CDECL rpl__wcslwr(LPWSTR str)
{
    WCHAR *s = str;
    while (*s)
    {
        *s = widetol(*s);
        s++;
    }
    return str;
}

SIZE_T rpl_wcsspn(LPCWSTR wcs, LPCWSTR accept)
{
    SIZE_T res;
    DECLAREVARCONV(wcsA);
    DECLAREVARCONV(acceptA);
    WSTR2STR(wcs);
    WSTR2STR(accept);

    res = strspn(wcsA, acceptA);
    Log("ntdll._wcsspn(\"%s\", \"%s\") = %d\n", wcsA, acceptA, res);
    return res;
}

/* ctype */
int CDECL rpl_isspace(int c)
{
    if (((unsigned) c <= 255) && strchr(" \f\n\r\t", c))
        return 1;
    return 0;
}

int CDECL rpl_isprint(int c)
{
    if (((unsigned) c <= 255) && (c >= ' ') && (c <= '~'))
        return 1;
    return 0;
}

ULONG CDECL rpl_wcstoul(const wchar_t *nptr, wchar_t **endptr, int base)
{
    ULONG ret;
    DECLAREVARCONV(nptrA);
    WSTR2STR(nptr);
    char *endptrA;

    ret = strtoul(nptrA, &endptrA, base);

    if (endptr)
        *endptr = ((wchar_t *) nptr) + (endptrA - nptrA);

    return ret;
}

ULONGLONG CDECL rpl__wcstoui64(const wchar_t *nptr, wchar_t **endptr, int base)
{
    ULONGLONG ret;
    DECLAREVARCONV(nptrA);
    WSTR2STR(nptr);
    char *endptrA;

    ret = strtoll(nptrA, &endptrA, base);

    if (endptr)
        *endptr = ((wchar_t *) nptr) + (endptrA - nptrA);
    return ret;
}

LONGLONG CDECL rpl__wtoi64(LPCWSTR str)
{
    ULONGLONG value = 0;
    BOOLEAN neg = FALSE;

    while (rpl_isspace(widetoa(*str)))
        str++;

    if (*str == L'+')
        str++;
    else if (*str == L'-')
    {
        neg = TRUE;
        str++;
    }

    while (widetoa(*str) >= '0' && widetoa(*str) <= '9')
    {
        value = value * 10 + widetoa(*str) - '0';
        str++;
    }
    return neg ? 0 - value : value;
}

LONG CDECL rpl__wtol(LPCWSTR str)
{
    return (LONG) rpl__wtoi64(str);
}
