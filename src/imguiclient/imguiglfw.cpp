/**
 * @file
 *
 * @brief ImGui shell for GLFW (Linux/macOS)
 *
 * @copyright Unassemblize is free software: you can redistribute it and/or
 *            modify it under the terms of the GNU General Public License
 *            as published by the Free Software Foundation, either version
 *            3 of the License, or (at your option) any later version.
 *            A full copy of the GNU General Public License can be found in
 *            LICENSE
 */

#ifdef __APPLE__
#define GLFW_INCLUDE_NONE // Prevent GLFW from including gl.h
#include <OpenGL/gl3.h> // Include only gl3.h on macOS
#else
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#endif

#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "imguiglfw.h"
#include "version.h"
#include <GLFW/glfw3.h>
#include <imgui.h>

namespace unassemblize::gui
{

static void glfw_error_callback(int error, const char *description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

ImGuiGLFW::ImGuiGLFW() : m_window(nullptr), m_app(nullptr), m_initialized(false), m_dpiScale(1.0f)
{
}

ImGuiGLFW::~ImGuiGLFW()
{
    shutdown();
}

bool ImGuiGLFW::init()
{
    if (m_initialized)
        return true;

#ifndef __APPLE__
    // Force software rendering on Linux to avoid MESA/ZINK warnings
    setenv("LIBGL_ALWAYS_SOFTWARE", "1", 1);
    // Disable hardware acceleration
    setenv("MESA_GL_VERSION_OVERRIDE", "3.0", 1);
#endif

    // Initialize GLFW
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
    {
        return false;
    }

    // Set OpenGL version and profile
#ifdef __APPLE__
    const char *glsl_version = "#version 150";

    // Core Profile is required for macOS
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    // Basic window hints
    glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_TRUE);
#else
    const char *glsl_version = "#version 130";

    // Try modern OpenGL first
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_NATIVE_CONTEXT_API);
#endif

    m_window = glfwCreateWindow(1280, 800, create_version_string().c_str(), nullptr, nullptr);
    if (m_window == nullptr)
    {
        // If modern OpenGL failed, try a more basic configuration
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_FALSE);

        m_window = glfwCreateWindow(1280, 800, create_version_string().c_str(), nullptr, nullptr);
        if (m_window == nullptr)
        {
            glfwTerminate();
            return false;
        }
    }

    glfwMakeContextCurrent(m_window);
    if (!glfwGetCurrentContext())
    {
        glfwDestroyWindow(m_window);
        glfwTerminate();
        return false;
    }

    // Basic setup
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    if (!ImGui::GetCurrentContext())
    {
        glfwDestroyWindow(m_window);
        glfwTerminate();
        return false;
    }

    // Configure ImGui
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

#ifdef __APPLE__
    io.ConfigMacOSXBehaviors = true;
#endif

    if (!ImGui_ImplGlfw_InitForOpenGL(m_window, true))
    {
        ImGui::DestroyContext();
        glfwDestroyWindow(m_window);
        glfwTerminate();
        return false;
    }

    if (!ImGui_ImplOpenGL3_Init(glsl_version))
    {
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        glfwDestroyWindow(m_window);
        glfwTerminate();
        return false;
    }

    ImGui::StyleColorsDark();

    // Initial DPI scale
    float xscale = 1.0f, yscale = 1.0f;
    glfwGetWindowContentScale(m_window, &xscale, &yscale);
    m_dpiScale = (xscale > yscale) ? xscale : yscale;

    m_initialized = true;
    return true;
}

void ImGuiGLFW::shutdown()
{
    if (!m_initialized)
        return;

    // Cleanup ImGui app
    m_app.reset();

    // Cleanup ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // Cleanup GLFW
    if (m_window)
    {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }
    glfwTerminate();
    m_initialized = false;
}

bool ImGuiGLFW::update()
{
    if (!m_initialized || !m_window)
        return false;

#ifdef __APPLE__
    glfwWaitEventsTimeout(0.016);
#else
    glfwPollEvents();
#endif

    if (glfwWindowShouldClose(m_window))
        return false;

    if (glfwGetWindowAttrib(m_window, GLFW_ICONIFIED))
    {
        glfwWaitEventsTimeout(0.1);
        return true;
    }

    // Get window info
    int window_x, window_y;
    glfwGetWindowPos(m_window, &window_x, &window_y);
    int width, height;
    glfwGetFramebufferSize(m_window, &width, &height);
    m_app->set_window_pos(ImVec2(static_cast<float>(window_x), static_cast<float>(window_y)));
    m_app->set_window_size(ImVec2(static_cast<float>(width), static_cast<float>(height)));

    // Start backend frames
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();

    // Let app handle ImGui frame
    ImGuiStatus status = m_app->update();
    if (status != ImGuiStatus::Ok)
    {
        glfwSetWindowShouldClose(m_window, GLFW_TRUE);
        return false;
    }

    // Render
    ImGui::Render();

    // Update platform windows before rendering
    ImGuiIO &io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        GLFWwindow *backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }

    // Complete render
    render();

    return true;
}

void ImGuiGLFW::render()
{
    if (!m_initialized || !m_window)
        return;

    int display_w, display_h;
    glfwGetFramebufferSize(m_window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);

    ImVec4 clear_color = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(m_window);
}

void ImGuiGLFW::updateDisplayScale()
{
    if (!m_window)
    {
        return;
    }

    if (!m_initialized)
    {
        return;
    }

    if (!glfwGetWindowAttrib(m_window, GLFW_VISIBLE))
    {
        return;
    }

    // Get new DPI scale
    float xscale = 1.0f, yscale = 1.0f;
    glfwGetWindowContentScale(m_window, &xscale, &yscale);
    float newScale = (xscale > yscale) ? xscale : yscale;

    // Only update if scale changed
    if (newScale != m_dpiScale)
    {
        m_dpiScale = newScale;

        // Update style
        ImGui::GetStyle().ScaleAllSizes(m_dpiScale);

        // Update fonts
        try
        {
            ImGuiIO &io = ImGui::GetIO();
            io.Fonts->Clear();
            ImFontConfig font_cfg;
            font_cfg.SizePixels = 13.0f * m_dpiScale;
            io.Fonts->AddFontDefault(&font_cfg);
            bool built = io.Fonts->Build();
        }
        catch (const std::exception &e)
        {
            fprintf(stderr, "Exception during font update: %s\n", e.what());
        }
        catch (...)
        {
            fprintf(stderr, "Unknown exception during font update\n");
        }
    }
}

ImGuiStatus ImGuiGLFW::run(const CommandLineOptions &clo)
{
    if (!init())
    {
        fprintf(stderr, "Failed to initialize ImGui GLFW backend\n");
        return ImGuiStatus::Error;
    }

    // Initialize the app after ImGui is fully set up
    m_app = std::make_unique<ImGuiApp>();

    if (m_app->init(clo) != ImGuiStatus::Ok)
    {
        return ImGuiStatus::Error;
    }

    // Main loop
    while (!glfwWindowShouldClose(m_window))
    {
        if (!update())
        {
            break;
        }
    }

    shutdown();
    return ImGuiStatus::Ok;
}

} // namespace unassemblize::gui
