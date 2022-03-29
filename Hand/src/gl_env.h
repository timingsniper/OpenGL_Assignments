#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib")

#ifdef GLEW_STATIC
#pragma comment(lib, "glew32s.lib")
#else
#pragma comment(lib, "glew32.lib")
#endif

#ifdef GLFW_STATIC
#pragma comment(lib, "glfw3.lib")
#else
#pragma comment(lib, "glfw3dll.lib")
#endif