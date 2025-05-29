cmake_minimum_required(VERSION 3.15 FATAL_ERROR)

file(GLOB_RECURSE VKR_GRAPHICS_SOURCES ${VKR_SRC_DIR}/graphics/*.cpp ${VKR_SRC_DIR}/graphics/*.h)
source_group(Graphics FILES ${VKR_GRAPHICS_SOURCES})

file(GLOB_RECURSE VKR_SHADER_SOURCES ${VKR_SRC_DIR}/../data/shaders/*.hlsl)
source_group(Shaders FILES ${VKR_SHADER_SOURCES})
set_source_files_properties(${VKR_SHADER_SOURCES} PROPERTIES HEADER_FILE_ONLY 1)

file(GLOB_RECURSE VKR_RENDER_SOURCES ${VKR_SRC_DIR}/render/*.cpp ${VKR_SRC_DIR}/render/*.h)
source_group(Render FILES ${VKR_RENDER_SOURCES})

file(GLOB_RECURSE VKR_UTILS_SOURCES ${VKR_SRC_DIR}/utils/*.cpp ${VKR_SRC_DIR}/utils/*.h)
source_group(Utils FILES ${VKR_UTILS_SOURCES})

add_library(VikingRenderer 
			${VKR_GRAPHICS_SOURCES}
			${VKR_SHADER_SOURCES} 
			${VKR_RENDER_SOURCES}
            		${VKR_UTILS_SOURCES}) 

find_package(directx-dxc CONFIG REQUIRED)
target_link_libraries(VikingRenderer PRIVATE Microsoft::DirectXShaderCompiler)

find_package(directx12-agility CONFIG REQUIRED)
target_link_libraries(VikingRenderer PRIVATE Microsoft::DirectX12-Agility)

find_package(directxtex CONFIG REQUIRED)
target_link_libraries(VikingRenderer PRIVATE Microsoft::DirectXTex)

find_path(CGLTF_INCLUDE_DIRS "cgltf.h")
target_include_directories(VikingRenderer PRIVATE ${CGLTF_INCLUDE_DIRS})

target_include_directories(VikingRenderer PUBLIC ${VKR_SRC_DIR})

if(MSVC)
	target_compile_options(VikingRenderer PRIVATE "/MP")
endif()