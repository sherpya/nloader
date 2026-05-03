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
/* All printf-family entries here are ms_abi, so the incoming va_list is
 * Win64-shaped (a `char *`). Use ms_va_list / ms_va_arg consistently — the
 * generic `va_list`/`va_arg` would expand to SysV semantics on Linux x86_64
 * even when the caller is ms_abi, and dereference garbage. */
static CDECL int __format(const WCHAR *format, __format_target_t *stream, size_t count, ms_va_list argv)
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
                count -= __emit_wchar(ms_va_arg(argv, int), stream);
                goto format_next;

            case L's':
                count -= __emit_wstring(ms_va_arg(argv, WCHAR *), stream);
                goto format_next;

            case L'w': case L'W':
                if ((c = *fmt++) == L's')
                    count -= __emit_wstring(ms_va_arg(argv, WCHAR *), stream);
                 goto format_next;

            case L'h': case L'H':
                if ((c = *fmt++) == L's')
                    count -= __emit_string(ms_va_arg(argv, char *), stream);
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
                count -= __emit_int64(ms_va_arg(argv, int64_t), nfmt, stream);
                goto format_next;
            case L'd': case L'u': case L'x': case L'X':
                nfmt[cf++] = (char) c;
                nfmt[cf] = 0;
                count -= __emit_int32(ms_va_arg(argv, int32_t), nfmt, stream);
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
    ms_va_list argptr;

    __format_target_t target;
    target.count = 0;
    target.type = FORMAT_BUFFER;
    target.data = (void *) buffer;
    ms_va_start(argptr, format);

    retval = __format(format, &target, -1, argptr);

    ms_va_end(argptr);
    return retval;
}

int CDECL rpl__vsnwprintf(WCHAR *buffer, size_t count, const WCHAR *format, ms_va_list argptr)
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
    ms_va_list argptr;

    __format_target_t target;
    target.count = 0;
    target.type = FORMAT_BUFFER;
    target.data = (void *) buffer;
    ms_va_start(argptr, format);

    retval = __format(format, &target, sizeOfBuffer, argptr);

    ms_va_end(argptr);
    return retval;
}

/* Walk a 16-bit WCHAR* and emit each unit to `out` as ASCII (non-ASCII → '?').
 * Cannot delegate to glibc's %S: glibc's wchar_t is 32-bit while our WCHAR is
 * 16-bit (-fshort-wchar), so glibc would misalign the read. */
static void __dbg_emit_wstr(const WCHAR *s, FILE *out)
{
    if (!s) { fputs("(null)", out); return; }
    while (*s)
    {
        WCHAR c = *s++;
        fputc(c < 0x80 ? (char) c : '?', out);
    }
}

/* Char-format printer with NT/MS conventions for an ANSI format string:
 *   %s/%hs        → char*           %S/%ws/%ls    → WCHAR* (16-bit)
 *   %c/%hc        → char            %C/%wc/%lc    → WCHAR
 *   %I/%I32/%I64  → MS size prefix; remapped to glibc-friendly ll/(none)
 * Numeric/float specs are forwarded to snprintf with a normalized sub-format.
 * The caller's va_list is ms_abi (CDECL == ms_abi on x86_64 Linux), so we
 * walk it via ms_va_arg — generic va_arg would expand to SysV semantics and
 * desync from the slots placed by the Win64 caller. */
static int __dbg_print(const char *fmt, ms_va_list argv, FILE *out)
{
    char spec[64];
    int total = 0;

    while (*fmt)
    {
        if (*fmt != '%')
        {
            fputc(*fmt++, out);
            total++;
            continue;
        }

        int n = 0;
        spec[n++] = *fmt++;  /* '%' */

        /* flags */
        while (*fmt && strchr("-+ #0", *fmt) && n < (int) sizeof(spec) - 8)
            spec[n++] = *fmt++;
        /* width */
        if (*fmt == '*' && n < (int) sizeof(spec) - 8)
            spec[n++] = *fmt++;
        else
            while (*fmt >= '0' && *fmt <= '9' && n < (int) sizeof(spec) - 8)
                spec[n++] = *fmt++;
        /* precision */
        if (*fmt == '.' && n < (int) sizeof(spec) - 8)
        {
            spec[n++] = *fmt++;
            if (*fmt == '*' && n < (int) sizeof(spec) - 8)
                spec[n++] = *fmt++;
            else
                while (*fmt >= '0' && *fmt <= '9' && n < (int) sizeof(spec) - 8)
                    spec[n++] = *fmt++;
        }

        /* length modifier — track wide/64-bit semantics, normalize for snprintf. */
        int wide_arg = 0;
        int long64 = 0;

        if (*fmt == 'l')
        {
            fmt++;
            if (*fmt == 'l')
            {
                fmt++; long64 = 1;
                spec[n++] = 'l'; spec[n++] = 'l';
            }
            else
                wide_arg = 1;  /* %ls/%lc → WCHAR* / WCHAR */
        }
        else if (*fmt == 'h')
        {
            fmt++;
            if (*fmt == 'h') { fmt++; spec[n++] = 'h'; spec[n++] = 'h'; }
            else { spec[n++] = 'h'; }
        }
        else if (*fmt == 'I')
        {
            fmt++;
            if (fmt[0] == '6' && fmt[1] == '4')
            {
                fmt += 2; long64 = 1;
                spec[n++] = 'l'; spec[n++] = 'l';
            }
            else if (fmt[0] == '3' && fmt[1] == '2')
                fmt += 2;  /* default 32-bit */
            else
            {
                /* %I — pointer-sized; on x86_64 that's 64-bit. */
                long64 = 1;
                spec[n++] = 'l'; spec[n++] = 'l';
            }
        }
        else if (*fmt == 'w')
        {
            fmt++;
            wide_arg = 1;
        }

        char conv = *fmt;
        if (!conv) break;
        fmt++;

        switch (conv)
        {
            case '%':
                fputc('%', out); total++;
                break;

            case 's':
                if (wide_arg)
                    __dbg_emit_wstr(ms_va_arg(argv, WCHAR *), out);
                else
                {
                    char *s = ms_va_arg(argv, char *);
                    fputs(s ? s : "(null)", out);
                }
                break;

            case 'S':
                /* Opposite of default: ANSI fn → WCHAR*. */
                __dbg_emit_wstr(ms_va_arg(argv, WCHAR *), out);
                break;

            case 'c':
                if (wide_arg)
                {
                    int c = ms_va_arg(argv, int) & 0xffff;
                    fputc(c < 0x80 ? (char) c : '?', out);
                }
                else
                    fputc((char) ms_va_arg(argv, int), out);
                break;

            case 'C':
            {
                int c = ms_va_arg(argv, int) & 0xffff;
                fputc(c < 0x80 ? (char) c : '?', out);
                break;
            }

            case 'd': case 'i': case 'u': case 'o': case 'x': case 'X':
            {
                char buf[64];
                spec[n++] = conv;
                spec[n] = 0;
                if (long64)
                    snprintf(buf, sizeof(buf), spec, ms_va_arg(argv, long long));
                else
                    snprintf(buf, sizeof(buf), spec, ms_va_arg(argv, int));
                fputs(buf, out);
                break;
            }

            case 'p':
            {
                char buf[64];
                spec[n++] = conv;
                spec[n] = 0;
                snprintf(buf, sizeof(buf), spec, ms_va_arg(argv, void *));
                fputs(buf, out);
                break;
            }

            case 'f': case 'e': case 'E': case 'g': case 'G': case 'a': case 'A':
            {
                char buf[64];
                spec[n++] = conv;
                spec[n] = 0;
                snprintf(buf, sizeof(buf), spec, ms_va_arg(argv, double));
                fputs(buf, out);
                break;
            }

            default:
                /* Unknown spec — copy literally and stop consuming args. */
                fputc('%', out);
                fputc(conv, out);
                break;
        }
    }

    fflush(out);
    return total;
}

ULONG CDECL DbgPrint(char *format, ...)
{
    ms_va_list argptr;
    ms_va_start(argptr, format);
    __dbg_print(format, argptr, stdout);
    ms_va_end(argptr);
    return STATUS_SUCCESS;
}

ULONG CDECL DbgPrintEx(ULONG ComponentId, ULONG Level, const char *format, ...)
{
    ms_va_list argptr;
    (void) ComponentId;
    (void) Level;
    ms_va_start(argptr, format);
    __dbg_print(format, argptr, stdout);
    ms_va_end(argptr);
    return STATUS_SUCCESS;
}
