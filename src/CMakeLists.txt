cmake_minimum_required(VERSION 3.15 FATAL_ERROR)

file(GLOB_RECURSE VKR_CORE_SOURCES ${VKR_SRC_DIR}/core/*.cpp ${VKR_SRC_DIR}/core/*.h)
source_group(Core FILES ${VKR_CORE_SOURCES})

file(GLOB_RECURSE VKR_APPLICATION_SOURCES ${VKR_SRC_DIR}/application/*.cpp ${VKR_SRC_DIR}/application/*.h)
source_group(Application FILES ${VKR_APPLICATION_SOURCES})

file(GLOB_RECURSE VKR_GRAPHICS_SOURCES ${VKR_SRC_DIR}/graphics/*.cpp ${VKR_SRC_DIR}/graphics/*.h)
source_group(Graphics FILES ${VKR_GRAPHICS_SOURCES})

file(GLOB_RECURSE VKR_SHADER_SOURCES ${VKR_SRC_DIR}/../content/shaders/*.hlsl ${VKR_SRC_DIR}/../content/shaders/*.hlsli)
source_group(Shaders FILES ${VKR_SHADER_SOURCES})
set_source_files_properties(${VKR_SHADER_SOURCES} PROPERTIES HEADER_FILE_ONLY 1)

file(GLOB_RECURSE VKR_RENDER_SOURCES ${VKR_SRC_DIR}/render/*.cpp ${VKR_SRC_DIR}/render/*.h)
source_group(Render FILES ${VKR_RENDER_SOURCES})

file(GLOB_RECURSE VKR_UTILS_SOURCES ${VKR_SRC_DIR}/utils/*.cpp ${VKR_SRC_DIR}/utils/*.h)
source_group(Utils FILES ${VKR_UTILS_SOURCES})

file(GLOB_RECURSE VKR_EDITOR_SOURCES ${VKR_SRC_DIR}/editor/*.cpp ${VKR_SRC_DIR}/editor/*.h)
source_group(Editor FILES ${VKR_EDITOR_SOURCES})

file(GLOB_RECURSE VKR_GAME_SOURCES ${VKR_SRC_DIR}/game/*.cpp ${VKR_SRC_DIR}/game/*.h)
source_group(Game FILES ${VKR_GAME_SOURCES})

file(GLOB VKR_PRECOMPILED_HEADER ${VKR_SRC_DIR}/vkr_precompiled.h)
source_group("Precompiled Header" FILES ${VKR_PRECOMPILED_HEADER})

file(GLOB_RECURSE VKR_GENERATED_SOURCES ${VKR_GENERATED_SRC_DIR}/*.generated.h ${VKR_GENERATED_SRC_DIR}/*.generated.cpp)
source_group("Generated Files" FILES ${VKR_GENERATED_SOURCES})

add_library(VikingRenderer 
			${VKR_CORE_SOURCES}
			${VKR_APPLICATION_SOURCES}
			${VKR_GRAPHICS_SOURCES}
			${VKR_SHADER_SOURCES} 
			${VKR_RENDER_SOURCES}
            ${VKR_UTILS_SOURCES}
            ${VKR_EDITOR_SOURCES}
			${VKR_GAME_SOURCES}
			${VKR_PRECOMPILED_HEADER}
			${VKR_GENERATED_SOURCES}) 

target_precompile_headers(VikingRenderer PRIVATE ${VKR_PRECOMPILED_HEADER})
source_group("Generated Files" FILES ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${PROJECT_NAME}.dir/cmake_pch.cxx)
foreach(config_type ${CMAKE_CONFIGURATION_TYPES})
    source_group("Generated Files" FILES ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${PROJECT_NAME}.dir/${config_type}/cmake_pch.hxx)
endforeach()

find_package(directx-dxc CONFIG REQUIRED)
target_link_libraries(VikingRenderer PRIVATE Microsoft::DirectXShaderCompiler)

find_package(directx-headers CONFIG REQUIRED)
find_package(directx12-agility CONFIG REQUIRED)
target_link_libraries(VikingRenderer PRIVATE Microsoft::DirectX-Headers Microsoft::DirectX-Guids Microsoft::DirectX12-Agility)

find_package(directxtex CONFIG REQUIRED)
target_link_libraries(VikingRenderer PRIVATE Microsoft::DirectXTex)

find_package(winpixevent CONFIG REQUIRED)
target_link_libraries(VikingRenderer PRIVATE Microsoft::WinPixEventRuntime)

find_path(CGLTF_INCLUDE_DIRS "cgltf.h")
target_include_directories(VikingRenderer PRIVATE ${CGLTF_INCLUDE_DIRS})

find_package(imgui CONFIG REQUIRED)
target_link_libraries(VikingRenderer PRIVATE imgui::imgui)

find_package(imguizmo CONFIG REQUIRED)
target_link_libraries(VikingRenderer PRIVATE imguizmo::imguizmo)

find_package(nlohmann_json CONFIG REQUIRED)
target_link_libraries(VikingRenderer PRIVATE nlohmann_json::nlohmann_json)

find_package(EnTT CONFIG REQUIRED)
target_link_libraries(VikingRenderer PRIVATE EnTT::EnTT)

target_include_directories(VikingRenderer PRIVATE ${STREAMLINE_INCLUDE_DIR})

add_dependencies(VikingRenderer StreamlineSDK)
add_dependencies(VikingRenderer VikingHeaderTool)

target_include_directories(VikingRenderer PUBLIC ${VKR_SRC_DIR})
target_include_directories(VikingRenderer PUBLIC ${VKR_GENERATED_SRC_DIR})

if(MSVC)
	target_compile_options(VikingRenderer PRIVATE "/MP")
endif()