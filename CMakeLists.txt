cmake_minimum_required(VERSION 3.0)

project(PBRSandbox VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
	set(CMAKE_EXE_LINKER_FLAGS_DEBUG
		"${CMAKE_EXE_LINKER_FLAGS_DEBUG} /NODEFAULTLIB:MSVCRT")
endif()

set(THIRDPARTY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty)
set(API_DIR ${CMAKE_CURRENT_SOURCE_DIR}/source/engine)

include_directories("3rdparty/assimp/include")
include_directories("3rdparty/glfw/include")
include_directories("3rdparty/glm")
include_directories("3rdparty/shaderc/include")
include_directories("3rdparty/stb")
include_directories("3rdparty/volk")
include_directories("3rdparty/imgui")
include_directories("3rdparty/imgui/examples")

link_directories("3rdparty/assimp/lib")
link_directories("3rdparty/glfw/lib")
link_directories("3rdparty/shaderc/lib")

find_package(Vulkan REQUIRED)
if (Vulkan_FOUND)
	message(STATUS "Found Vulkan, including now")
	include_directories(${Vulkan_INCLUDE_DIRS})
endif()

add_subdirectory(source/engine)
add_subdirectory(source/app)
