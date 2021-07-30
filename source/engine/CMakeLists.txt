cmake_minimum_required(VERSION 3.10)
project(scapes)

# ==================================================================================================
# Options
# ==================================================================================================
option(USE_RENDER_BACKEND_VK "Include Vulkan rendering backend" TRUE)

# ==================================================================================================
# Variables
# ==================================================================================================
if (WIN32)
	set(SCAPES_PLATFORM windows)
	set(SCAPES_ABI x64)
endif()

# ==================================================================================================
# Dependencies
# ==================================================================================================
add_library(shaderc STATIC IMPORTED)

if (WIN32)
	set_target_properties(shaderc PROPERTIES IMPORTED_LOCATION_RELEASE ${DIR_THIRDPARTY}/shaderc/lib/${SCAPES_PLATFORM}/${SCAPES_ABI}/shaderc_combined.lib)
	set_target_properties(shaderc PROPERTIES IMPORTED_LOCATION_MINSIZEREL ${DIR_THIRDPARTY}/shaderc/lib/${SCAPES_PLATFORM}/${SCAPES_ABI}/shaderc_combined.lib)
	set_target_properties(shaderc PROPERTIES IMPORTED_LOCATION_RELWITHDEBINFO ${DIR_THIRDPARTY}/shaderc/lib/${SCAPES_PLATFORM}/${SCAPES_ABI}/shaderc_combined.lib)
	set_target_properties(shaderc PROPERTIES IMPORTED_LOCATION_DEBUG ${DIR_THIRDPARTY}/shaderc/lib/${SCAPES_PLATFORM}/${SCAPES_ABI}/shaderc_combinedd.lib)
endif()

# ==================================================================================================
# Sources
# ==================================================================================================
file(GLOB COMMON_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/common/*.cpp
)

file(GLOB HEADER_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/common/*.h
)

file(GLOB GAME_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/game/*.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/game/flecs/*.cpp
	${DIR_THIRDPARTY}/flecs/flecs.c
)

file(GLOB GAME_HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/game/*.h
	${CMAKE_CURRENT_SOURCE_DIR}/game/flecs/*.h
	${DIR_THIRDPARTY}/flecs/flecs.h
)

file(GLOB RENDER_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/render/*.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/render/shaders/*.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/render/shaders/spirv/*.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/render/backend/*.cpp
)

file(GLOB RENDER_HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/render/*.h
	${CMAKE_CURRENT_SOURCE_DIR}/render/shaders/*.h
	${CMAKE_CURRENT_SOURCE_DIR}/render/shaders/spirv/*.h
	${CMAKE_CURRENT_SOURCE_DIR}/render/backend/*.h
)

if (WIN32)
	set(PLATFORM_DEFINES
		SCAPES_PLATFORM_WIN32
		NOMINMAX
	)
endif()

if (USE_RENDER_BACKEND_VK)
	file(GLOB VK_RENDER_SOURCES
		${DIR_THIRDPARTY}/volk/volk.c
		${CMAKE_CURRENT_SOURCE_DIR}/render/backend/vulkan/*.cpp
	)

	file(GLOB VK_RENDER_HEADERS
		${DIR_THIRDPARTY}/volk/volk.h
		${CMAKE_CURRENT_SOURCE_DIR}/render/backend/vulkan/*.h
	)

	list(APPEND RENDER_SOURCES ${VK_RENDER_SOURCES})
	list(APPEND RENDER_HEADERS ${VK_RENDER_HEADERS})

	if (WIN32)
		list(APPEND PLATFORM_DEFINES VK_USE_PLATFORM_WIN32_KHR)
	endif()

	list(APPEND RENDER_DEBUG_DEFINES SCAPES_VULKAN_USE_VALIDATION_LAYERS)
endif()

# ==================================================================================================
# Target
# ==================================================================================================
add_library(
	scapes SHARED
	${COMMON_SOURCES} ${COMMON_HEADERS}
	${GAME_SOURCES} ${GAME_HEADERS}
	${RENDER_SOURCES} ${RENDER_HEADERS}
)
set_target_properties(scapes PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${DIR_EXPORT}/${SCAPES_PLATFORM}/${SCAPES_ABI})
set_target_properties(scapes PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${DIR_EXPORT}/${SCAPES_PLATFORM}/${SCAPES_ABI})

# ==================================================================================================
# Includes
# ==================================================================================================
target_include_directories(scapes PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${DIR_ENGINE_API})

# ==================================================================================================
# Preprocessor
# ==================================================================================================
target_compile_definitions(scapes PUBLIC SCAPES_SHARED_LIBRARY ${PLATFORM_DEFINES} ${RENDER_DEFINES})
target_compile_definitions(scapes PUBLIC $<$<CONFIG:Debug>:${RENDER_DEBUG_DEFINES}>)

# ==================================================================================================
# Libraries
# ==================================================================================================
target_link_libraries(scapes PUBLIC shaderc)
