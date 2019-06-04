cmake_minimum_required (VERSION 2.6)

project (PBRSandbox)

include_directories("3rdparty/glfw/include")
include_directories("3rdparty/glm")
include_directories("3rdparty/assimp/include")

add_subdirectory("3rdparty/glfw")
add_subdirectory("3rdparty/assimp")

add_executable(PBRSandbox source/app/main.cpp)

find_package(Vulkan REQUIRED)
if (Vulkan_FOUND)
	message(STATUS "Found Vulkan, Including and Linking now")
	include_directories(${Vulkan_INCLUDE_DIRS})
	target_link_libraries (PBRSandbox ${Vulkan_LIBRARIES})
endif()

target_link_libraries (PBRSandbox glfw)
