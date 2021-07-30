cmake_minimum_required(VERSION 3.10)
project(scapes)

option(USE_RENDER_BACKEND_VK "Include Vulkan rendering backend" TRUE)

add_library(shaderc STATIC IMPORTED)

if (WIN32)
	set_target_properties(shaderc PROPERTIES IMPORTED_LOCATION_RELEASE ${THIRDPARTY_DIR}/shaderc/lib/windows/x64/shaderc_combined.lib)
	set_target_properties(shaderc PROPERTIES IMPORTED_LOCATION_MINSIZEREL ${THIRDPARTY_DIR}/shaderc/lib/windows/x64/shaderc_combined.lib)
	set_target_properties(shaderc PROPERTIES IMPORTED_LOCATION_RELWITHDEBINFO ${THIRDPARTY_DIR}/shaderc/lib/windows/x64/shaderc_combined.lib)
	set_target_properties(shaderc PROPERTIES IMPORTED_LOCATION_DEBUG ${THIRDPARTY_DIR}/shaderc/lib/windows/x64/shaderc_combinedd.lib)
endif()

file(GLOB PROFILER_SOURCES
	${THIRDPARTY_DIR}/tracy/TracyClient.cpp
)

file(GLOB PROFILER_HEADERS
	${THIRDPARTY_DIR}/tracy/Tracy.hpp
)

set(PROFILER_DEFINES
	TRACY_ENABLE
)

file(GLOB COMMON_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/common/*.cpp
)

file(GLOB HEADER_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/common/*.h
)

file(GLOB GAME_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/game/*.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/game/flecs/*.cpp
	${THIRDPARTY_DIR}/flecs/flecs.c
)

file(GLOB GAME_HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/game/*.h
	${CMAKE_CURRENT_SOURCE_DIR}/game/flecs/*.h
	${THIRDPARTY_DIR}/flecs/flecs.h
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
		${THIRDPARTY_DIR}/volk/volk.c
		${CMAKE_CURRENT_SOURCE_DIR}/render/backend/vulkan/*.cpp
	)

	file(GLOB VK_RENDER_HEADERS
		${THIRDPARTY_DIR}/volk/volk.h
		${CMAKE_CURRENT_SOURCE_DIR}/render/backend/vulkan/*.h
	)

	list(APPEND RENDER_SOURCES ${VK_RENDER_SOURCES})
	list(APPEND RENDER_HEADERS ${VK_RENDER_HEADERS})

	if (WIN32)
		list(APPEND PLATFORM_DEFINES VK_USE_PLATFORM_WIN32_KHR)
	endif()

	list(APPEND RENDER_DEBUG_DEFINES SCAPES_VULKAN_USE_VALIDATION_LAYERS)
endif()

add_library(
	scapes STATIC
	${COMMON_SOURCES} ${COMMON_HEADERS}
	${GAME_SOURCES} ${GAME_HEADERS}
	${RENDER_SOURCES} ${RENDER_HEADERS}
	${PROFILER_SOURCES} ${PROFILER_HEADERS}
)

target_include_directories(scapes PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${ENGINE_API_DIR})
target_compile_definitions(scapes PUBLIC ${PLATFORM_DEFINES} ${RENDER_DEFINES} ${PROFILER_DEFINES})
target_compile_definitions(scapes PUBLIC $<$<CONFIG:Debug>:${RENDER_DEBUG_DEFINES}>)
target_link_libraries(scapes PUBLIC shaderc)
