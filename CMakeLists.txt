cmake_minimum_required(VERSION 3.20)

set(CMAKE_C_COMPILER "clang")
set(CMAKE_CXX_COMPILER "clang++")

project(Columnar-Engine LANGUAGES CXX)

set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Wall -Wextra -Wpedantic -Werror -stdlib=libc++ -Wconversion -Wshadow -Wzero-as-null-pointer-constant -Wnull-dereference)
    add_link_options(-stdlib=libc++)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-O0 -g -fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(-O3 -march=native)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(src)
add_subdirectory(tests)
