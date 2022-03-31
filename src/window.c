/*
	Copyright (c) 2017-2020 ByteBit

	This file is part of BetterSpades.

	BetterSpades is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	BetterSpades is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with BetterSpades.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "main.h"
#include "window.h"
#include "config.h"
#include "hud.h"

#ifdef OS_WINDOWS
#include <sysinfoapi.h>
#include <windows.h>
#endif

#ifdef OS_LINUX
#include <unistd.h>
#endif

#ifdef OS_HAIKU
#include <kernel/OS.h>
#endif

#ifdef USE_GLFW

static bool joystick_available = false;
static int joystick_id;
static float joystick_mouse[2] = {0, 0};
static GLFWgamepadstate joystick_state;

static void window_impl_joystick(int jid, int event) {
	if(event == GLFW_CONNECTED) {
		joystick_available = true;
		joystick_id = jid;
		log_info("Joystick detected: %s", glfwGetJoystickName(joystick_id));
	} else if(event == GLFW_DISCONNECTED) {
		joystick_available = false;
		log_info("Joystick removed: %s", glfwGetJoystickName(joystick_id));
	}
}

void window_textinput(int allow) { }

void window_setmouseloc(double x, double y) { }

static void window_impl_mouseclick(GLFWwindow* window, int button, int action, int mods) {
	int b = 0;
	switch(button) {
		case GLFW_MOUSE_BUTTON_LEFT: b = WINDOW_MOUSE_LMB; break;
		case GLFW_MOUSE_BUTTON_RIGHT: b = WINDOW_MOUSE_RMB; break;
		case GLFW_MOUSE_BUTTON_MIDDLE: b = WINDOW_MOUSE_MMB; break;
	}

	int a = -1;
	switch(action) {
		case GLFW_RELEASE: a = WINDOW_RELEASE; break;
		case GLFW_PRESS: a = WINDOW_PRESS; break;
	}

	if(a >= 0)
		mouse_click(hud_window, b, a, mods & GLFW_MOD_CONTROL);
}
static void window_impl_mouse(GLFWwindow* window, double x, double y) {
	if(!joystick_available)
		mouse(hud_window, x, y);
}
static void window_impl_mousescroll(GLFWwindow* window, double xoffset, double yoffset) {
	mouse_scroll(hud_window, xoffset, yoffset);
}
static void window_impl_error(int i, const char* s) {
	on_error(i, s);
}
static void window_impl_reshape(GLFWwindow* window, int width, int height) {
	reshape(hud_window, width, height);
}
static void window_impl_textinput(GLFWwindow* window, unsigned int codepoint) {
	text_input(hud_window, codepoint);
}
static void window_impl_keys(GLFWwindow* window, int key, int scancode, int action, int mods) {
	int count = config_key_translate(key, 0, NULL);

	int a = -1;
	switch(action) {
		case GLFW_RELEASE: a = WINDOW_RELEASE; break;
		case GLFW_PRESS: a = WINDOW_PRESS; break;
		case GLFW_REPEAT: a = WINDOW_REPEAT; break;
	}

	if(count > 0) {
		int results[count];
		config_key_translate(key, 0, results);

		for(int k = 0; k < count; k++) {
			keys(hud_window, results[k], scancode, a, mods & GLFW_MOD_CONTROL);

			if(hud_active->input_keyboard)
				hud_active->input_keyboard(results[k], action, mods & GLFW_MOD_CONTROL, key);
		}
	} else {
		if(hud_active->input_keyboard)
			hud_active->input_keyboard(WINDOW_KEY_UNKNOWN, action, mods & GLFW_MOD_CONTROL, key);
	}
}

void window_keyname(int keycode, char* output, size_t length) {
#ifdef OS_WINDOWS
	GetKeyNameTextA(glfwGetKeyScancode(keycode) << 16, output, length);
#else
	const char* name = glfwGetKeyName(keycode, 0);

	if(name) {
		strncpy(output, name, length);
		output[length - 1] = 0;
	} else {
		if(length >= 2)
			strcpy(output, "?");
	}
#endif
}

float window_time() {
	return glfwGetTime();
}

int window_pressed_keys[64] = {0};

const char* window_clipboard() {
	return glfwGetClipboardString(hud_window->impl);
}

int window_key_down(int key) {
	return window_pressed_keys[key];
}

void window_mousemode(int mode) {
	int s = glfwGetInputMode(hud_window->impl, GLFW_CURSOR);
	if((s == GLFW_CURSOR_DISABLED && mode == WINDOW_CURSOR_ENABLED)
	   || (s == GLFW_CURSOR_NORMAL && mode == WINDOW_CURSOR_DISABLED))
		glfwSetInputMode(hud_window->impl, GLFW_CURSOR,
						 mode == WINDOW_CURSOR_ENABLED ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
}

void window_mouseloc(double* x, double* y) {
	glfwGetCursorPos(hud_window->impl, x, y);
}

void window_swapping(int value) {
	glfwSwapInterval(value);
}

void window_title(char* suffix) {
	if(suffix) {
		char title[128];
		snprintf(title, sizeof(title) - 1, "BetterSpades %s - %s", BETTERSPADES_VERSION, suffix);
		glfwSetWindowTitle(hud_window->impl, title);
	} else {
		glfwSetWindowTitle(hud_window->impl, "BetterSpades " BETTERSPADES_VERSION);
	}
}

void window_init() {
	static struct window_instance i;
	hud_window = &i;

	glfwWindowHint(GLFW_VISIBLE, 0);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 1);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
#ifdef OPENGL_ES
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
	glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
#endif

	glfwSetErrorCallback(window_impl_error);

	if(!glfwInit()) {
		log_fatal("GLFW3 init failed");
		exit(1);
	}

	glfwSetJoystickCallback(window_impl_joystick);

	if(settings.multisamples > 0) {
		glfwWindowHint(GLFW_SAMPLES, settings.multisamples);
	}

	/*
	#FIXME: This is intended to fix the issue #145.
	This is dirty because it disables the application-level Hi-DPI support for every installation
	instead of being applied only to those who needs it.
	*/
	glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_FALSE);

	hud_window->impl
		= glfwCreateWindow(settings.window_width, settings.window_height, "BetterSpades " BETTERSPADES_VERSION,
						   settings.fullscreen ? glfwGetPrimaryMonitor() : NULL, NULL);
	if(!hud_window->impl) {
		log_fatal("Could not open window");
		glfwTerminate();
		exit(1);
	}

	const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
	glfwSetWindowPos(hud_window->impl, (mode->width - settings.window_width) / 2.0F,
					 (mode->height - settings.window_height) / 2.0F);
	glfwShowWindow(hud_window->impl);

	glfwMakeContextCurrent(hud_window->impl);

	glfwSetFramebufferSizeCallback(hud_window->impl, window_impl_reshape);
	glfwSetCursorPosCallback(hud_window->impl, window_impl_mouse);
	glfwSetKeyCallback(hud_window->impl, window_impl_keys);
	glfwSetMouseButtonCallback(hud_window->impl, window_impl_mouseclick);
	glfwSetScrollCallback(hud_window->impl, window_impl_mousescroll);
	glfwSetCharCallback(hud_window->impl, window_impl_textinput);

	if(glfwRawMouseMotionSupported())
		glfwSetInputMode(hud_window->impl, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
}

void window_fromsettings() {
	glfwWindowHint(GLFW_SAMPLES, settings.multisamples);
	glfwSetWindowSize(hud_window->impl, settings.window_width, settings.window_height);

	if(settings.vsync < 2)
		window_swapping(settings.vsync);
	if(settings.vsync > 1)
		window_swapping(0);

	const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
	if(settings.fullscreen)
		glfwSetWindowMonitor(hud_window->impl, glfwGetPrimaryMonitor(), 0, 0, settings.window_width,
							 settings.window_height, mode->refreshRate);
	else
		glfwSetWindowMonitor(hud_window->impl, NULL, (mode->width - settings.window_width) / 2,
							 (mode->height - settings.window_height) / 2, settings.window_width, settings.window_height,
							 0);
}

void window_deinit() {
	glfwTerminate();
}

static void gamepad_translate_key(GLFWgamepadstate* state, GLFWgamepadstate* old, int gamepad, enum window_keys key) {
	if(!old->buttons[gamepad] && state->buttons[gamepad]) {
		keys(hud_window, key, 0, WINDOW_PRESS, 0);

		if(hud_active->input_keyboard)
			hud_active->input_keyboard(key, WINDOW_PRESS, 0, 0);
	} else if(old->buttons[gamepad] && !state->buttons[gamepad]) {
		keys(hud_window, key, 0, WINDOW_RELEASE, 0);

		if(hud_active->input_keyboard)
			hud_active->input_keyboard(key, WINDOW_RELEASE, 0, 0);
	}
}

static void gamepad_translate_button(GLFWgamepadstate* state, GLFWgamepadstate* old, int gamepad,
									 enum window_buttons button) {
	if(!old->buttons[gamepad] && state->buttons[gamepad]) {
		mouse_click(hud_window, button, WINDOW_PRESS, 0);
	} else if(old->buttons[gamepad] && !state->buttons[gamepad]) {
		mouse_click(hud_window, button, WINDOW_RELEASE, 0);
	}
}

void window_update() {
	glfwSwapBuffers(hud_window->impl);
	glfwPollEvents();

	if(joystick_available && glfwJoystickIsGamepad(joystick_id)) {
		GLFWgamepadstate state;

		if(glfwGetGamepadState(joystick_id, &state)) {
			gamepad_translate_key(&state, &joystick_state, GLFW_GAMEPAD_BUTTON_DPAD_UP, WINDOW_KEY_TOOL1);
			gamepad_translate_key(&state, &joystick_state, GLFW_GAMEPAD_BUTTON_DPAD_DOWN, WINDOW_KEY_TOOL3);
			gamepad_translate_key(&state, &joystick_state, GLFW_GAMEPAD_BUTTON_DPAD_LEFT, WINDOW_KEY_TOOL4);
			gamepad_translate_key(&state, &joystick_state, GLFW_GAMEPAD_BUTTON_DPAD_RIGHT, WINDOW_KEY_TOOL2);

			gamepad_translate_key(&state, &joystick_state, GLFW_GAMEPAD_BUTTON_START, WINDOW_KEY_ESCAPE);
			gamepad_translate_key(&state, &joystick_state, GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER, WINDOW_KEY_SPACE);
			gamepad_translate_key(&state, &joystick_state, GLFW_GAMEPAD_BUTTON_LEFT_THUMB, WINDOW_KEY_CROUCH);
			gamepad_translate_key(&state, &joystick_state, GLFW_GAMEPAD_BUTTON_LEFT_BUMPER, WINDOW_KEY_SPRINT);
			gamepad_translate_key(&state, &joystick_state, GLFW_GAMEPAD_BUTTON_X, WINDOW_KEY_RELOAD);

			window_pressed_keys[WINDOW_KEY_UP] = state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y] < -0.25F;
			window_pressed_keys[WINDOW_KEY_DOWN] = state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y] > 0.25F;
			window_pressed_keys[WINDOW_KEY_LEFT] = state.axes[GLFW_GAMEPAD_AXIS_LEFT_X] < -0.25F;
			window_pressed_keys[WINDOW_KEY_RIGHT] = state.axes[GLFW_GAMEPAD_AXIS_LEFT_X] > 0.25F;

			joystick_mouse[0] += state.axes[GLFW_GAMEPAD_AXIS_RIGHT_X] * 15.0F;
			joystick_mouse[1] += state.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y] * 15.0F;
			mouse(hud_window, joystick_mouse[0], joystick_mouse[1]);

			gamepad_translate_button(&state, &joystick_state, GLFW_GAMEPAD_BUTTON_A, WINDOW_MOUSE_LMB);
			gamepad_translate_button(&state, &joystick_state, GLFW_GAMEPAD_BUTTON_B, WINDOW_MOUSE_RMB);
		}

		joystick_state = state;
	}
}

int window_closed() {
	return glfwWindowShouldClose(hud_window->impl);
}

#endif

#ifdef USE_SDL

void window_textinput(int allow) {
	if(allow && !SDL_IsTextInputActive())
		SDL_StartTextInput();
	if(!allow && SDL_IsTextInputActive())
		SDL_StopTextInput();
}

void window_fromsettings() {
	SDL_SetWindowSize(hud_window->impl, settings.window_width, settings.window_height);

	if(settings.vsync < 2)
		window_swapping(settings.vsync);
	if(settings.vsync > 1)
		window_swapping(0);

	if(settings.fullscreen)
		SDL_SetWindowFullscreen(hud_window->impl, SDL_WINDOW_FULLSCREEN);
	else
		SDL_SetWindowFullscreen(hud_window->impl, 0);
}

void window_keyname(int keycode, char* output, size_t length) {
	strncpy(output, SDL_GetKeyName(keycode), length);
	output[length - 1] = 0;
}

float window_time() {
	return ((double)SDL_GetTicks()) / 1000.0F;
}

int window_pressed_keys[64] = {0};

const char* window_clipboard() {
	return SDL_HasClipboardText() ? SDL_GetClipboardText() : NULL;
}

int window_key_down(int key) {
	return window_pressed_keys[key];
}

void window_mousemode(int mode) {
	int s = SDL_GetRelativeMouseMode();
	if((s && mode == WINDOW_CURSOR_ENABLED) || (!s && mode == WINDOW_CURSOR_DISABLED))
		SDL_SetRelativeMouseMode(mode == WINDOW_CURSOR_ENABLED ? 0 : 1);
}

static double mx = -1, my = -1;

void window_setmouseloc(double x, double y) {
	mx = x;
	my = y;
}

void window_mouseloc(double* x, double* y) {
	if(mx < 0 && my < 0) {
		int xi, yi;
		SDL_GetMouseState(&xi, &yi);
		*x = xi;
		*y = yi;
	} else {
		*x = mx;
		*y = my;
	}
}

void window_swapping(int value) {
	SDL_GL_SetSwapInterval(value);
}

static struct window_finger fingers[8];

void window_init() {
	static struct window_instance i;
	hud_window = &i;

#ifdef USE_TOUCH
	SDL_SetHintWithPriority(SDL_HINT_ANDROID_SEPARATE_MOUSE_AND_TOUCH, "1", SDL_HINT_OVERRIDE);
#endif

	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER);

	hud_window->impl
		= SDL_CreateWindow("BetterSpades " BETTERSPADES_VERSION, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
						   settings.window_width, settings.window_height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
#ifdef OPENGL_ES
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#endif
	SDL_GLContext* ctx = SDL_GL_CreateContext(hud_window->impl);

	memset(fingers, 0, sizeof(fingers));
}

void window_deinit() {
	SDL_DestroyWindow(hud_window->impl);
	SDL_Quit();
}

static int quit = 0;
void window_update() {
	SDL_GL_SwapWindow(hud_window->impl);
	SDL_Event event;
	while(SDL_PollEvent(&event)) {
		switch(event.type) {
			case SDL_QUIT: quit = 1; break;
			case SDL_KEYDOWN: {
				int count = config_key_translate(event.key.keysym.sym, 0, NULL);

				if(count > 0) {
					int results[count];
					config_key_translate(event.key.keysym.sym, 0, results);

					for(int k = 0; k < count; k++) {
						keys(hud_window, results[k], event.key.keysym.sym, WINDOW_PRESS,
							 event.key.keysym.mod & KMOD_CTRL);

						if(hud_active->input_keyboard)
							hud_active->input_keyboard(results[k], WINDOW_PRESS, event.key.keysym.mod & KMOD_CTRL,
													   event.key.keysym.sym);
					}
				} else {
					if(hud_active->input_keyboard)
						hud_active->input_keyboard(WINDOW_KEY_UNKNOWN, WINDOW_PRESS, event.key.keysym.mod & KMOD_CTRL,
												   event.key.keysym.sym);
				}
				break;
			}
			case SDL_KEYUP: {
				int count = config_key_translate(event.key.keysym.sym, 0, NULL);

				if(count > 0) {
					int results[count];
					config_key_translate(event.key.keysym.sym, 0, results);

					for(int k = 0; k < count; k++) {
						keys(hud_window, results[k], event.key.keysym.sym, WINDOW_RELEASE,
							 event.key.keysym.mod & KMOD_CTRL);

						if(hud_active->input_keyboard)
							hud_active->input_keyboard(results[k], WINDOW_RELEASE, event.key.keysym.mod & KMOD_CTRL,
													   event.key.keysym.sym);
					}
				} else {
					if(hud_active->input_keyboard)
						hud_active->input_keyboard(WINDOW_KEY_UNKNOWN, WINDOW_RELEASE, event.key.keysym.mod & KMOD_CTRL,
												   event.key.keysym.sym);
				}
				break;
			}
			case SDL_MOUSEBUTTONDOWN: {
				int a = 0;
				switch(event.button.button) {
					case SDL_BUTTON_LEFT: a = WINDOW_MOUSE_LMB; break;
					case SDL_BUTTON_RIGHT: a = WINDOW_MOUSE_RMB; break;
					case SDL_BUTTON_MIDDLE: a = WINDOW_MOUSE_MMB; break;
				}
				mouse_click(hud_window, a, WINDOW_PRESS, 0);
				break;
			}
			case SDL_MOUSEBUTTONUP: {
				int a = 0;
				switch(event.button.button) {
					case SDL_BUTTON_LEFT: a = WINDOW_MOUSE_LMB; break;
					case SDL_BUTTON_RIGHT: a = WINDOW_MOUSE_RMB; break;
					case SDL_BUTTON_MIDDLE: a = WINDOW_MOUSE_MMB; break;
				}
				mouse_click(hud_window, a, WINDOW_RELEASE, 0);
				break;
			}
			case SDL_WINDOWEVENT:
				if(event.window.event == SDL_WINDOWEVENT_RESIZED
				   || event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
					reshape(hud_window, event.window.data1, event.window.data2);
				}
				break;
			case SDL_MOUSEWHEEL: mouse_scroll(hud_window, event.wheel.x, event.wheel.y); break;
			case SDL_MOUSEMOTION: {
				if(SDL_GetRelativeMouseMode()) {
					static int x, y;
					x += event.motion.xrel;
					y += event.motion.yrel;
					mouse(hud_window, x, y);
				} else {
					mouse(hud_window, event.motion.x, event.motion.y);
				}
				break;
			}
			case SDL_TEXTINPUT: text_input(hud_window, event.text.text[0]); break;
			case SDL_FINGERDOWN:
				if(hud_active->input_touch) {
					struct window_finger* f;
					for(int k = 0; k < 8; k++) {
						if(!fingers[k].full) {
							fingers[k].finger = event.tfinger.fingerId;
							fingers[k].start.x = event.tfinger.x * settings.window_width;
							fingers[k].start.y = event.tfinger.y * settings.window_height;
							fingers[k].down_time = window_time();
							fingers[k].full = 1;
							f = fingers + k;
							break;
						}
					}

					hud_active->input_touch(f, TOUCH_DOWN, event.tfinger.x * settings.window_width,
											event.tfinger.y * settings.window_height,
											event.tfinger.dx * settings.window_width,
											event.tfinger.dy * settings.window_height);
				}
				break;
			case SDL_FINGERUP:
				if(hud_active->input_touch) {
					struct window_finger* f;
					for(int k = 0; k < 8; k++) {
						if(fingers[k].full && fingers[k].finger == event.tfinger.fingerId) {
							fingers[k].full = 0;
							f = fingers + k;
							break;
						}
					}
					hud_active->input_touch(
						f, TOUCH_UP, event.tfinger.x * settings.window_width, event.tfinger.y * settings.window_height,
						event.tfinger.dx * settings.window_width, event.tfinger.dy * settings.window_height);
				}
				break;
			case SDL_FINGERMOTION:
				if(hud_active->input_touch) {
					struct window_finger* f;
					for(int k = 0; k < 8; k++) {
						if(fingers[k].full && fingers[k].finger == event.tfinger.fingerId) {
							f = fingers + k;
							break;
						}
					}
					hud_active->input_touch(f, TOUCH_MOVE, event.tfinger.x * settings.window_width,
											event.tfinger.y * settings.window_height,
											event.tfinger.dx * settings.window_width,
											event.tfinger.dy * settings.window_height);
				}
				break;
		}
	}
}

int window_closed() {
	return quit;
}

void window_title(char* suffix) {
	if(suffix) {
		char title[128];
		snprintf(title, sizeof(title) - 1, "BetterSpades %s - %s", BETTERSPADES_VERSION, suffix);
		SDL_SetWindowTitle(hud_window->impl, title);
	} else {
		SDL_SetWindowTitle(hud_window->impl, "BetterSpades " BETTERSPADES_VERSION);
	}
}

#endif

int window_cpucores() {
#ifdef OS_LINUX
#ifdef USE_TOUCH
	return sysconf(_SC_NPROCESSORS_CONF);
#else
	return get_nprocs();
#endif
#endif
#ifdef OS_WINDOWS
	SYSTEM_INFO info;
	GetSystemInfo(&info);
	return info.dwNumberOfProcessors;
#endif
#ifdef OS_HAIKU
	system_info info;
	get_system_info(&info);
	return info.cpu_count;
#endif
return 1;
}
