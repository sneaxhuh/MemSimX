# Google Test
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.12.1
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Unit tests
add_executable(unit_tests
    unit/test_physical_memory.cpp
)

target_link_libraries(unit_tests
    memsim_lib
    gtest_main
)

# Integration tests (will be added later)
# add_executable(integration_tests
#     integration/test_full_system.cpp
# )
# target_link_libraries(integration_tests
#     memsim_lib
#     gtest_main
# )

include(GoogleTest)
gtest_discover_tests(unit_tests)
