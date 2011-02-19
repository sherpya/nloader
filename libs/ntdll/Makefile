top=../..

include $(top)/common.mak

all: $(top)/libs/ntdll.dll.so

ntdll_sources=$(wildcard $(top)/libs/ntdll/*.c)
ntdll_objects=$(ntdll_sources:.c=.o)
ntdll_asm=$(wildcard $(top)/libs/ntdll/*.asm)
ntdll_objects+=$(ntdll_asm:.asm=.o)

$(top)/libs/ntdll/libntdll.a: $(ntdll_objects) ntdll.dll.def
	$(AR) cru $@ $(ntdll_objects)
	$(RANLIB) $@

$(top)/libs/ntdll.dll.so: $(top)/libs/ntdll/libntdll.a
	$(MKSO) -o $@ $^

clean:
	rm -f $(top)/libs/ntdll.dll.so $(top)/libs/ntdll/libntdll.a $(ntdll_objects) *.d

-include $(ntdll_sources:.c=.d)

.PHONY: all clean
