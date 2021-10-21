cmake_minimum_required(VERSION 3.10)
set(TARGET app)

project(${TARGET})

# ==================================================================================================
# Variables
# ==================================================================================================

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
add_executable(${TARGET}
	${SOURCES} ${HEADERS}
	${PROFILER_SOURCES} ${PROFILER_HEADERS}
)
set_target_properties(${TARGET} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${DIR_EXPORT}/${SCAPES_PLATFORM}/${SCAPES_ABI})
set_target_properties(${TARGET} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${DIR_EXPORT}/${SCAPES_PLATFORM}/${SCAPES_ABI})
set_target_properties(${TARGET} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${DIR_EXPORT}/${SCAPES_PLATFORM}/${SCAPES_ABI})
set_target_properties(${TARGET} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${DIR_EXPORT}/${SCAPES_PLATFORM}/${SCAPES_ABI})

set_target_properties(${TARGET} PROPERTIES DEBUG_POSTFIX d)

# ==================================================================================================
# Includes
# ==================================================================================================
target_include_directories(${TARGET} PUBLIC ${API_DIR})

# ==================================================================================================
# Preprocessor
# ==================================================================================================
target_compile_definitions(${TARGET} PUBLIC ${DEFINES} ${PROFILER_DEFINES})

# ==================================================================================================
# Libraries
# ==================================================================================================
target_link_libraries(${TARGET} PUBLIC glfw3 foundation visual assimp-vc141-mt IrrXml zlibstatic)

# ==================================================================================================
# Custom commands
# ==================================================================================================
add_custom_command(
	TARGET ${TARGET} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy
		${DIR_THIRDPARTY}/assimp/lib/assimp-vc141-mt.dll
		${DIR_EXPORT}/${SCAPES_PLATFORM}/${SCAPES_ABI}/assimp-vc141-mt.dll
)
