cmake_minimum_required(VERSION 3.10)
project(app)
set(TARGET app)

file(GLOB SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
	${THIRDPARTY_DIR}/imgui/examples/imgui_impl_glfw.cpp
)

file(GLOB HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/*.h
)

add_executable(${TARGET} ${SOURCES} ${HEADERS})

target_compile_definitions(${TARGET} PUBLIC GLFW_EXPOSE_NATIVE_WIN32)

target_link_libraries(${TARGET} PUBLIC glfw3 scapes)

# TODO: remove this once engine get public API
target_include_directories(${TARGET} PUBLIC ${API_DIR})

add_custom_command(
	TARGET ${TARGET} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy
		${THIRDPARTY_DIR}/assimp/lib/assimp-vc141-mt.dll
		${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/assimp-vc141-mt.dll
)

add_custom_command(
	TARGET ${TARGET} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy
		${THIRDPARTY_DIR}/shaderc/lib/shaderc_shared.dll
		${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/shaderc_shared.dll
)
