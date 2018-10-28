/*
	Copyright (c) 2017-2018 ByteBit

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

#include "common.h"

#ifdef USE_GLFW

static void window_impl_mouseclick(GLFWwindow* window, int button, int action, int mods) {
    int b = 0;
    switch(button) {
        case GLFW_MOUSE_BUTTON_LEFT:
            b = WINDOW_MOUSE_LMB;
            break;
        case GLFW_MOUSE_BUTTON_RIGHT:
            b = WINDOW_MOUSE_RMB;
            break;
        case GLFW_MOUSE_BUTTON_MIDDLE:
            b = WINDOW_MOUSE_MMB;
            break;
    }
    int a = -1;
    switch(action) {
        case GLFW_RELEASE:
            a = WINDOW_RELEASE;
            break;
        case GLFW_PRESS:
            a = WINDOW_PRESS;
            break;
    }
    mouse_click(hud_window,b,a,mods>0);
}
static void window_impl_mouse(GLFWwindow* window, double x, double y) {
    mouse(hud_window,x,y);
}
static void window_impl_mousescroll(GLFWwindow* window, double xoffset, double yoffset) {
    mouse_scroll(hud_window,xoffset,yoffset);
}
static void window_impl_error(int i, const char* s) {
    on_error(i,s);
}
static void window_impl_reshape(GLFWwindow* window, int width, int height) {
    reshape(hud_window,width,height);
}
static void window_impl_textinput(GLFWwindow* window, unsigned int codepoint) {
    text_input(hud_window,codepoint);
}
static void window_impl_keys(GLFWwindow* window, int key, int scancode, int action, int mods) {
    int a = -1;
    switch(action) {
        case GLFW_RELEASE:
            a = WINDOW_RELEASE;
            break;
        case GLFW_PRESS:
            a = WINDOW_PRESS;
            break;
    }
    int tr = window_key_translate(key,0);
    if(tr>=0)
        keys(hud_window,tr,scancode,a,mods>0);
	else
		tr = WINDOW_KEY_UNKNOWN;
	if(hud_active->input_keyboard)
		hud_active->input_keyboard(tr,action,mods,key);
}

char* window_keyname(int keycode) {
	return glfwGetKeyName(keycode,0)!=NULL?(char*)glfwGetKeyName(keycode,0):"?";
}

float window_time() {
    return glfwGetTime();
}

int window_pressed_keys[64] = {0};

const char* window_clipboard() {
    return glfwGetClipboardString(hud_window->impl);
}

int window_key_translate(int key, int dir) {
    return config_key_translate(key,dir);
}

int window_key_down(int key) {
    return window_pressed_keys[key];
}

void window_mousemode(int mode) {
    int s = glfwGetInputMode(hud_window->impl,GLFW_CURSOR);
    if((s==GLFW_CURSOR_DISABLED && mode==WINDOW_CURSOR_ENABLED)
    || (s==GLFW_CURSOR_NORMAL && mode==WINDOW_CURSOR_DISABLED)) {
        glfwSetInputMode(hud_window->impl,GLFW_CURSOR,mode==WINDOW_CURSOR_ENABLED?GLFW_CURSOR_NORMAL:GLFW_CURSOR_DISABLED);
    }
}

void window_mouseloc(double* x, double* y) {
    glfwGetCursorPos(hud_window->impl,x,y);
}

void window_swapping(int value) {
    glfwSwapInterval(value);
}

void window_init() {
    static struct window_instance i;
    hud_window = &i;

    glfwWindowHint(GLFW_VISIBLE,0);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,1);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,1);

    glfwSetErrorCallback(window_impl_error);

	if(!glfwInit()) {
		log_fatal("GLFW3 init failed");
    	exit(1);
	}

	if(settings.multisamples>0) {
		glfwWindowHint(GLFW_SAMPLES,settings.multisamples);
	}

	hud_window->impl = glfwCreateWindow(settings.window_width,settings.window_height,"BetterSpades "BETTERSPADES_VERSION,settings.fullscreen?glfwGetPrimaryMonitor():NULL,NULL);
	if(!hud_window->impl) {
		log_fatal("Could not open window");
		glfwTerminate();
		exit(1);
	}

	const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
	glfwSetWindowPos(hud_window->impl,(mode->width-settings.window_width)/2.0F,(mode->height-settings.window_height)/2.0F);
	glfwShowWindow(hud_window->impl);

	glfwMakeContextCurrent(hud_window->impl);

	glfwSetFramebufferSizeCallback(hud_window->impl,window_impl_reshape);
	glfwSetCursorPosCallback(hud_window->impl,window_impl_mouse);
	glfwSetKeyCallback(hud_window->impl,window_impl_keys);
	glfwSetMouseButtonCallback(hud_window->impl,window_impl_mouseclick);
	glfwSetScrollCallback(hud_window->impl,window_impl_mousescroll);
	glfwSetCharCallback(hud_window->impl,window_impl_textinput);
}

void window_fromsettings() {
	glfwWindowHint(GLFW_SAMPLES,settings.multisamples);
	glfwSetWindowSize(hud_window->impl,settings.window_width,settings.window_height);

	if(settings.vsync<2)
		window_swapping(settings.vsync);
	if(settings.vsync>1)
		window_swapping(0);

	const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
	if(settings.fullscreen)
		glfwSetWindowMonitor(hud_window->impl,glfwGetPrimaryMonitor(),0,0,settings.window_width,settings.window_height,mode->refreshRate);
	else
		glfwSetWindowMonitor(hud_window->impl,NULL,(mode->width-settings.window_width)/2,(mode->height-settings.window_height)/2,settings.window_width,settings.window_height,0);
}

void window_deinit() {
	glfwTerminate();
}

void window_update() {
    glfwSwapBuffers(hud_window->impl);
    glfwPollEvents();
}

int window_closed() {
    return glfwWindowShouldClose(hud_window->impl);
}

#endif


#ifdef USE_SDL

void window_fromsettings() {

}

char* window_keyname(int keycode) {
	return (char*)SDL_GetKeyName(keycode);
}

float window_time() {
    return ((double)SDL_GetTicks())/1000.0F;
}

int window_pressed_keys[64] = {0};

const char* window_clipboard() {
    return SDL_HasClipboardText()?SDL_GetClipboardText():NULL;
}

int window_key_translate(int key, int dir) {
    return config_key_translate(key,dir);
}

int window_key_down(int key) {
    return window_pressed_keys[key];
}

void window_mousemode(int mode) {
	int s = SDL_GetRelativeMouseMode();
	if((s && mode==WINDOW_CURSOR_ENABLED) || (!s && mode==WINDOW_CURSOR_DISABLED)) {
		SDL_SetRelativeMouseMode(mode==WINDOW_CURSOR_ENABLED?0:1);
	}
}

void window_mouseloc(double* x, double* y) {
	int xi,yi;
	SDL_GetMouseState(&xi,&yi);
	*x = xi;
	*y = yi;
}

void window_swapping(int value) {
    SDL_GL_SetSwapInterval(value);
}

void window_init() {
	static struct window_instance i;
	hud_window = &i;

	SDL_Init(SDL_INIT_VIDEO|SDL_INIT_EVENTS|SDL_INIT_TIMER);

	hud_window->impl = SDL_CreateWindow("BetterSpades "BETTERSPADES_VERSION,SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,settings.window_width,settings.window_height,SDL_WINDOW_OPENGL);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,1);
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE,8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE,8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,8);
	//SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,16);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);
	SDL_GLContext* ctx = SDL_GL_CreateContext(hud_window->impl);
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
			case SDL_QUIT:
				quit = 1;
				break;
			case SDL_KEYDOWN:
			{
				if(event.key.keysym.sym==SDLK_p)
					exit(0);
				int tr = window_key_translate(event.key.keysym.sym,0);
				if(tr>=0)
					keys(hud_window,tr,event.key.keysym.sym,WINDOW_PRESS,0);
				else
					tr = WINDOW_KEY_UNKNOWN;
				if(hud_active->input_keyboard)
					hud_active->input_keyboard(tr,WINDOW_PRESS,0,event.key.keysym.sym);
				break;
			}
			case SDL_KEYUP:
			{
				int tr = window_key_translate(event.key.keysym.sym,0);
				if(tr>=0)
					keys(hud_window,tr,event.key.keysym.sym,WINDOW_RELEASE,0);
				else
					tr = WINDOW_KEY_UNKNOWN;
				if(hud_active->input_keyboard)
					hud_active->input_keyboard(tr,WINDOW_RELEASE,0,event.key.keysym.sym);
				break;
			}
			case SDL_MOUSEBUTTONDOWN:
			{
				int a = 0;
				switch(event.button.button) {
					case SDL_BUTTON_LEFT:
						a = WINDOW_MOUSE_LMB;
						break;
					case SDL_BUTTON_RIGHT:
						a = WINDOW_MOUSE_RMB;
						break;
					case SDL_BUTTON_MIDDLE:
						a = WINDOW_MOUSE_MMB;
						break;
				}
				mouse_click(hud_window,a,WINDOW_PRESS,0);
				break;
			}
			case SDL_MOUSEBUTTONUP:
			{
				int a = 0;
				switch(event.button.button) {
					case SDL_BUTTON_LEFT:
						a = WINDOW_MOUSE_LMB;
						break;
					case SDL_BUTTON_RIGHT:
						a = WINDOW_MOUSE_RMB;
						break;
					case SDL_BUTTON_MIDDLE:
						a = WINDOW_MOUSE_MMB;
						break;
				}
				mouse_click(hud_window,a,WINDOW_RELEASE,0);
				break;
			}
			case SDL_WINDOWEVENT:
				if(event.window.event==SDL_WINDOWEVENT_RESIZED
				|| event.window.event==SDL_WINDOWEVENT_SIZE_CHANGED) {
					reshape(hud_window,event.window.data1,event.window.data2);
				}
				break;
			case SDL_MOUSEWHEEL:
				mouse_scroll(hud_window,event.wheel.x,event.wheel.y);
				break;
			case SDL_MOUSEMOTION:
			{
				static int x_sum = 0, y_sum = 0;
				x_sum += event.motion.xrel;
				y_sum += event.motion.yrel;
				mouse(hud_window,x_sum,y_sum);
				break;
			}
			case SDL_TEXTINPUT:
				text_input(hud_window,event.text.text[0]);
				break;
		}
	}
}

int window_closed() {
    return quit;
}

#endif

int window_cpucores() {
	#ifdef OS_LINUX
		int count;
		int size = sizeof(int);
		sysctlbyname("hw.ncpu",&count,&size,NULL,0);
		return count;
	#endif
	#ifdef OS_WINDOWS
		SYSTEM_INFO info;
		GetSystemInfo(&info);
		return info.dwNumberOfProcessors;
	#endif
	return 1;
}
