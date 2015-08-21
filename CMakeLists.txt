project (MemoryModule)
cmake_minimum_required (VERSION 2.8.7)

set (PLATFORM "x86_64" CACHE STRING "Platform to compile for")
message (STATUS "Compile for ${PLATFORM} platform")

if (NOT MSVC)
    set (CMAKE_SYSTEM_NAME Windows)
    set (CMAKE_POSITION_INDEPENDENT_CODE False)

    set (COMPILER_PREFIX "${PLATFORM}-w64-mingw32")
    set (CMAKE_C_COMPILER "${COMPILER_PREFIX}-gcc")
    set (CMAKE_CXX_COMPILER "${COMPILER_PREFIX}-g++")
    set (CMAKE_RC_COMPILER "${COMPILER_PREFIX}-windres")
    set (CMAKE_AR "${COMPILER_PREFIX}-ar")
    set (CMAKE_RANLIB "${COMPILER_PREFIX}-ranlib")

    set (CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
    set (CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
    set (CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

    set (CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
    set (CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

    set (CMAKE_RC_COMPILE_OBJECT "${CMAKE_RC_COMPILER} -O coff -I${CMAKE_CURRENT_SOURCE_DIR} <SOURCE> <OBJECT>")
endif ()

if (NOT MSVC)
    add_definitions ("-Wall")
else ()
    add_definitions ("-W4")
endif ()

option(UNICODE "Compile with UNICODE support" OFF)
if (UNICODE)
    message (STATUS "Compile with UNICODE support")
    add_definitions ("-DUNICODE" "-D_UNICODE")
else ()
    message (STATUS "Compile without UNICODE support")
endif ()

add_library (MemoryModule STATIC MemoryModule.c MemoryModule.h)
if (NOT MSVC)
    set_target_properties ("MemoryModule" PROPERTIES PREFIX "")
endif ()

add_subdirectory (example)

enable_language (RC)
