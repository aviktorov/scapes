cmake_minimum_required(VERSION 3.0)

project(PBRSandbox VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories("3rdparty/assimp/include")
include_directories("3rdparty/glfw/include")
include_directories("3rdparty/glm")
include_directories("3rdparty/shaderc/include")
include_directories("3rdparty/stb")
include_directories("3rdparty/volk")

link_directories("3rdparty/assimp/lib")
link_directories("3rdparty/glfw/lib")
link_directories("3rdparty/shaderc/lib")

find_package(Vulkan REQUIRED)
if (Vulkan_FOUND)
	message(STATUS "Found Vulkan, including now")
	include_directories(${Vulkan_INCLUDE_DIRS})
endif()

file(GLOB PBR_SANDBOX_SOURCES
	source/app/*.h
	source/app/*.cpp
	source/engine/*.h
	source/engine/*.cpp
	3rdparty/volk/volk.c
)

add_compile_definitions(NOMINMAX VK_USE_PLATFORM_WIN32_KHR)
add_executable(PBRSandbox ${PBR_SANDBOX_SOURCES})
target_link_libraries(PBRSandbox glfw3 assimp-vc141-mt IrrXml zlibstatic shaderc_shared)

add_custom_command(
	TARGET PBRSandbox POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy
		${CMAKE_SOURCE_DIR}/3rdparty/assimp/lib/assimp-vc141-mt.dll
		${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/assimp-vc141-mt.dll
)

add_custom_command(
	TARGET PBRSandbox POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy
		${CMAKE_SOURCE_DIR}/3rdparty/shaderc/lib/shaderc_shared.dll
		${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/shaderc_shared.dll
)
