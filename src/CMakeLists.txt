# Library containing all implementation
add_library(memsim_lib STATIC
    memory/physical_memory.cpp
    allocator/standard_allocator.cpp
    allocator/buddy_allocator.cpp
    cache/cache_level.cpp
    cache/cache_hierarchy.cpp
    virtual_memory/virtual_memory.cpp
    manager/memory_manager.cpp
    cli/command_parser.cpp
    cli/cli.cpp
)

target_include_directories(memsim_lib PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

# All source files implemented!
