%ifidn __OUTPUT_FORMAT__,win32
    %define mangle(x) _ %+ x
    %define RODATA .rdata
%elifidn __OUTPUT_FORMAT__,macho32
    %define mangle(x) _ %+ x
    %define RODATA .rodata
%else
    %define mangle(x) x
    %define RODATA .rodata
%endif

; TEB->LogModule offset (Win64 layout)
; Verified at C build time via static_assert(offsetof(TEB, LogModule) == ...).
%define TEB_LOGMODULE_OFFSET 0xf0
%define TEB_STACKBASE_OFFSET 0x08
%define TEB_PEB_OFFSET       0x60

%macro cextern 1
    %xdefine %1 mangle(%1)
    extern %1
%endmacro

%macro cfunction 1
    %xdefine %1 mangle(%1)
    global %1:function
    %1:
%endmacro
