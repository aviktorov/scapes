cmake_minimum_required(VERSION 3.10)

project(PBRSandbox VERSION 1.0.0)

# ==================================================================================================
# Variables
# ==================================================================================================
set(DIR_THIRDPARTY ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty)
set(DIR_EXPORT ${CMAKE_CURRENT_SOURCE_DIR}/build)
set(DIR_ENGINE_API ${CMAKE_CURRENT_SOURCE_DIR}/include)

# ==================================================================================================
# General compile flags
# ==================================================================================================
if (WIN32)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17 /Zc:__cplusplus")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D_SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D_CRT_SECURE_NO_WARNINGS")
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -fstrict-aliasing -Wno-unknown-pragmas -Wno-unused-function")
endif()

# ==================================================================================================
# General includes
# ==================================================================================================
include_directories("3rdparty/")
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
include_directories("3rdparty/flecs")

# ==================================================================================================
# General libraries
# ==================================================================================================
link_directories("3rdparty/assimp/lib")
link_directories("3rdparty/glfw/lib")
link_directories("3rdparty/shaderc/lib")
link_directories("3rdparty/spirv-cross/lib")

# ==================================================================================================
# Sub-projects
# ==================================================================================================
add_subdirectory(source/engine)
add_subdirectory(source/app)
