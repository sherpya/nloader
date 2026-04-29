# libs/ntdll/CMakeLists.txt
#
# Builds:
#   libs/ntdll.dll.so       - shared (Linux/FreeBSD); via dllwrap on MinGW
#   libs/ntdll/libntdll.a   - static, used by the standalone autochk binary

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
    if(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
        add_yasm_object(_obj
            SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/${_asm}
            EXTRA_FLAGS -Dstderr=__stderrp)
    else()
        add_yasm_object(_obj
            SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/${_asm})
    endif()
    list(APPEND NTDLL_ASM_OBJS ${_obj})
endforeach()

# Mark the YASM-produced files as externally generated objects so CMake passes
# them straight to the linker (and to `ar` for the static archive) without
# trying to compile them.
set_source_files_properties(${NTDLL_ASM_OBJS} PROPERTIES
    EXTERNAL_OBJECT TRUE
    GENERATED       TRUE)

# Object library for the C sources; reused by both static and shared variants
# so each TU is compiled exactly once. ASM objects are added separately to
# each consumer because $<TARGET_OBJECTS:> does not propagate EXTERNAL_OBJECTs.
#
# POSITION_INDEPENDENT_CODE=OFF: CMake otherwise auto-adds -fPIC because the
# objs feed into a SHARED lib. The make build does not pass -fPIC on i386 (it
# accepts the resulting DT_TEXTREL warning), and the slightly different code
# gen between -fPIC and Debian's default -fPIE shifts where autochk.exe.last
# happens to SEGV. nloader_arch still adds -fPIC on x86_64 (where text relocs
# are rejected by ld.bfd), so this only changes behaviour on i386.
add_library(ntdll_objs OBJECT ${NTDLL_C_SOURCES})
target_link_libraries(ntdll_objs PRIVATE nloader_arch)
set_target_properties(ntdll_objs PROPERTIES POSITION_INDEPENDENT_CODE OFF)

# Static archive at libs/ntdll/libntdll.a (preserved path; autochk links it).
add_library(ntdll_static STATIC $<TARGET_OBJECTS:ntdll_objs> ${NTDLL_ASM_OBJS})
target_link_libraries(ntdll_static PRIVATE nloader_arch)
set_target_properties(ntdll_static PROPERTIES
    OUTPUT_NAME              ntdll
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/libs/ntdll
    LINKER_LANGUAGE          C)

# Shared library at libs/ntdll.dll.so. nt_compat.h:load_library() probes for
# `libs/<name>.so` relative to cwd, so the file name and path matter.
if(MINGW)
    add_custom_command(
        OUTPUT  ${CMAKE_BINARY_DIR}/libs/ntdll.dll
        COMMAND dllwrap --def ${CMAKE_CURRENT_SOURCE_DIR}/ntdll.dll.def
                -o ${CMAKE_BINARY_DIR}/libs/ntdll.dll
                $<TARGET_OBJECTS:ntdll_objs>
        DEPENDS ntdll_objs ${CMAKE_CURRENT_SOURCE_DIR}/ntdll.dll.def
        COMMAND_EXPAND_LISTS VERBATIM)
    add_custom_target(ntdll_shared_target ALL
        DEPENDS ${CMAKE_BINARY_DIR}/libs/ntdll.dll)
    add_library(ntdll_shared INTERFACE)
    add_dependencies(ntdll_shared ntdll_shared_target)
else()
    add_library(ntdll_shared SHARED
        $<TARGET_OBJECTS:ntdll_objs>
        ${NTDLL_ASM_OBJS})
    target_link_libraries(ntdll_shared PRIVATE nloader_arch)
    set_target_properties(ntdll_shared PROPERTIES
        OUTPUT_NAME              ntdll.dll
        PREFIX                   ""
        SUFFIX                   ".so"
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/libs
        LINKER_LANGUAGE          C
        # Match the make build: no SONAME entry, no auto -fPIC. See ntdll_objs
        # comment above for the i386 codegen reasoning.
        POSITION_INDEPENDENT_CODE OFF
        NO_SONAME                TRUE)
endif()
