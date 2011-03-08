CFLAGS  = -I$(top)
CFLAGS += -D_FILE_OFFSET_BITS=64
CFLAGS += -m32
CFLAGS += -MD -MP
CFLAGS += -O0 -g3 -Wall
CFLAGS += -DLIBNLOADER=\"/usr/lib/nloader\"

ifdef RELEASE
CFLAGS  += -O2
LDFLAGS += -s
endif

TARGETS = nloader lznt1 libs

RANLIB = ranlib

YFMT = elf32
YDBG = dwarf2
CC := $(shell which ccache 2>/dev/null) $(CC)
OS = $(shell uname -s)

ifneq (,$(findstring Linux, $(OS)))
	TARGETS += disk.img
	CFLAGS += -fshort-wchar
	LDFLAGS += -ldl
	MKSO = $(CC) $(CFLAGS) -shared
ifneq (,$(wildcard autochk.exe))
	TARGETS += autochk
	LDALONE = -rdynamic -Wl,--whole-archive -L$(top)/libs/ntdll -lntdll -Wl,--no-whole-archive
endif
else ifneq (,$(findstring FreeBSD, $(OS)))
	TARGETS += disk.img
	CFLAGS += -fshort-wchar
	MKSO = $(CC) $(CFLAGS) -shared
	YFLAGS = -Dstderr=__stderrp
else ifneq (,$(findstring Darwin, $(OS)))
	CFLAGS += -fshort-wchar -mstackrealign
	LDFLAGS += -ldl -Wl,-read_only_relocs,suppress,-undefined,dynamic_lookup -single-module
	YFMT = macho32
	YDBG = null
	MKSO = $(CC) $(CFLAGS) $(LDFLAGS) -dynamiclib
else ifneq (,$(findstring MINGW32, $(OS)))
	TARGETS += volumeinfo
	MKSO = dllwrap --def $(basename $(@F)).def
	EXE = .exe
	YFMT = win32
else
$(error $(OS) is not supported)
endif

#CFLAGS += -DDEBUG_HEAP
#CFLAGS += -DDEBUG_CRT

#CFLAGS += -DTHREADED -pthread

#CFLAGS += -DREDIR_IO
#CFLAGS += -DREDIR_SYSCALL

%.o: %.c Makefile $(top)/common.mak
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.asm Makefile $(top)/common.mak $(top)/asmdefs.inc
	yasm $(YFLAGS) -I$(top) -g $(YDBG) -a x86 -f $(YFMT) -o $@ $<

# disable most of builtin rules
.SUFFIXES:
