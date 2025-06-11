cmake_minimum_required(VERSION 3.15 FATAL_ERROR)

add_executable(Raytracer WIN32 "raytracer_sample.cpp")

target_link_libraries(Raytracer VikingRenderer)

if(MSVC)
	target_compile_options(Raytracer PRIVATE "/MP")
endif()

find_package(directx-dxc CONFIG REQUIRED)
set(VCPKG_DLL_DIR "${VKR_VCPKG_ROOT_DIR}/installed/x64-windows/bin")

add_custom_command(TARGET Raytracer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${VCPKG_DLL_DIR}"
    $<TARGET_FILE_DIR:Raytracer>
)

find_package(directx12-agility CONFIG REQUIRED)
add_custom_command(TARGET Raytracer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_PROPERTY:Microsoft::DirectX12-Layers,IMPORTED_LOCATION_DEBUG> 
    $<TARGET_FILE_DIR:Raytracer>
)