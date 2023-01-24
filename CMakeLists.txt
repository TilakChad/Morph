
cmake_minimum_required(VERSION 3.10)

project(Morph)
set (SRC "./src")
set (GLAD_INCLUDE "./glad/include")
set (GLFW_INCLUDE  "./GLFW/include")
set (GLFW "./GLFW/src")

set (COMMON_GLFW ${GLFW}/context.c ${GLFW}/window.c ${GLFW}/init.c ${GLFW}/input.c ${GLFW}/vulkan.c ${GLFW}/egl_context.c ${GLFW}/osmesa_context.c ${GLFW}/monitor.c)
set (WIN32_GLFW ${GLFW}/win32_thread.c ${GLFW}/win32_window.c ${GLFW}/win32_monitor.c ${GLFW}/win32_init.c ${GLFW}/win32_time.c ${GLFW}/win32_joystick.c ${GLFW}/wgl_context.c)
set (X11_GLFW  ${GLFW}/posix_thread.c  ${GLFW}/x11_window.c  ${GLFW}/x11_monitor.c ${GLFW}/x11_init.c  ${GLFW}/posix_time.c  ${GLFW}/linux_joystick.c  ${GLFW}/glx_context.c  ${GLFW}/xkb_unicode.c)

include_directories(${GLFW_INCLUDE} ${GLAD_INCLUDE})
if(WIN32)
	# ${SRC}/graph.c, removed from here for now
	add_executable(morph ./utility/bmp.c ${SRC}/main.c ${SRC}/parser.c ${SRC}/interactive.c ${SRC}/Morph.c ./glad/src/glad.c ${COMMON_GLFW} ${WIN32_GLFW} )
	target_link_libraries(morph gdi32 kernel32 user32)
	set (CMAKE_C_FLAGS "-std=c11")
	add_compile_definitions(_GLFW_WIN32)
endif (WIN32)

if (UNIX)
        add_executable(morph ./utility/bmp.c ${SRC}/main.c  ${SRC}/parser.c ${SRC}/Morph.c ${SRC}/interactive.c ./glad/src/glad.c ${COMMON_GLFW} ${X11_GLFW})
	target_link_libraries(morph pthread dl X11 m)
	set (CMAKE_C_FLAGS "-std=c11")
	add_compile_definitions(_GLFW_X11)
endif (UNIX)
