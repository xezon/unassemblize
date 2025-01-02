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
#pragma once

#include "imguiapp.h"
#include <memory>

struct GLFWwindow;

namespace unassemblize::gui
{

class ImGuiGLFW
{
public:
    ImGuiGLFW();
    ~ImGuiGLFW();

    ImGuiStatus run(const CommandLineOptions& clo);

private:
    bool init();
    void shutdown();
    bool update();
    void render();
    void updateDisplayScale();

    GLFWwindow* m_window;
    std::unique_ptr<ImGuiApp> m_app;
    bool m_initialized;
    float m_dpiScale;
};

} // namespace unassemblize::gui
