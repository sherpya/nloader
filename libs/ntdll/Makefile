
ntdll_sources   =   $(wildcard $(top)/libs/ntdll/*.c)
ntdll_objects   =   $(ntdll_sources:.c=.o)
ntdll_asm       =   $(wildcard $(top)/libs/ntdll/*.asm)

# i386-only assembly helpers (the 64-bit compiler emits native instructions instead)
ifeq ($(ARCH),x86_64)
    ntdll_asm   :=  $(filter-out %_aulldvrm.asm %bitscan.asm,$(ntdll_asm))
endif

ntdll_objects   +=  $(ntdll_asm:.asm=.o)
ntdll_deps      =   $(ntdll_sources:.c=.d)

LIBS            += $(top)/libs/ntdll.dll.so
STATIC_LIBS     += $(top)/libs/ntdll/libntdll.a
CLEAN_TARGETS   += $(ntdll_objects) $(ntdll_deps)

$(top)/libs/ntdll/libntdll.a: $(ntdll_objects)
	$(AR) cru $@ $(ntdll_objects)
	$(RANLIB) $@

$(top)/libs/ntdll.dll.so: $(ntdll_objects)
	$(MKSO) -o $@ $(ntdll_objects)

-include $(ntdll_deps)
