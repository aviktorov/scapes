cmake_minimum_required (VERSION 3.0)

project (PBRSandbox VERSION 1.0.0)

include_directories("3rdparty/glfw/include")
include_directories("3rdparty/glm")
include_directories("3rdparty/assimp/include")
include_directories("3rdparty/assimp/contrib/stb_image")

add_subdirectory("3rdparty/glfw")
add_subdirectory("3rdparty/assimp")

file(GLOB PBR_SANDBOX_SOURCES
	source/app/*.h
	source/app/*.cpp
)

add_executable(PBRSandbox ${PBR_SANDBOX_SOURCES})

find_package(Vulkan REQUIRED)
if (Vulkan_FOUND)
	message(STATUS "Found Vulkan, Including and Linking now")
	include_directories(${Vulkan_INCLUDE_DIRS})
	target_link_libraries (PBRSandbox ${Vulkan_LIBRARIES})
endif()

target_link_libraries (PBRSandbox glfw)
