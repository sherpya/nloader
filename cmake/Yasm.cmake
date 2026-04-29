# Yasm.cmake - assemble .asm sources with yasm into .o files.
#
# Usage:
#   add_yasm_object(VAR SOURCE path/to/foo.asm [EXTRA_FLAGS -Dxxx ...])
# Sets VAR (in caller scope) to the absolute path of the produced object,
# which can be added to add_library/add_executable source lists.

find_program(YASM_EXECUTABLE yasm REQUIRED)

function(add_yasm_object OUT_VAR)
    cmake_parse_arguments(P "" "SOURCE" "EXTRA_FLAGS" ${ARGN})

    if(NOT P_SOURCE)
        message(FATAL_ERROR "add_yasm_object: SOURCE is required")
    endif()

    get_filename_component(_name ${P_SOURCE} NAME_WE)
    set(_out ${CMAKE_CURRENT_BINARY_DIR}/${_name}.yasm.o)

    add_custom_command(
        OUTPUT  ${_out}
        COMMAND ${YASM_EXECUTABLE}
                ${P_EXTRA_FLAGS}
                -I${CMAKE_SOURCE_DIR}
                -g dwarf2
                -a x86 -m ${NLOADER_YASM_MACHINE}
                -f ${NLOADER_YASM_FORMAT}
                -o ${_out} ${P_SOURCE}
        DEPENDS ${P_SOURCE} ${CMAKE_SOURCE_DIR}/asmdefs.inc
        COMMENT "YASM ${P_SOURCE}"
        VERBATIM)

    set(${OUT_VAR} ${_out} PARENT_SCOPE)
endfunction()
