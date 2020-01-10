cmake_minimum_required(VERSION 3.15)
project(Poker)

set(CMAKE_CXX_STANDARD 20)

include_directories(src)
include_directories(src/glfw/include)
include_directories(src/imgui)
include_directories(src/imgui/examples)

add_subdirectory(src/glfw)

find_package(OpenGL)

add_executable(Poker
	${CMAKE_CURRENT_SOURCE_DIR}/src/Entry/Main.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/Profiler/Timer.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/OS/Windows/file.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/IA/Genome.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/IA/Network.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/IA/Population.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/Experiments.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/dyn_struct.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/Poker.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/Extensions/imgui_ext.cpp



	${CMAKE_CURRENT_SOURCE_DIR}/src/imgui/imgui.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/imgui/imgui_demo.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/imgui/imgui_draw.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/imgui/imgui_widgets.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/imgui/examples/imgui_impl_glfw.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/imgui/examples/imgui_impl_opengl2.cpp
)
target_link_libraries(Poker glfw)
target_link_libraries(Poker ${OPENGL_gl_LIBRARY})
