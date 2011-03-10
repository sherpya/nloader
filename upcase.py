import ctypes
import textwrap

DWORD       = ctypes.c_uint
USHORT      = ctypes.c_ushort
PVOID       = ctypes.c_void_p
NTSTATUS    = ctypes.c_long

class UNICODE_STRING(ctypes.Structure):
    _fields_ = [
        ("Length",          USHORT),
        ("MaximumLength",   USHORT),
        ("Buffer",          PVOID),
    ]

if __name__ == '__main__':
    length = 65535
    low = u''.join([ unichr(c) for c in xrange(1, length) ])

    srcbuf = ctypes.create_unicode_buffer(low, length)
    src = UNICODE_STRING(ctypes.sizeof(srcbuf), ctypes.sizeof(srcbuf), ctypes.addressof(srcbuf))

    dstbuf = ctypes.create_unicode_buffer(u'', length)
    dst = UNICODE_STRING(ctypes.sizeof(dstbuf), ctypes.sizeof(dstbuf), ctypes.addressof(dstbuf))

    res = ctypes.windll.ntdll.RtlUpcaseUnicodeString(ctypes.addressof(dst), ctypes.addressof(src), DWORD(0))

    if res:
        print 'Error', res
        raise SystemExit

    values = [ '0x0000' ] + [ '0x%04x' % ord(x) for x in dstbuf.value ]
    values = ', '.join(values)

    table  = '// Unicode UpperCase Table, automatically generated\n\n'
    table += 'static const WCHAR UpcaseTable[65536] = \n{\n    '
    table += '\n    '.join(textwrap.wrap(values, 72))
    table += '\n};\n'
    open('libs/ntdll/upcasetable.h', 'wb').write(table)
