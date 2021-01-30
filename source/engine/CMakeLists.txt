cmake_minimum_required(VERSION 3.10)
project(scapes)

option(USE_RENDER_BACKEND_VK "Include Vulkan rendering backend" TRUE)
option(USE_RENDER_BACKEND_OPENGL "Include OpenGL rendering backend" TRUE)

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

file(GLOB RENDER_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/render/*.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/render/backend/*.cpp
)

file(GLOB RENDER_HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/render/*.h
	${CMAKE_CURRENT_SOURCE_DIR}/render/backend/*.h
)

# TODO: add platform dependent checks
set(RENDER_DEFINES
	SCAPES_PLATFORM_WIN32
	NOMINMAX
)

set(RENDER_LIBRARIES
	assimp-vc141-mt
	IrrXml
	zlibstatic
	shaderc_shared
)

if (USE_RENDER_BACKEND_OPENGL)
	file(GLOB GL_RENDER_SOURCES
		${THIRDPARTY_DIR}/glad/glad.c
		${THIRDPARTY_DIR}/glad/glad_loader.c
		${CMAKE_CURRENT_SOURCE_DIR}/render/backend/opengl/Driver.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/render/backend/opengl/Utils.cpp
	)

	file(GLOB GL_RENDER_HEADERS
		${CMAKE_CURRENT_SOURCE_DIR}/render/backend/opengl/*.h
		${THIRDPARTY_DIR}/glad/glad.h
		${THIRDPARTY_DIR}/glad/glad_loader.h
	)

	list(APPEND RENDER_SOURCES ${GL_RENDER_SOURCES})
	list(APPEND RENDER_HEADERS ${GL_RENDER_HEADERS})

	# TODO: add platform dependent checks
	list(APPEND RENDER_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/render/backend/opengl/PlatformWGL.cpp)
	list(APPEND RENDER_SOURCES ${THIRDPARTY_DIR}/glad/glad_wgl.c)
	list(APPEND RENDER_HEADERS ${THIRDPARTY_DIR}/glad/glad_wgl.h)

	list(APPEND RENDER_LIBRARIES opengl32)
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

	# TODO: add platform dependent checks
	list(APPEND RENDER_DEFINES VK_USE_PLATFORM_WIN32_KHR)
endif()

add_library(
	scapes STATIC
	${COMMON_SOURCES} ${COMMON_HEADERS}
	${RENDER_SOURCES} ${RENDER_HEADERS}
	${PROFILER_SOURCES} ${PROFILER_HEADERS}
)

target_include_directories(scapes PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(scapes PUBLIC ${RENDER_DEFINES} ${PROFILER_DEFINES})
target_link_libraries(scapes PUBLIC ${RENDER_LIBRARIES})

# TODO: add common library for all engine modules
