cmake_minimum_required(VERSION 3.20)
project(nloader C)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

include(Arch)      # NLOADER_ARCH + interface target nloader_arch
include(Yasm)      # add_yasm_object()
include(PeBlob)    # add_pe_blob()

# Allow targets in libs/ to reference top-level headers (nt_structs.h, nt_compat.h).
target_include_directories(nloader_arch INTERFACE ${CMAKE_SOURCE_DIR})

add_subdirectory(libs)

# stubs.asm trampolines for nloader/autochk; arch-dependent source.
if(NLOADER_ARCH STREQUAL "x86_64")
    add_yasm_object(STUBS_OBJ SOURCE ${CMAKE_SOURCE_DIR}/stubs_x64.asm)
else()
    add_yasm_object(STUBS_OBJ SOURCE ${CMAKE_SOURCE_DIR}/stubs.asm)
endif()

# Main loader binary. ntdll.dll.so is loaded at runtime via dlopen
# (nt_compat.h:load_library), so we only need a build-order dependency,
# not a real link against ntdll_shared. --as-needed would drop it anyway.
add_executable(nloader nloader.c loader.c ${STUBS_OBJ})
target_link_libraries(nloader PRIVATE nloader_arch)
add_dependencies(nloader ntdll_shared)

# lznt1: standalone CLI built from the same source as the in-ntdll lznt1.c
# but with -DMAIN to expose its main().
add_executable(lznt1 ${CMAKE_SOURCE_DIR}/libs/ntdll/lznt1.c)
target_compile_definitions(lznt1 PRIVATE MAIN)
target_link_libraries(lznt1 PRIVATE nloader_arch)

# autochk: standalone build that embeds autochk.exe via objcopy + statically
# links the whole libntdll.a. Only built if the .exe is present in the tree.
if(EXISTS ${CMAKE_SOURCE_DIR}/autochk.exe)
    add_pe_blob(AUTOCHK_BLOB SOURCE ${CMAKE_SOURCE_DIR}/autochk.exe)
    add_executable(autochk autochk.c loader.c ${STUBS_OBJ} ${AUTOCHK_BLOB})
    target_link_libraries(autochk PRIVATE
        nloader_arch
        -Wl,--whole-archive ntdll_static -Wl,--no-whole-archive)
    target_link_options(autochk PRIVATE -rdynamic)
endif()

# MinGW-only utility (mirrors common.mak:83).
if(MINGW)
    add_executable(volumeinfo volumeinfo.c)
    target_link_libraries(volumeinfo PRIVATE nloader_arch ntdll_shared)
endif()

# disk.img: NTFS image used as the autochk target. Not part of `all`; build
# explicitly with `cmake --build . --target disk_img`.
if(CMAKE_SYSTEM_NAME MATCHES "Linux|FreeBSD")
    add_custom_target(disk_img
        COMMAND dd if=/dev/zero of=${CMAKE_BINARY_DIR}/disk.img bs=1 count=0 seek=2072738304
        COMMAND env PATH=$ENV{PATH}:/sbin:/usr/sbin
                mkntfs -Ff -p 63 -L Native -H 255 -s 512 -S 63 ${CMAKE_BINARY_DIR}/disk.img
        BYPRODUCTS ${CMAKE_BINARY_DIR}/disk.img
        COMMENT "Creating disk.img via mkntfs"
        VERBATIM)
endif()
