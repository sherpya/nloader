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

%macro cextern 1
    %xdefine %1 mangle(%1)
    extern %1
%endmacro

%macro cfunction 1
    %xdefine %1 mangle(%1)
    global %1:function
    %1:
%endmacro
