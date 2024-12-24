# Example CMake command line to create project build files:
#
# *** Windows ***
# cmake -G "Visual Studio 17 2022" -A Win32 -B build -S .
#
# *** Linux ***
# cmake -G "Unix Makefiles" -B build -S .

cmake_minimum_required(VERSION 3.0)
project(MessageSerializer)

add_compile_definitions(_CRT_SECURE_NO_WARNINGS)

# Set C++ standard to C++14
#set(CMAKE_CXX_STANDARD 14)
#set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_CXX_EXTENSIONS OFF)

# Add the executable
add_executable(Serializer main.cpp)