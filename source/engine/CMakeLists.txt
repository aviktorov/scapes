cmake_minimum_required(VERSION 3.10)
project(scapes)
set(TARGET scapes)

file(GLOB SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/render/backend/vulkan/driver.cpp
	${THIRDPARTY_DIR}/volk/volk.c
	${THIRDPARTY_DIR}/imgui/imgui.cpp
	${THIRDPARTY_DIR}/imgui/imgui_draw.cpp
	${THIRDPARTY_DIR}/imgui/imgui_widgets.cpp
	${THIRDPARTY_DIR}/imgui/examples/imgui_impl_vulkan.cpp
)

file(GLOB HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

add_library(${TARGET} STATIC ${SOURCES} ${HEADERS})

target_include_directories(${TARGET} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_definitions(${TARGET} PUBLIC NOMINMAX VK_USE_PLATFORM_WIN32_KHR PBR_SANDBOX_WIN32)
target_link_libraries(${TARGET} PUBLIC assimp-vc141-mt IrrXml zlibstatic shaderc_shared)
