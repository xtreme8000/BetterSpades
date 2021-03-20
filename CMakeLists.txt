cmake_minimum_required(VERSION 3.11)
project(BetterSpades C)

set(BETTERSPADES_MAJOR 0)
set(BETTERSPADES_MINOR 1)
set(BETTERSPADES_PATCH 4)

option(ENABLE_TOUCH "Enable touch support including ingame overlay" OFF)
option(ENABLE_ANDROID_FILE "Use SDL's file functions" OFF)
option(ENABLE_OPENGLES "Build for OpenGL ES" OFF)
option(ENABLE_SDL "Build against SDL backend" OFF)
option(ENABLE_GLFW "Build against GLFW3 backend" ON)
option(ENABLE_SOUND "Enable sound support using OpenAL" ON)
option(ENABLE_RPC "Enable Discord Rich Presence support" OFF)


if((ENABLE_ANDROID_FILE OR ENABLE_TOUCH OR ENABLE_OPENGLES) AND NOT ENABLE_SDL)
	message(FATAL_ERROR "Enable SDL to use ENABLE_ANDROID_FILE, ENABLE_TOUCH or ENABLE_OPENGLES")
endif()
if(ENABLE_SDL AND ENABLE_GLFW)
	message(FATAL_ERROR "Only one backend must be selected")
endif()
if(NOT ENABLE_SDL AND NOT ENABLE_GLFW)
	message(FATAL_ERROR "Enable SDL or GLFW")
endif()

message(STATUS "Compiler is ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "Build type is ${CMAKE_BUILD_TYPE}")

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	if(CMAKE_BUILD_TYPE STREQUAL "Release")
		add_compile_options(-Ofast)
	else()
		add_compile_options(-Ofast -g -march=native)
	endif()
endif()

add_subdirectory(src)
