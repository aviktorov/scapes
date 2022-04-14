cmake_minimum_required(VERSION 3.10)
set(TARGET foundation)

project(${TARGET})

# ==================================================================================================
# Options
# ==================================================================================================

# ==================================================================================================
# Variables
# ==================================================================================================

# ==================================================================================================
# Dependencies
# ==================================================================================================

# ==================================================================================================
# Sources
# ==================================================================================================
file(GLOB PROFILER_HEADERS
	${DIR_API}/scapes/3rdparty/tracy/Tracy.hpp
)

file(GLOB PROFILER_SOURCES
	${DIR_API}/scapes/3rdparty/tracy/TracyClient.cpp
)

set(PROFILER_DEFINES
	TRACY_ENABLE TRACY_EXPORTS
)

file(GLOB COMMON_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/../common/*.cpp
)

file(GLOB COMMON_HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/../common/*.h
)

file(GLOB ROOT_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

file(GLOB ROOT_HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/*.h
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

file (GLOB RESOURCES_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/resources/*.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/resources/impl/*.cpp
)

file (GLOB RESOURCES_HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/resources/*.h
	${CMAKE_CURRENT_SOURCE_DIR}/resources/impl/*.h
)

file (GLOB SERDE_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/serde/*.cpp
)

file (GLOB SERDE_HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/serde/*.h
)

set(SERDE_DEFINES
	RYML_SHARED RYML_EXPORTS
	C4CORE_SHARED C4CORE_EXPORTS
)

# ==================================================================================================
# Target
# ==================================================================================================
add_library(
	${TARGET} SHARED
	${COMMON_SOURCES} ${COMMON_HEADERS}
	${ROOT_SOURCES} ${ROOT_HEADERS}
	${GAME_SOURCES} ${GAME_HEADERS}
	${RESOURCES_SOURCES} ${RESOURCES_HEADERS}
	${SERDE_SOURCES} ${SERDE_HEADERS}
	${PROFILER_SOURCES} ${PROFILER_HEADERS}
)

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
target_include_directories(${TARGET} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../common/ ${CMAKE_CURRENT_SOURCE_DIR} ${DIR_API})

# ==================================================================================================
# Preprocessor
# ==================================================================================================
target_compile_definitions(${TARGET} PRIVATE SCAPES_SHARED_LIBRARY SCAPES_PROFILER_ENABLED ${PLATFORM_DEFINES} ${PROFILER_DEFINES} ${SERDE_DEFINES})

# ==================================================================================================
# Libraries
# ==================================================================================================
target_link_libraries(${TARGET} PUBLIC)
