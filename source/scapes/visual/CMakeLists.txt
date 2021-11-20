cmake_minimum_required(VERSION 3.10)
set(TARGET visual)

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
file(GLOB COMMON_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/../common/*.cpp
)

file(GLOB COMMON_HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/../common/*.h
)

file(GLOB ROOT_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/resources/*.cpp
)

file(GLOB ROOT_HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/*.h
	${CMAKE_CURRENT_SOURCE_DIR}/resources/*.h
)

# ==================================================================================================
# Target
# ==================================================================================================
add_library(
	${TARGET} SHARED
	${COMMON_SOURCES} ${COMMON_HEADERS}
	${ROOT_SOURCES} ${ROOT_HEADERS}
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
target_include_directories(${TARGET} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../common ${CMAKE_CURRENT_SOURCE_DIR} ${DIR_API})

# ==================================================================================================
# Preprocessor
# ==================================================================================================
target_compile_definitions(${TARGET} PRIVATE SCAPES_SHARED_LIBRARY SCAPES_PROFILER_ENABLED)

# ==================================================================================================
# Libraries
# ==================================================================================================
target_link_libraries(${TARGET} PRIVATE foundation)
