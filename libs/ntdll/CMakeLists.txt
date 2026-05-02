# libs/ntdll/CMakeLists.txt
#
# Builds libs/ntdll/libntdll.a — the static archive linked into nloader and
# autochk. The shared variant is gone now that imports are resolved via the
# in-process nl_exports table (see exports.c) instead of dlopen/dlsym.

file(GLOB NTDLL_C_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

# Assembly: _chkstk on every arch, _aulldvrm + bitscan only on i386 (the x86_64
# compiler emits the equivalent native instructions inline).
#
# Order matches the make build (libs/ntdll/Makefile): _aulldvrm, bitscan,
# _chkstk on i386. The link order shifts where each routine lands and was
# enough to perturb autochk.exe.last's SEGV behaviour on the smoke test.
if(NLOADER_ARCH STREQUAL "i386")
    set(NTDLL_ASM_NAMES _aulldvrm.asm bitscan.asm _chkstk.asm)
else()
    set(NTDLL_ASM_NAMES _chkstk.asm)
endif()

set(NTDLL_ASM_OBJS "")
foreach(_asm IN LISTS NTDLL_ASM_NAMES)
    add_yasm_object(_obj
        SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/${_asm})
    list(APPEND NTDLL_ASM_OBJS ${_obj})
endforeach()

# Mark the YASM-produced files as externally generated objects so CMake passes
# them straight to the linker (and to `ar` for the static archive) without
# trying to compile them.
set_source_files_properties(${NTDLL_ASM_OBJS} PROPERTIES
    EXTERNAL_OBJECT TRUE
    GENERATED       TRUE)

# Static archive at libs/ntdll/libntdll.a. Consumers must link with
# `-Wl,--whole-archive` so the nl_export records in exports.c (no other
# references) are not dropped by --gc-sections.
add_library(ntdll_static STATIC ${NTDLL_C_SOURCES} ${NTDLL_ASM_OBJS})
target_link_libraries(ntdll_static PRIVATE nloader_arch)
# Match the make build: no -fPIC on i386. See history for why this affected
# autochk.exe.last's SEGV behaviour.
set_target_properties(ntdll_static PROPERTIES
    OUTPUT_NAME              ntdll
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/libs/ntdll
    POSITION_INDEPENDENT_CODE OFF
    LINKER_LANGUAGE          C)
