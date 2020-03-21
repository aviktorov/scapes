cmake_minimum_required(VERSION 3.10)
project(app)
set(TARGET app)

file(GLOB SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
	${THIRDPARTY_DIR}/imgui/examples/imgui_impl_glfw.cpp
)
file(GLOB HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

add_executable(${TARGET} ${SOURCES} ${HEADERS})

target_compile_definitions(${TARGET} PUBLIC GLFW_EXPOSE_NATIVE_WIN32)

target_link_libraries(${TARGET} PUBLIC glfw3 scapes)

# TODO: remove this once engine get public API
target_include_directories(${TARGET} PUBLIC ${API_DIR})
