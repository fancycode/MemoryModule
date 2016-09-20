set (sources_testsuite
  TestSuite.c
)

if (NOT MSVC)
    set (CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "-static")
    set (CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "-static")
endif ()

add_executable (TestSuite ${sources_testsuite})
target_link_libraries ("TestSuite" "MemoryModule")
if (NOT MSVC)
    set_target_properties ("TestSuite" PROPERTIES SUFFIX ".exe")
endif ()
