cmake_minimum_required(VERSION 3.10)

project(PBRSandbox VERSION 1.0.0)

if (WIN32)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17 /Zc:__cplusplus")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D_SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D_CRT_SECURE_NO_WARNINGS")
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -fstrict-aliasing -Wno-unknown-pragmas -Wno-unused-function")
endif()

set(THIRDPARTY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty)
set(ENGINE_API_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

include_directories("3rdparty/assimp/include")
include_directories("3rdparty/glfw/include")
include_directories("3rdparty/glm")
include_directories("3rdparty/shaderc/include")
include_directories("3rdparty/stb")
include_directories("3rdparty/volk")
include_directories("3rdparty/vma")
include_directories("3rdparty/vulkan")
include_directories("3rdparty/imgui")
include_directories("3rdparty/imgui/examples")
include_directories("3rdparty/tracy")

link_directories("3rdparty/assimp/lib")
link_directories("3rdparty/glfw/lib")
link_directories("3rdparty/shaderc/lib")
link_directories("3rdparty/spirv-cross/lib")

add_subdirectory(source/engine)
add_subdirectory(source/app)
