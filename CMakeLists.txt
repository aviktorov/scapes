cmake_minimum_required(VERSION 3.0)

project(PBRSandbox VERSION 1.0.0)

include_directories("3rdparty/glfw/include")
include_directories("3rdparty/glm")
include_directories("3rdparty/volk")
include_directories("3rdparty/assimp/include")
include_directories("3rdparty/assimp/contrib/stb_image")
include_directories("build/3rdparty/assimp/include")

add_subdirectory("3rdparty/glfw")
add_subdirectory("3rdparty/assimp")

file(GLOB PBR_SANDBOX_SOURCES
	source/app/*.h
	source/app/*.cpp
	3rdparty/volk/volk.c
)

find_package(Vulkan REQUIRED)
if (Vulkan_FOUND)
	message(STATUS "Found Vulkan, Including and Linking now")
	include_directories(${Vulkan_INCLUDE_DIRS})
endif()

add_compile_definitions(NOMINMAX VK_USE_PLATFORM_WIN32_KHR)
add_executable(PBRSandbox ${PBR_SANDBOX_SOURCES})
target_link_libraries(PBRSandbox glfw assimp)
