cmake_minimum_required(VERSION 3.15 FATAL_ERROR)

file(GLOB_RECURSE VKR_GRAPHICS_SOURCES ${VKR_SRC_DIR}/graphics/*.cpp ${VKR_SRC_DIR}/graphics/*.h)
source_group(Graphics FILES ${VKR_GRAPHICS_SOURCES})

file(GLOB_RECURSE VKR_RENDER_SOURCES ${VKR_SRC_DIR}/render/*.cpp ${VKR_SRC_DIR}/render/*.h)
source_group(Render FILES ${VKR_RENDER_SOURCES})

file(GLOB_RECURSE VKR_UTILS_SOURCES ${VKR_SRC_DIR}/utils/*.cpp ${VKR_SRC_DIR}/utils/*.h)
source_group(Utils FILES ${VKR_UTILS_SOURCES})

add_library(VikingRenderer 
			${VKR_GRAPHICS_SOURCES} 
			${VKR_RENDER_SOURCES}
            ${VKR_UTILS_SOURCES}) 
