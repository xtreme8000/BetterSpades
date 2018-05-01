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
    keys(hud_window,window_key_translate(key,0),scancode,a,mods>0);
}

float window_time() {
    return glfwGetTime();
}

int window_pressed_keys[64] = {0};

const char* window_clipboard() {
    return glfwGetClipboardString(hud_window->impl);
}

int window_key_translate(int key, int dir) {
    int table[128] = {
                        WINDOW_KEY_UP,GLFW_KEY_W,
                        WINDOW_KEY_DOWN,GLFW_KEY_S,
                        WINDOW_KEY_LEFT,GLFW_KEY_A,
                        WINDOW_KEY_RIGHT,GLFW_KEY_D,
                        WINDOW_KEY_SPACE,GLFW_KEY_SPACE,
                        WINDOW_KEY_SPRINT,GLFW_KEY_LEFT_SHIFT,
                        WINDOW_KEY_CURSOR_UP,GLFW_KEY_UP,
                        WINDOW_KEY_CURSOR_DOWN,GLFW_KEY_DOWN,
                        WINDOW_KEY_CURSOR_LEFT,GLFW_KEY_LEFT,
                        WINDOW_KEY_CURSOR_RIGHT,GLFW_KEY_RIGHT,
                        WINDOW_KEY_BACKSPACE,GLFW_KEY_BACKSPACE,
                        WINDOW_KEY_TOOL1,GLFW_KEY_1,
                        WINDOW_KEY_TOOL2,GLFW_KEY_2,
                        WINDOW_KEY_TOOL3,GLFW_KEY_3,
                        WINDOW_KEY_TOOL4,GLFW_KEY_4,
                        WINDOW_KEY_TAB,GLFW_KEY_TAB,
                        WINDOW_KEY_ESCAPE,GLFW_KEY_ESCAPE,
                        WINDOW_KEY_MAP,GLFW_KEY_M,
                        WINDOW_KEY_CROUCH,GLFW_KEY_LEFT_CONTROL,
                        WINDOW_KEY_SNEAK,GLFW_KEY_V,
                        WINDOW_KEY_ENTER,GLFW_KEY_ENTER,
                        WINDOW_KEY_F1,GLFW_KEY_F1,
                        WINDOW_KEY_F2,GLFW_KEY_F2,
                        WINDOW_KEY_F3,GLFW_KEY_F3,
                        WINDOW_KEY_F4,GLFW_KEY_F4,
                        WINDOW_KEY_YES,GLFW_KEY_Y,
                        WINDOW_KEY_YES,GLFW_KEY_Z,
                        WINDOW_KEY_NO,GLFW_KEY_N,
                        WINDOW_KEY_VOLUME_UP,GLFW_KEY_KP_ADD,
                        WINDOW_KEY_VOLUME_DOWN,GLFW_KEY_KP_SUBTRACT,
                        WINDOW_KEY_V,GLFW_KEY_V,
                        WINDOW_KEY_RELOAD,GLFW_KEY_R,
                        WINDOW_KEY_CHAT,GLFW_KEY_T,
                        WINDOW_KEY_FULLSCREEN,GLFW_KEY_F11,
                        WINDOW_KEY_SCREENSHOT,GLFW_KEY_F5,
                        WINDOW_KEY_CHANGETEAM,GLFW_KEY_COMMA,
                        WINDOW_KEY_CHANGEWEAPON,GLFW_KEY_PERIOD,
                        WINDOW_KEY_PICKCOLOR,GLFW_KEY_E,
                    };
    if(dir) {
        for(int k=0;k<128;k+=2)
            if(table[k]==key)
                return table[k+1];
    } else {
        for(int k=0;k<128;k+=2)
            if(table[k+1]==key)
                return table[k];
    }
    return -1;
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
		printf("GLFW3 init failed\n");
    	exit(1);
	}

	if(settings.multisamples>0) {
		glfwWindowHint(GLFW_SAMPLES,settings.multisamples);
	}

	hud_window->impl = glfwCreateWindow(settings.window_width,settings.window_height,"BetterSpades "BETTERSPADES_VERSION,settings.fullscreen?glfwGetPrimaryMonitor():NULL,NULL);
	if(!hud_window->impl) {
		printf("Could not open window\n");
		glfwTerminate();
		exit(1);
	}

	const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
	glfwSetWindowPos(hud_window->impl,(mode->width-settings.window_width)/2.0F,(mode->height-settings.window_height)/2.0F);
	glfwShowWindow(hud_window->impl);

    glfwMakeContextCurrent(hud_window->impl);

	glfwMakeContextCurrent(hud_window->impl);
	glfwSetFramebufferSizeCallback(hud_window->impl,window_impl_reshape);
	glfwSetCursorPosCallback(hud_window->impl,window_impl_mouse);
	glfwSetKeyCallback(hud_window->impl,window_impl_keys);
	glfwSetMouseButtonCallback(hud_window->impl,window_impl_mouseclick);
	glfwSetScrollCallback(hud_window->impl,window_impl_mousescroll);
	glfwSetCharCallback(hud_window->impl,window_impl_textinput);
	glfwSetInputMode(hud_window->impl,GLFW_CURSOR,GLFW_CURSOR_DISABLED);
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
