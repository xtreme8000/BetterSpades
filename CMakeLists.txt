cmake_minimum_required(VERSION 2.9)
project(client)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_compile_options(-Ofast)

#file(COPY resources/.* DESTINATION ${CMAKE_BINARY_DIR}/bin)

file(GLOB sources src/*c)
add_executable(client ${sources})

file(GLOB libs libs/*lib)
target_link_libraries(client ${libs})
file(GLOB libs2 libs/*a)
target_link_libraries(client ${libs2})

target_link_libraries(client glu32)
target_link_libraries(client gdi32)
target_link_libraries(client opengl32)
target_link_libraries(client ws2_32)
target_link_libraries(client winmm)
