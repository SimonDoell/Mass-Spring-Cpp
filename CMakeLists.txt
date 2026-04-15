cmake_minimum_required(VERSION 3.15)
project(Boilerplate_SFML)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-O3 -march=native)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

find_package(SFML 2.5 COMPONENTS graphics window system REQUIRED)

add_executable(Boilerplate_SFML
    src/main.cpp
)

target_include_directories(Boilerplate_SFML PRIVATE
    external/SFML
)

target_link_libraries(Boilerplate_SFML
    sfml-graphics
    sfml-window
    sfml-system
)