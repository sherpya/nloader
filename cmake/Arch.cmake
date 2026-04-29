# Arch.cmake - target architecture selector + base flags interface target.
#
# Mirrors common.mak: ARCH selector (i386/x86_64), per-arch CFLAGS/YASM/objcopy,
# OS-specific tweaks (Linux/FreeBSD/Darwin/MinGW), and threading defaults.

set(NLOADER_ARCH "" CACHE STRING "Target architecture: i386 or x86_64")

if(NOT NLOADER_ARCH)
    set(_host "${CMAKE_HOST_SYSTEM_PROCESSOR}")
    if(_host MATCHES "^(i.86)$")
        set(NLOADER_ARCH "i386" CACHE STRING "" FORCE)
    elseif(_host MATCHES "^(x86_64|amd64|AMD64)$")
        set(NLOADER_ARCH "x86_64" CACHE STRING "" FORCE)
    else()
        message(FATAL_ERROR "Unsupported host architecture '${_host}'; pass -DNLOADER_ARCH=i386 or x86_64")
    endif()
endif()

if(NOT NLOADER_ARCH MATCHES "^(i386|x86_64)$")
    message(FATAL_ERROR "NLOADER_ARCH must be i386 or x86_64 (got '${NLOADER_ARCH}')")
endif()

message(STATUS "NLOADER_ARCH = ${NLOADER_ARCH}")

if(NLOADER_ARCH STREQUAL "i386")
    set(NLOADER_M_FLAG       "-m32")
    set(NLOADER_YASM_MACHINE "x86")
    set(NLOADER_OBJCOPY_BIN  "i386")
    set(NLOADER_OBJCOPY_FMT  "elf32-i386")
else()
    set(NLOADER_M_FLAG       "-m64")
    set(NLOADER_YASM_MACHINE "amd64")
    set(NLOADER_OBJCOPY_BIN  "i386:x86-64")
    set(NLOADER_OBJCOPY_FMT  "elf64-x86-64")
endif()

# YASM output format depends on OS (and arch on Darwin/MinGW which are 32-only here).
if(CMAKE_SYSTEM_NAME MATCHES "Linux|FreeBSD")
    if(NLOADER_ARCH STREQUAL "x86_64")
        set(NLOADER_YASM_FORMAT "elf64")
    else()
        set(NLOADER_YASM_FORMAT "elf32")
    endif()
elseif(APPLE)
    set(NLOADER_YASM_FORMAT "macho32")
elseif(MINGW OR WIN32)
    set(NLOADER_YASM_FORMAT "win32")
else()
    message(FATAL_ERROR "Unsupported system: ${CMAKE_SYSTEM_NAME}")
endif()

# Interface target carrying base flags. Every nloader-built target links it.
add_library(nloader_arch INTERFACE)

target_compile_options(nloader_arch INTERFACE
    ${NLOADER_M_FLAG}
    -O0 -g3 -Wall -Wno-format-truncation
    # Make __FILE__ / debug_str path-independent. CMake compiles from
    # ${CMAKE_BINARY_DIR}/<subdir> with absolute source paths; the make build
    # compiled from ${CMAKE_SOURCE_DIR} with relative paths. The path
    # difference shifted .rodata layout enough to perturb autochk.exe.last's
    # SEGV-vs-clean behaviour. Mapping the source dir to a stable token makes
    # the produced .o byte-equivalent regardless of build dir.
    -ffile-prefix-map=${CMAKE_SOURCE_DIR}/=)

target_compile_definitions(nloader_arch INTERFACE
    _FILE_OFFSET_BITS=64
    LIBNLOADER="/usr/lib/nloader"
    THREADED)

target_link_options(nloader_arch INTERFACE ${NLOADER_M_FLAG})

if(NLOADER_ARCH STREQUAL "x86_64")
    # Modern x86_64 ld.bfd refuses text-relocations in shared objects; build
    # everything PIC so libntdll.dll.so links cleanly. Mirrors common.mak:30.
    target_compile_options(nloader_arch INTERFACE -fPIC)
elseif(NLOADER_ARCH STREQUAL "i386")
    # glibc strcasestr SSE2 path crashes on misaligned i386 stacks; force
    # alignment on entry. Compiler-conditional, mirrors common.mak:21-25.
    if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
        target_compile_options(nloader_arch INTERFACE -mstackrealign)
    elseif(CMAKE_C_COMPILER_ID STREQUAL "GNU")
        target_compile_options(nloader_arch INTERFACE -mincoming-stack-boundary=2)
    endif()
endif()

# Threading: only path supported (no setjmp/longjmp fallback). -pthread is
# explicit (instead of just Threads::Threads) so it propagates to compile too:
# on modern glibc Threads::Threads resolves to no flags, but the make build
# always passed -pthread at compile (-> _REENTRANT) and link, and dropping it
# changed runtime behaviour on i386 (autochk.exe.last no longer SEGV-ed in the
# same place).
find_package(Threads REQUIRED)
target_compile_options(nloader_arch INTERFACE -pthread)
target_link_options(nloader_arch INTERFACE -pthread)
target_link_libraries(nloader_arch INTERFACE Threads::Threads)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_compile_options(nloader_arch INTERFACE -fshort-wchar -Werror)
    target_link_libraries(nloader_arch INTERFACE ${CMAKE_DL_LIBS})
elseif(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    target_compile_options(nloader_arch INTERFACE -fshort-wchar)
elseif(APPLE)
    target_compile_options(nloader_arch INTERFACE -fshort-wchar -mstackrealign)
    target_link_options(nloader_arch INTERFACE
        -Wl,-read_only_relocs,suppress -Wl,-undefined,dynamic_lookup -single-module)
    target_link_libraries(nloader_arch INTERFACE ${CMAKE_DL_LIBS})
endif()
