cmake_minimum_required(VERSION 3.10)
set(TARGET visual)

project(${TARGET})

# ==================================================================================================
# Options
# ==================================================================================================
option(USE_RENDER_BACKEND_VK "Include Vulkan rendering backend" TRUE)

# ==================================================================================================
# Variables
# ==================================================================================================

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
	${CMAKE_CURRENT_SOURCE_DIR}/../common/*.cpp
)

file(GLOB COMMON_HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/../common/*.h
)

file(GLOB ROOT_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/impl/*.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/resources/*.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/utils/*.cpp
)

file(GLOB ROOT_HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/*.h
	${CMAKE_CURRENT_SOURCE_DIR}/impl/*.h
	${CMAKE_CURRENT_SOURCE_DIR}/resources/*.h
	${CMAKE_CURRENT_SOURCE_DIR}/utils/*.h
)

file(GLOB RENDER_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/hardware/*.cpp
)

file(GLOB RENDER_HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/hardware/*.h
)

file (GLOB RESOURCES_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/resources/*.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/impl/*.cpp
)

file (GLOB RESOURCES_HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/resources/*.h
	${CMAKE_CURRENT_SOURCE_DIR}/impl/*.h
)

file (GLOB SHADERS_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/shaders/spirv/*.cpp
)

file(GLOB SHADERS_HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.h
	${CMAKE_CURRENT_SOURCE_DIR}/shaders/spirv/*.h
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
		${CMAKE_CURRENT_SOURCE_DIR}/hardware/vulkan/*.cpp
	)

	file(GLOB VK_RENDER_HEADERS
		${DIR_THIRDPARTY}/volk/volk.h
		${CMAKE_CURRENT_SOURCE_DIR}/hardware/vulkan/*.h
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
	${TARGET} SHARED
	${COMMON_SOURCES} ${COMMON_HEADERS}
	${RENDER_SOURCES} ${RENDER_HEADERS}
	${ROOT_SOURCES} ${ROOT_HEADERS}
	${SHADERS_SOURCES} ${SHADERS_HEADERS}
)

set_target_properties(${TARGET} PROPERTIES LINK_FLAGS "/ignore:4099")

set_target_properties(${TARGET} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${DIR_EXPORT}/${SCAPES_PLATFORM}/${SCAPES_ABI})
set_target_properties(${TARGET} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${DIR_EXPORT}/${SCAPES_PLATFORM}/${SCAPES_ABI})
set_target_properties(${TARGET} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${DIR_EXPORT}/${SCAPES_PLATFORM}/${SCAPES_ABI})
set_target_properties(${TARGET} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${DIR_EXPORT}/${SCAPES_PLATFORM}/${SCAPES_ABI})

set_target_properties(${TARGET} PROPERTIES LIBRARY_OUTPUT_DIRECTORY_RELEASE ${DIR_EXPORT}/${SCAPES_PLATFORM}/${SCAPES_ABI})
set_target_properties(${TARGET} PROPERTIES LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL ${DIR_EXPORT}/${SCAPES_PLATFORM}/${SCAPES_ABI})
set_target_properties(${TARGET} PROPERTIES LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO ${DIR_EXPORT}/${SCAPES_PLATFORM}/${SCAPES_ABI})
set_target_properties(${TARGET} PROPERTIES LIBRARY_OUTPUT_DIRECTORY_DEBUG ${DIR_EXPORT}/${SCAPES_PLATFORM}/${SCAPES_ABI})

set_target_properties(${TARGET} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${DIR_EXPORT}/${SCAPES_PLATFORM}/${SCAPES_ABI})
set_target_properties(${TARGET} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_MINSIZEREL ${DIR_EXPORT}/${SCAPES_PLATFORM}/${SCAPES_ABI})
set_target_properties(${TARGET} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO ${DIR_EXPORT}/${SCAPES_PLATFORM}/${SCAPES_ABI})
set_target_properties(${TARGET} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${DIR_EXPORT}/${SCAPES_PLATFORM}/${SCAPES_ABI})

set_target_properties(${TARGET} PROPERTIES DEBUG_POSTFIX d)

# ==================================================================================================
# Includes
# ==================================================================================================
target_include_directories(${TARGET} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../common ${CMAKE_CURRENT_SOURCE_DIR} ${DIR_API})

# ==================================================================================================
# Preprocessor
# ==================================================================================================
target_compile_definitions(${TARGET} PRIVATE SCAPES_SHARED_LIBRARY SCAPES_PROFILER_ENABLED ${PLATFORM_DEFINES} ${RENDER_DEFINES})
target_compile_definitions(${TARGET} PRIVATE $<$<CONFIG:Debug>:${RENDER_DEBUG_DEFINES}>)

# ==================================================================================================
# Libraries
# ==================================================================================================
target_link_libraries(${TARGET} PRIVATE foundation shaderc)
