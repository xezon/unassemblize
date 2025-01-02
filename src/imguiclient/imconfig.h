#pragma once

// Include OpenGL headers before GLFW
#ifdef __APPLE__
    #define GL_SILENCE_DEPRECATION
    #define GL_DO_NOT_WARN_IF_MULTI_GL_VERSION_HEADERS_INCLUDED
    #include <OpenGL/gl3.h>
#else
    #include <GL/gl3.h>
#endif

// Force ImGui to use our GLFW 3.3.9 headers
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

// Default ImGui configuration
// #define IMGUI_ENABLE_FREETYPE  // Disabled until we set up FreeType properly
#define IMGUI_ENABLE_VIEWPORT
#define IMGUI_ENABLE_DOCKING
