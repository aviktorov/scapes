cmake_minimum_required(VERSION 3.10)
project(app)

# ==================================================================================================
# Variables
# ==================================================================================================
if (WIN32)
	set(SCAPES_PLATFORM windows)
	set(SCAPES_ABI x64)
endif()

# ==================================================================================================
# Sources
# ==================================================================================================
file(GLOB PROFILER_SOURCES
	${DIR_THIRDPARTY}/tracy/TracyClient.cpp
)

file(GLOB PROFILER_HEADERS
	${DIR_THIRDPARTY}/tracy/Tracy.hpp
)

set(PROFILER_DEFINES
	TRACY_ENABLE
)

file(GLOB SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
	${DIR_THIRDPARTY}/imgui/examples/imgui_impl_glfw.cpp
	${DIR_THIRDPARTY}/imgui/imgui.cpp
	${DIR_THIRDPARTY}/imgui/imgui_draw.cpp
	${DIR_THIRDPARTY}/imgui/imgui_widgets.cpp
)

file(GLOB HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/*.h
)

if (WIN32)
	set (DEFINES
		GLFW_EXPOSE_NATIVE_WIN32
	)
endif()

# ==================================================================================================
# Target
# ==================================================================================================
add_executable(app
	${SOURCES} ${HEADERS}
	${PROFILER_SOURCES} ${PROFILER_HEADERS}
)
set_target_properties(app PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${DIR_EXPORT}/${SCAPES_PLATFORM}/${SCAPES_ABI})
set_target_properties(app PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${DIR_EXPORT}/${SCAPES_PLATFORM}/${SCAPES_ABI})

# ==================================================================================================
# Includes
# ==================================================================================================
target_include_directories(app PUBLIC ${API_DIR})

# ==================================================================================================
# Preprocessor
# ==================================================================================================
target_compile_definitions(app PUBLIC ${DEFINES} ${PROFILER_DEFINES})

# ==================================================================================================
# Libraries
# ==================================================================================================
target_link_libraries(app PUBLIC glfw3 scapes assimp-vc141-mt IrrXml zlibstatic)

# ==================================================================================================
# Custom commands
# ==================================================================================================
add_custom_command(
	TARGET app POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy
		${DIR_THIRDPARTY}/assimp/lib/assimp-vc141-mt.dll
		${DIR_EXPORT}/${SCAPES_PLATFORM}/${SCAPES_ABI}/assimp-vc141-mt.dll
)
