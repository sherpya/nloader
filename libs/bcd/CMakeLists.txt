# libs/bcd/CMakeLists.txt
#
# Builds:
#   libs/bcd.dll.so   - placeholder shared lib that logs every BCD call

set(BCD_C_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/bcd.c")

if(MINGW)
    add_library(bcd_objs OBJECT ${BCD_C_SOURCES})
    target_link_libraries(bcd_objs PRIVATE nloader_arch)
    set_target_properties(bcd_objs PROPERTIES POSITION_INDEPENDENT_CODE OFF)
    add_custom_command(
        OUTPUT  ${CMAKE_BINARY_DIR}/libs/bcd.dll
        COMMAND dllwrap --def ${CMAKE_CURRENT_SOURCE_DIR}/bcd.dll.def
                -o ${CMAKE_BINARY_DIR}/libs/bcd.dll
                $<TARGET_OBJECTS:bcd_objs>
        DEPENDS bcd_objs ${CMAKE_CURRENT_SOURCE_DIR}/bcd.dll.def
        COMMAND_EXPAND_LISTS VERBATIM)
    add_custom_target(bcd_shared_target ALL
        DEPENDS ${CMAKE_BINARY_DIR}/libs/bcd.dll)
else()
    add_library(bcd_shared SHARED ${BCD_C_SOURCES})
    target_link_libraries(bcd_shared PRIVATE nloader_arch)
    set_target_properties(bcd_shared PROPERTIES
        OUTPUT_NAME              bcd.dll
        PREFIX                   ""
        SUFFIX                   ".so"
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/libs
        LINKER_LANGUAGE          C
        POSITION_INDEPENDENT_CODE OFF
        NO_SONAME                TRUE)
endif()
