cmake_minimum_required(VERSION 3.15)
project(MassSpring)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optimiztaion flags
add_compile_options(-O3 -march=native)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

find_package(SFML 2.5 COMPONENTS graphics window system REQUIRED)

add_executable(MassSpring
    src/main.cpp
)

target_include_directories(MassSpring PRIVATE
    external/SFML
)

target_link_libraries(MassSpring
    sfml-graphics
    sfml-window
    sfml-system
)