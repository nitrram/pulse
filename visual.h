#pragma once

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <GLFW/glfw3.h>

void key_callback(
	GLFWwindow */*window*/,
	int /*key*/,
	int /*scancode*/,
	int /*action*/,
	int /*mods*/
);

void setup_gl(
	char */*zero_argv*/
);

void setPixels(
	unsigned char* /*pixels*/
);

void releaseVisual();

void draw();

bool wflgRead();

void wflgPoll();

void wflgInit();
