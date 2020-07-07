cmake_minimum_required(VERSION 3.10)
project(scapes)
set(TARGET scapes)

file(GLOB VK_BACKEND_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/render/backend/vulkan/*.cpp
)

file(GLOB VK_BACKEND_HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/render/backend/vulkan/*.h
)

file(GLOB SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/render/*.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/render/backend/driver.cpp
	${THIRDPARTY_DIR}/volk/volk.c
	${THIRDPARTY_DIR}/imgui/imgui.cpp
	${THIRDPARTY_DIR}/imgui/imgui_draw.cpp
	${THIRDPARTY_DIR}/imgui/imgui_widgets.cpp
	${THIRDPARTY_DIR}/imgui/examples/imgui_impl_vulkan.cpp
)

file(GLOB HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/*.h
)

add_library(${TARGET} STATIC ${SOURCES} ${HEADERS} ${VK_BACKEND_SOURCES} ${VK_BACKEND_HEADERS})

target_include_directories(${TARGET} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_definitions(${TARGET} PUBLIC NOMINMAX VK_USE_PLATFORM_WIN32_KHR PBR_SANDBOX_WIN32)
target_link_libraries(${TARGET} PUBLIC assimp-vc141-mt IrrXml zlibstatic shaderc_shared)
