cmake_minimum_required(VERSION 3.15 FATAL_ERROR)

file(GLOB_RECURSE VKR_DEMO_SRC CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
)

file(GLOB VKR_DEMO_PRECOMPILED_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/src/vkr_demo_precompiled.h)
source_group("Precompiled Header" FILES ${VKR_DEMO_PRECOMPILED_HEADER})

add_executable(VikingDemo WIN32 ${VKR_DEMO_SRC} ${VKR_ICON_RESOURCE})

target_precompile_headers(VikingDemo PRIVATE ${VKR_DEMO_PRECOMPILED_HEADER})
source_group("Generated Files" FILES ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/VikingDemo.dir/cmake_pch.cxx)
foreach(config_type ${CMAKE_CONFIGURATION_TYPES})
    source_group("Generated Files" FILES ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/VikingDemo.dir/${config_type}/cmake_pch.hxx)
endforeach()

target_link_libraries(VikingDemo VikingRenderer)
target_link_libraries(VikingDemo ${STREAMLINE_LIB_DIR}/sl.interposer.lib)

# Copy required runtime DLLs for NVidia Streamline
add_custom_command(TARGET VikingDemo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${STREAMLINE_DLL_DIR}/sl.common.dll
        ${STREAMLINE_DLL_DIR}/sl.dlss.dll
        ${STREAMLINE_DLL_DIR}/sl.dlss_d.dll
        ${STREAMLINE_DLL_DIR}/sl.interposer.dll
        ${STREAMLINE_DLL_DIR}/nvngx_dlss.dll
        ${STREAMLINE_DLL_DIR}/nvngx_dlssd.dll
        $<TARGET_FILE_DIR:VikingDemo>
)

if(MSVC)
	target_compile_options(VikingDemo PRIVATE "/MP")
endif()

find_package(directx-dxc CONFIG REQUIRED)
set(VCPKG_DLL_DIR "${VKR_VCPKG_ROOT_DIR}/installed/x64-windows/bin")

add_custom_command(TARGET VikingDemo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${VCPKG_DLL_DIR}"
    $<TARGET_FILE_DIR:VikingDemo>
)

find_package(directx12-agility CONFIG REQUIRED)
add_custom_command(TARGET VikingDemo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_PROPERTY:Microsoft::DirectX12-Layers,IMPORTED_LOCATION_DEBUG> 
    $<TARGET_FILE_DIR:VikingDemo>
)