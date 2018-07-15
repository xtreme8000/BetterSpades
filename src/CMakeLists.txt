macro(download_file_if_it_doesnt_exist filename url)
	if (NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${filename})
		message(STATUS "Downloading ${filename}")
		file(DOWNLOAD ${url} ${CMAKE_CURRENT_SOURCE_DIR}/${filename} STATUS stat)
		list(GET stat 0 success)
		list(GET stat 1 error)
		if (NOT ${success})
			message(STATUS "Downloaded ${filename} succesfully")
		else()
			file(REMOVE ${CMAKE_CURRENT_SOURCE_DIR}/${filename})
			message(FATAL_ERROR "Failed to download ${filename} (${error})")
		endif()
	endif()
endmacro(download_file_if_it_doesnt_exist)

download_file_if_it_doesnt_exist(ini.c https://raw.githubusercontent.com/benhoyt/inih/master/ini.c)
download_file_if_it_doesnt_exist(ini.h https://raw.githubusercontent.com/benhoyt/inih/master/ini.h)
download_file_if_it_doesnt_exist(lodepng/lodepng.c https://raw.githubusercontent.com/lvandeve/lodepng/master/lodepng.cpp)
download_file_if_it_doesnt_exist(lodepng/lodepng.h https://raw.githubusercontent.com/lvandeve/lodepng/master/lodepng.h)
download_file_if_it_doesnt_exist(dr_wav.c https://raw.githubusercontent.com/mackron/dr_libs/master/dr_wav.h)
download_file_if_it_doesnt_exist(stb_truetype.h https://raw.githubusercontent.com/nothings/stb/master/stb_truetype.h)
download_file_if_it_doesnt_exist(parson.c https://raw.githubusercontent.com/kgabis/parson/master/parson.c)
download_file_if_it_doesnt_exist(parson.h https://raw.githubusercontent.com/kgabis/parson/master/parson.h)
download_file_if_it_doesnt_exist(http.h https://raw.githubusercontent.com/mattiasgustavsson/libs/master/http.h)

list(APPEND CLIENT_SOURCES aabb.c)
list(APPEND CLIENT_SOURCES camera.c)
list(APPEND CLIENT_SOURCES cameracontroller.c)
list(APPEND CLIENT_SOURCES chunk.c)
list(APPEND CLIENT_SOURCES config.c)
list(APPEND CLIENT_SOURCES dr_wav.c)
list(APPEND CLIENT_SOURCES file.c)
list(APPEND CLIENT_SOURCES font.c)
list(APPEND CLIENT_SOURCES glx.c)
list(APPEND CLIENT_SOURCES grenade.c)
list(APPEND CLIENT_SOURCES http.c)
list(APPEND CLIENT_SOURCES hud.c)
list(APPEND CLIENT_SOURCES ini.c)
list(APPEND CLIENT_SOURCES list.c)
list(APPEND CLIENT_SOURCES main.c)
list(APPEND CLIENT_SOURCES map.c)
list(APPEND CLIENT_SOURCES matrix.c)
list(APPEND CLIENT_SOURCES model.c)
list(APPEND CLIENT_SOURCES network.c)
list(APPEND CLIENT_SOURCES parson.c)
list(APPEND CLIENT_SOURCES particle.c)
list(APPEND CLIENT_SOURCES player.c)
list(APPEND CLIENT_SOURCES sound.c)
list(APPEND CLIENT_SOURCES stb_truetype.c)
list(APPEND CLIENT_SOURCES texture.c)
list(APPEND CLIENT_SOURCES tracer.c)
list(APPEND CLIENT_SOURCES weapon.c)
list(APPEND CLIENT_SOURCES window.c)
list(APPEND CLIENT_SOURCES ${BetterSpades_SOURCE_DIR}/resources/icon.rc)

add_executable(client ${CLIENT_SOURCES})
target_compile_definitions(client PRIVATE DR_WAV_IMPLEMENTATION)
set_target_properties(
	client PROPERTIES 
	RUNTIME_OUTPUT_DIRECTORY ${BetterSpades_SOURCE_DIR}/build/BetterSpades
	C_STANDARD 99
)

set(CMAKE_PREFIX_PATH  ${CMAKE_PREFIX_PATH} ${BetterSpades_SOURCE_DIR}/deps)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${BetterSpades_SOURCE_DIR}/cmake/Modules)
find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(OpenAL CONFIG REQUIRED)
find_package(enet REQUIRED)
find_package(deflate REQUIRED)

target_link_libraries(client glfw OpenGL::GL OpenAL::OpenAL enet::enet deflate::deflate)

add_custom_command(
	TARGET client
	POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E echo "Copying resources.."
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${BetterSpades_SOURCE_DIR}/resources ${BetterSpades_SOURCE_DIR}/build/BetterSpades
)
