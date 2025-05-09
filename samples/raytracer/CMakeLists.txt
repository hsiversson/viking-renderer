cmake_minimum_required(VERSION 3.15 FATAL_ERROR)

add_executable(Raytracer WIN32 "raytracer_sample.cpp")

target_include_directories(Raytracer PRIVATE ${VKR_SRC_DIR})
target_link_libraries(Raytracer VikingRenderer)