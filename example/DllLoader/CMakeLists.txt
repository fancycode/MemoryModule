set (sources_dllloader
  DllLoader.cpp
)

set (sources_dllloaderloader
  DllLoaderLoader.cpp
)

if (NOT MSVC)
    set (CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "-static")
    set (CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "-static")
endif ()

add_executable (DllLoader ${sources_dllloader})
target_link_libraries ("DllLoader" "MemoryModule")
if (NOT MSVC)
    set_target_properties ("DllLoader" PROPERTIES SUFFIX ".exe")
    set_target_properties ("DllLoader" PROPERTIES LINK_FLAGS "-Wl,--image-base -Wl,0x20000000")
endif ()

add_executable (DllLoaderLoader ${sources_dllloaderloader})
target_link_libraries ("DllLoaderLoader" "MemoryModule")
if (NOT MSVC)
    set_target_properties ("DllLoaderLoader" PROPERTIES SUFFIX ".exe")
    set_target_properties ("DllLoaderLoader" PROPERTIES LINK_FLAGS "-Wl,--image-base -Wl,0x10000000")
endif ()
