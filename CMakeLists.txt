cmake_minimum_required(VERSION 3.25)
project(Lab_4_Shaders)

set(CMAKE_CXX_STANDARD 20)
set(SDL2_INCLUDE_DIR C:/Users/javie/Documents/SDL2-devel-2.28.1-VC/SDL2-2.28.1/include)
set(SDL2_LIB_DIR C:/Users/javie/Documents/SDL2-devel-2.28.1-VC/SDL2-2.28.1/lib/x64)

include_directories(${SDL2_INCLUDE_DIR})
link_directories(${SDL2_LIB_DIR})


add_executable(Lab_4_Shaders main.cpp GraphicsHeaders.h Shaders.h FastNoiseLite.h FrameBuffer.h)

target_link_libraries(${PROJECT_NAME} SDL2main SDL2)