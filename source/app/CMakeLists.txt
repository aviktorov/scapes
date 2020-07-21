cmake_minimum_required(VERSION 3.10)
project(app)

file(GLOB SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
	${THIRDPARTY_DIR}/imgui/examples/imgui_impl_glfw.cpp
	${THIRDPARTY_DIR}/imgui/imgui.cpp
	${THIRDPARTY_DIR}/imgui/imgui_draw.cpp
	${THIRDPARTY_DIR}/imgui/imgui_widgets.cpp
)

file(GLOB HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/*.h
)

set (DEFINES
	SCAPES_PLATFORM_WIN32
)

add_executable(app ${SOURCES} ${HEADERS})

target_include_directories(app PUBLIC ${API_DIR})
target_compile_definitions(app PUBLIC ${DEFINES})
target_link_libraries(app PUBLIC glfw3 scapes)

add_custom_command(
	TARGET app POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy
		${THIRDPARTY_DIR}/assimp/lib/assimp-vc141-mt.dll
		${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/assimp-vc141-mt.dll
)

add_custom_command(
	TARGET app POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy
		${THIRDPARTY_DIR}/shaderc/lib/shaderc_shared.dll
		${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/shaderc_shared.dll
)
