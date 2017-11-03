cmake_minimum_required(VERSION 2.9)
project(client)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  add_compile_options(-Ofast -g)
endif()

file(GLOB resources resources/*)
file(COPY ${resources} DESTINATION ${CMAKE_BINARY_DIR}/bin)

file(GLOB sources src/*c)
add_executable(client ${sources})

file(GLOB libs libs/*lib)
target_link_libraries(client ${libs})
file(GLOB libs2 libs/*a)
target_link_libraries(client ${libs2})

if(WIN32)
	target_link_libraries(client gdi32)
	target_link_libraries(client opengl32)
	target_link_libraries(client ws2_32)
	target_link_libraries(client winmm)
else()
	target_link_libraries(client GL)
	target_link_libraries(client m)
	target_link_libraries(client openal)
	target_link_libraries(client glfw)
	target_link_libraries(client enet)
endif()