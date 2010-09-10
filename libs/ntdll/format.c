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

FORWARD_FUNCTION(sprintf, rpl_sprintf);
FORWARD_FUNCTION(vsnprintf, rpl__vsnprintf);

typedef struct _format_target_t
{
    int type;
    void *data;
    int count;
} __format_target_t;

typedef enum
{
    FORMAT_FILE = 0,
    FORMAT_BUFFER
} __format_type_t;

static int __emit_wchar(WCHAR c, __format_target_t *stream)
{
    switch (stream->type)
    {
        case FORMAT_BUFFER:
        {
            ((WCHAR *)(stream->data))[stream->count] = c;
            break;
        }
    }
    stream->count++;
    return 1;
}

static int __emit_wstring(const WCHAR *str, __format_target_t *stream)
{
    int chars = 0;

    if (!str)
       str = L"(nil)";

    while (*str)
    {
        switch (stream->type)
        {
            case FORMAT_BUFFER:
            {
                ((WCHAR *)(stream->data))[stream->count++] = *str;
                break;
            }
        }
        str++; chars++;
    }
    return chars;
}

static int __emit_string(const char *str, __format_target_t *stream)
{
    int chars = 0;

    if (!str)
       str = "(null)";

    while (*str)
    {
        switch (stream->type)
        {
            case FORMAT_BUFFER:
            {
                ((WCHAR *)(stream->data))[stream->count++] = *str;
                break;
            }
        }
        str++; chars++;
    }
    return chars;
}

static int __emit_int32(int32_t i, const char *fmt, __format_target_t *stream)
{
    char number[32];
    int chars = snprintf(number, sizeof(number), fmt, i);
    __emit_string(number, stream);
    return chars;
}

static int __emit_int64(int64_t i, const char *fmt, __format_target_t *stream)
{
    char number[64];
    int chars = snprintf(number, sizeof(number), fmt, i);
    __emit_string(number, stream);
    return chars;
}

// FIXME: missing many types, no width, size has no checks
// FIXME: check count/size
static int __format(const WCHAR *format, __format_target_t *stream, size_t count, va_list argv)
{
    size_t size = count;
    WCHAR c;
    const WCHAR *fmt = format;
    char nfmt[32];
    int cf;

    format_next: while ((count > 0) && (c = *fmt++))
    {
        if (c != L'%')
        {
            count -= __emit_wchar(c, stream);
            continue;
        }

        cf = 1; nfmt[0] = '%';

        switch_next: switch ((c = *fmt++))
        {
            case L'c':
                count -= __emit_wchar(va_arg(argv, int), stream);
                goto format_next;

            case L's':
                count -= __emit_wstring(va_arg(argv, WCHAR *), stream);
                goto format_next;

            case L'w': case L'W':
                if ((c = *fmt++) == L's')
                    count -= __emit_wstring(va_arg(argv, WCHAR *), stream);
                 goto format_next;

            case L'h': case L'H':
                if ((c = *fmt++) == L's')
                    count -= __emit_string(va_arg(argv, char *), stream);
                 goto format_next;

            case L'I': case L'i':
                if (*fmt++ != L'6') goto format_next;
                if (*fmt++ != L'4') goto format_next;
                nfmt[cf++] = 'l';
                nfmt[cf++] = 'l';
                switch (*fmt)
                {
                    case L'd': case L'u': case L'x':
                        nfmt[cf++] = (char) *fmt++;
                        break;
                    default:
                        nfmt[cf++] = 'u';
                }
                nfmt[cf] = 0;
                count -= __emit_int64(va_arg(argv, int64_t), nfmt, stream);
                goto format_next;
            case L'd': case L'u': case L'x': case L'X':
                nfmt[cf++] = (char) c;
                nfmt[cf] = 0;
                count -= __emit_int32(va_arg(argv, int32_t), nfmt, stream);
                goto format_next;

            /* width */
            case L'0': case L'1': case L'2': case L'3': case L'4':
            case L'5': case L'6': case L'7': case L'8': case L'9':
                if (cf >= sizeof(nfmt) - sizeof("%llx "))
                {
                    fprintf(stderr, "__format: format prefix overflow\n");
                    abort();
                }
                nfmt[cf++] = (char) c;
                goto switch_next;
            default:
            {
                DECLAREVARCONV(formatA);
                WSTR2STR(format);
                fprintf(stderr, "__format: unsupported type %c in format '%s'\n", c, formatA);
                abort();
            }
        }
    }

    ((WCHAR *)(stream->data))[stream->count++] = 0;
    return size - count;
}

int CDECL rpl_swprintf(WCHAR *buffer, const WCHAR *format, ...)
{
    int retval;
    va_list argptr;

    __format_target_t target;
    target.count = 0;
    target.type = FORMAT_BUFFER;
    target.data = (void *) buffer;
    va_start(argptr, format);

    retval = __format(format, &target, -1, argptr);

    va_end(argptr);
    return retval;
}

int CDECL rpl__vsnwprintf(WCHAR *buffer, size_t count, const WCHAR *format, va_list argptr)
{
    __format_target_t target;
    target.count = 0;
    target.type = FORMAT_BUFFER;
    target.data = (void *) buffer;
    return __format(format, &target, count, argptr);
}

int CDECL rpl_swprintf_s(WCHAR *buffer, size_t sizeOfBuffer, const WCHAR *format, ...)
{
    int retval;
    va_list argptr;

    __format_target_t target;
    target.count = 0;
    target.type = FORMAT_BUFFER;
    target.data = (void *) buffer;
    va_start(argptr, format);

    retval = __format(format, &target, sizeOfBuffer, argptr);

    va_end(argptr);
    return retval;
}
FORWARD_FUNCTION(rpl_swprintf_s, rpl__snwprintf);

ULONG CDECL DbgPrint(char *format, ...)
{
    va_list argptr;
    va_start(argptr, format);
    vprintf(format, argptr);
    va_end(argptr);
    return STATUS_SUCCESS;
}

ULONG CDECL DbgPrintEx(ULONG ComponentId, ULONG Level, const char *format, ...)
{
    va_list argptr;
    va_start(argptr, format);
    vprintf(format, argptr);
    va_end(argptr);
    return STATUS_SUCCESS;
}
