# PeBlob.cmake - wrap a binary file (PE executable) into a linkable .o blob
# via objcopy -I binary, mirroring the autochk.exe handling in Makefile.

find_program(OBJCOPY_EXECUTABLE objcopy REQUIRED)

function(add_pe_blob OUT_VAR)
    cmake_parse_arguments(P "" "SOURCE" "" ${ARGN})

    if(NOT P_SOURCE)
        message(FATAL_ERROR "add_pe_blob: SOURCE is required")
    endif()

    get_filename_component(_name ${P_SOURCE} NAME_WE)
    get_filename_component(_basename ${P_SOURCE} NAME)
    get_filename_component(_dir ${P_SOURCE} DIRECTORY)
    set(_out ${CMAKE_CURRENT_BINARY_DIR}/${_name}_blob.o)

    # objcopy -I binary derives symbol names (_binary_<input>_start etc.) from
    # the input PATH as given. Run from the source directory with a bare
    # basename so the symbols match the names autochk.c expects.
    add_custom_command(
        OUTPUT  ${_out}
        COMMAND ${OBJCOPY_EXECUTABLE}
                -B ${NLOADER_OBJCOPY_BIN}
                -I binary
                -O ${NLOADER_OBJCOPY_FMT}
                ${_basename} ${_out}
        WORKING_DIRECTORY ${_dir}
        DEPENDS ${P_SOURCE}
        COMMENT "OBJCOPY blob ${P_SOURCE}"
        VERBATIM)

    set(${OUT_VAR} ${_out} PARENT_SCOPE)
endfunction()
