# libs/bcd/CMakeLists.txt
#
# Builds libs/bcd/libbcd.a — static archive linked into nloader/autochk.
# Imports are resolved through the in-process nl_exports table (see
# exports.c); the old .so / dlsym path is gone.

file(GLOB BCD_C_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

add_library(bcd_static STATIC ${BCD_C_SOURCES})
target_link_libraries(bcd_static PRIVATE nloader_arch)
set_target_properties(bcd_static PROPERTIES
    OUTPUT_NAME              bcd
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/libs/bcd
    POSITION_INDEPENDENT_CODE OFF
    LINKER_LANGUAGE          C)
