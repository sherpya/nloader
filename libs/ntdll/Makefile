top=../..

include $(top)/common.mak

ntdll_sources=$(wildcard $(top)/libs/ntdll/*.c)
ntdll_objects=$(ntdll_sources:.c=.o)
ntdll_asm=$(wildcard $(top)/libs/ntdll/*.asm)
ntdll_objects+=$(ntdll_asm:.asm=.o)

$(top)/libs/ntdll.dll.so: $(ntdll_objects) ntdll.dll.def
	$(MKSO) -o $@ $(ntdll_objects)

all: $(top)/libs/ntdll.dll.so

clean:
	rm -f $(top)/libs/ntdll.dll.so $(ntdll_objects) *.d

-include $(ntdll_sources:.c=.d)

.PHONY: all clean
