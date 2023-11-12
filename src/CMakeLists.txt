include(FetchContent)

FetchContent_Declare(
	cglm
	GIT_REPOSITORY	https://github.com/recp/cglm
	GIT_TAG			v0.8.5
)

set(CGLM_STATIC ON CACHE INTERNAL "")
set(CGLM_USE_C99 ON CACHE INTERNAL "")
FetchContent_MakeAvailable(cglm)

FetchContent_Declare(
	vxl
	GIT_REPOSITORY	https://github.com/xtreme8000/libvxl
	GIT_TAG			origin/master
)

FetchContent_MakeAvailable(vxl)

macro(download_file_if_it_doesnt_exist filename url)
	if (NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${filename})
		message(STATUS "Downloading ${filename}")
		file(DOWNLOAD ${url} ${CMAKE_CURRENT_SOURCE_DIR}/${filename} STATUS stat)
		list(GET stat 0 success)
		list(GET stat 1 error)
		if (NOT ${success})
			message(STATUS "Downloaded ${filename} successfully")
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
download_file_if_it_doesnt_exist(dr_wav.c https://raw.githubusercontent.com/mackron/dr_libs/14707e8ee5aec7620781bcc3b484b8b1607aba50/dr_wav.h)
download_file_if_it_doesnt_exist(stb_truetype.h https://raw.githubusercontent.com/nothings/stb/master/stb_truetype.h)
download_file_if_it_doesnt_exist(parson.c https://raw.githubusercontent.com/kgabis/parson/master/parson.c)
download_file_if_it_doesnt_exist(parson.h https://raw.githubusercontent.com/kgabis/parson/master/parson.h)
download_file_if_it_doesnt_exist(http.h https://raw.githubusercontent.com/mattiasgustavsson/libs/main/http.h)
download_file_if_it_doesnt_exist(log.h https://raw.githubusercontent.com/xtreme8000/log.c/master/src/log.h)
download_file_if_it_doesnt_exist(log.c https://raw.githubusercontent.com/xtreme8000/log.c/master/src/log.c)
download_file_if_it_doesnt_exist(hashtable.c https://raw.githubusercontent.com/goldsborough/hashtable/master/hashtable.c)
download_file_if_it_doesnt_exist(hashtable.h https://raw.githubusercontent.com/goldsborough/hashtable/master/hashtable.h)
download_file_if_it_doesnt_exist(../deps/libdeflate.h https://raw.githubusercontent.com/ebiggers/libdeflate/master/libdeflate.h)
download_file_if_it_doesnt_exist(../bsresources.zip http://aos.party/bsresources.zip)
download_file_if_it_doesnt_exist(microui.c https://raw.githubusercontent.com/rxi/microui/master/src/microui.c)
download_file_if_it_doesnt_exist(microui.h https://raw.githubusercontent.com/rxi/microui/master/src/microui.h)

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
list(APPEND CLIENT_SOURCES utils.c)
list(APPEND CLIENT_SOURCES ping.c)
list(APPEND CLIENT_SOURCES log.c)
list(APPEND CLIENT_SOURCES minheap.c)
list(APPEND CLIENT_SOURCES hashtable.c)
list(APPEND CLIENT_SOURCES rpc.c)
list(APPEND CLIENT_SOURCES tesselator.c)
list(APPEND CLIENT_SOURCES microui.c)
list(APPEND CLIENT_SOURCES channel.c)
list(APPEND CLIENT_SOURCES entitysystem.c)
list(APPEND CLIENT_SOURCES ${BetterSpades_SOURCE_DIR}/resources/icon.rc)

add_executable(client ${CLIENT_SOURCES})

if(CMAKE_BUILD_TYPE STREQUAL "Release")
	set_property(TARGET client PROPERTY WIN32_EXECUTABLE true)
	target_compile_definitions(client PRIVATE NDEBUG)
endif()

set(GIT_COMMIT_HASH "?")

execute_process(
  COMMAND git log -1 --format=%h
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_COMMIT_HASH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

include(CheckIPOSupported)
check_ipo_supported(RESULT lto_supported OUTPUT error)

if(lto_supported)
	message(STATUS "IPO / LTO enabled")
	set_property(TARGET client PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
else()
	message(STATUS "IPO / LTO not supported: <${error}>")
endif()

target_compile_definitions(client PRIVATE GIT_COMMIT_HASH="${GIT_COMMIT_HASH}")
target_compile_definitions(client PRIVATE DR_WAV_IMPLEMENTATION)
target_compile_definitions(client PRIVATE LOG_USE_COLOR)

if(ENABLE_TOUCH)
	target_compile_definitions(client PRIVATE USE_TOUCH)
endif()
if(ENABLE_ANDROID_FILE)
	target_compile_definitions(client PRIVATE USE_ANDROID_FILE)
endif()

target_compile_definitions(client PRIVATE BETTERSPADES_MAJOR=${BETTERSPADES_MAJOR})
target_compile_definitions(client PRIVATE BETTERSPADES_MINOR=${BETTERSPADES_MINOR})
target_compile_definitions(client PRIVATE BETTERSPADES_PATCH=${BETTERSPADES_PATCH})
target_compile_definitions(client PRIVATE BETTERSPADES_VERSION="v${BETTERSPADES_MAJOR}.${BETTERSPADES_MINOR}.${BETTERSPADES_PATCH}")


set_target_properties(
	client PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ${BetterSpades_SOURCE_DIR}/build/BetterSpades
	C_STANDARD 99
)

set(CMAKE_PREFIX_PATH  ${CMAKE_PREFIX_PATH} ${BetterSpades_SOURCE_DIR}/deps)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${BetterSpades_SOURCE_DIR}/cmake/Modules)
set(OpenGL_GL_PREFERENCE LEGACY)
if(ENABLE_GLFW)
	find_package(glfw3 REQUIRED)
	target_link_libraries(client glfw3::glfw3)
	target_compile_definitions(client PRIVATE USE_GLFW)
endif()
if(ENABLE_SDL)
	find_package(SDL2 REQUIRED)
	target_link_libraries(client ${SDL2_LIBRARIES})
	target_compile_definitions(client PRIVATE USE_SDL)
endif()
if(ENABLE_SOUND)
	find_package(OpenAL REQUIRED)
	target_link_libraries(client ${OPENAL_LIBRARY})
	target_compile_definitions(client PRIVATE USE_SOUND)
endif()
find_package(enet REQUIRED)
find_package(deflate REQUIRED)
find_package(Threads REQUIRED)
if(NOT ENABLE_OPENGLES)
	find_package(OpenGL REQUIRED)
	find_package(GLEW REQUIRED)
	target_link_libraries(client GLEW::GLEW)
else()
	find_package(OpenGL REQUIRED COMPONENTS EGL)
	target_link_libraries(client OpenGL::EGL)
	target_compile_definitions(client PRIVATE OPENGL_ES)
endif()
if(ENABLE_RPC)
	target_link_libraries(client discord-rpc stdc++)
	target_compile_definitions(client PRIVATE USE_RPC DISCORD_DISABLE_IO_THREAD)
endif()

if(WIN32)
	target_compile_definitions(client PRIVATE LIBDEFLATE_STATIC)
endif()

target_link_libraries(client ${CMAKE_THREAD_LIBS_INIT} ${OPENGL_LIBRARIES} enet::enet deflate::deflate m cglm vxl)
target_include_directories(client PRIVATE ${OPENAL_INCLUDE_DIRS} ${OPENGL_INCLUDE_DIR} ${OPENGL_EGL_INCLUDE_DIRS})

add_custom_command(
	TARGET client
	POST_BUILD
	COMMENT "Copying resources..."
	COMMAND ${CMAKE_COMMAND} -E copy_directory ${BetterSpades_SOURCE_DIR}/resources ${BetterSpades_SOURCE_DIR}/build/BetterSpades
)

add_custom_command(
	TARGET client
	POST_BUILD
	COMMENT "Extracting additional third-party assets..."
	DEPENDS ${BetterSpades_SOURCE_DIR}/bsresources.zip
	WORKING_DIRECTORY ${BetterSpades_SOURCE_DIR}/build/BetterSpades
	COMMAND ${CMAKE_COMMAND} -E tar xzf ${BetterSpades_SOURCE_DIR}/bsresources.zip
)

install(
	TARGETS client
	RUNTIME
	DESTINATION .
)

install(
	FILES "${CMAKE_BINARY_DIR}/BetterSpades/config.ini" "${CMAKE_BINARY_DIR}/BetterSpades/OpenAL32.dll"
	DESTINATION .
	PERMISSIONS	OWNER_EXECUTE OWNER_WRITE OWNER_READ
				GROUP_EXECUTE GROUP_READ
)

install(
	DIRECTORY "${CMAKE_BINARY_DIR}/BetterSpades/fonts"
	DESTINATION .
	FILES_MATCHING PATTERN "*.ttf"
)

install(
	DIRECTORY "${CMAKE_BINARY_DIR}/BetterSpades/kv6"
	DESTINATION .
	FILES_MATCHING PATTERN "*.kv6"
)

install(
	DIRECTORY "${CMAKE_BINARY_DIR}/BetterSpades/png"
	DESTINATION .
	FILES_MATCHING PATTERN "*.png"
)

install(
	DIRECTORY "${CMAKE_BINARY_DIR}/BetterSpades/wav"
	DESTINATION .
	FILES_MATCHING PATTERN "*.wav"
)

install(
	DIRECTORY
	DESTINATION "cache"
)

install(
	DIRECTORY
	DESTINATION "logs"
)

install(
	DIRECTORY
	DESTINATION "screenshots"
)

set(CPACK_GENERATOR NSIS)
set(CPACK_PACKAGE_NAME "PixSpades")
set(CPACK_PACKAGE_VENDOR "github.com")
set(CPACK_PACKAGE_VERSION "v${BETTERSPADES_MAJOR}.${BETTERSPADES_MINOR}.${BETTERSPADES_PATCH}")
set(CPACK_PACKAGE_VERSION_MAJOR "${BETTERSPADES_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${BETTERSPADES_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${BETTERSPADES_PATCH}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "${CPACK_PACKAGE_NAME}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_STRIP_FILES TRUE)
set(CPACK_NSIS_MENU_LINKS "client" "PixSpades")
set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
set(CPACK_PACKAGE_CHECKSUM SHA256)

INCLUDE(CPack)
