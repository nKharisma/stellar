cmake_minimum_required(VERSION 3.10)
project(StellarSim)

set(CMAKE_CXX_STANDARD 17)

link_directories(lib)
include_directories(include)

add_executable(stellar src/main.cpp src/glad.c)

target_link_libraries(stellar PRIVATE glfw3 opengl32 gdi32)