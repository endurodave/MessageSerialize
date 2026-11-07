# Example CMake command line to create project build files:
#
# cmake -B build .
#
# Fixed-block allocator build (dmq::xstring, xlist, xmap, ... use the
# allocator/ pools instead of the global heap):
#
# cmake -B build -DMSG_SERIALIZE_ALLOCATOR=ON .

cmake_minimum_required(VERSION 3.10)
project(MessageSerializer)

add_compile_definitions(_CRT_SECURE_NO_WARNINGS)

# Set C++ standard to C++14 minimum
#set(CMAKE_CXX_STANDARD 14)
#set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_CXX_EXTENSIONS OFF)

option(MSG_SERIALIZE_ALLOCATOR "Use the fixed-block allocator (allocator/) instead of the global heap" OFF)
option(MSG_SERIALIZE_ALLOCATOR_SAFEGUARDS "Fixed-block allocator: guard words and checks on every block" OFF)

if(MSG_SERIALIZE_ALLOCATOR)
    add_compile_definitions(DMQ_ALLOCATOR)
    if(MSG_SERIALIZE_ALLOCATOR_SAFEGUARDS)
        add_compile_definitions(DMQ_ALLOCATOR_SAFEGUARDS)
    endif()
    message(STATUS "MessageSerialize: fixed-block allocator")
else()
    message(STATUS "MessageSerialize: global heap")
endif()

# Find package threads (xallocator uses std::mutex)
find_package(Threads REQUIRED)

# Add the executable: examples (main.cpp) and unit tests (SerializeTests.cpp).
# The allocator sources compile to nothing unless MSG_SERIALIZE_ALLOCATOR is ON.
add_executable(Serializer
    main.cpp
    SerializeTests.cpp
    allocator/Allocator.cpp
    allocator/xallocator.cpp
)
target_include_directories(Serializer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/allocator)
target_link_libraries(Serializer PRIVATE Threads::Threads)

# Visual Studio: make Serializer the startup project instead of ALL_BUILD
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Serializer)
