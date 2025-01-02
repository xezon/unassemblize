set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/imgui)

FetchContent_Populate(imgui DOWNLOAD_EXTRACT_TIMESTAMP
    URL https://github.com/ocornut/imgui/archive/refs/tags/v1.91.4-docking.zip
    SOURCE_DIR ${IMGUI_DIR}
)

# Common ImGui sources for all platforms
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/misc/cpp/imgui_stdlib.cpp
)

set(IMGUI_HEADERS
    ${IMGUI_DIR}/imconfig.h
    ${IMGUI_DIR}/imgui.h
    ${IMGUI_DIR}/imgui_internal.h
    ${IMGUI_DIR}/misc/cpp/imgui_stdlib.h
)

if(WINDOWS)
    add_library(imgui_win32 STATIC ${IMGUI_SOURCES}
        ${IMGUI_DIR}/backends/imgui_impl_dx9.cpp
        ${IMGUI_DIR}/backends/imgui_impl_win32.cpp
    )

    if (CMAKE_GENERATOR MATCHES "Visual Studio")
        target_sources(imgui_win32 PUBLIC
            ${IMGUI_DIR}/misc/debuggers/imgui.natstepfilter
            ${IMGUI_DIR}/misc/debuggers/imgui.natvis
        )
    endif()

    target_sources(imgui_win32 PRIVATE
        ${IMGUI_HEADERS}
        ${IMGUI_DIR}/backends/imgui_impl_dx9.h
        ${IMGUI_DIR}/backends/imgui_impl_win32.h
    )

    target_include_directories(imgui_win32 PUBLIC
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
    )

    # Windows uses DirectX, no need for OpenGL
    target_link_libraries(imgui_win32 PRIVATE d3d9)
    set(IMGUI_LIBRARY imgui_win32)
else()
    # Linux and macOS use OpenGL3
    add_library(imgui_unix STATIC ${IMGUI_SOURCES}
        ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
        ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
    )

    target_sources(imgui_unix PRIVATE
        ${IMGUI_HEADERS}
        ${IMGUI_DIR}/backends/imgui_impl_glfw.h
        ${IMGUI_DIR}/backends/imgui_impl_opengl3.h
    )

    target_include_directories(imgui_unix PUBLIC
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
        ${glfw_SOURCE_DIR}/include  # Add GLFW 3.3 headers before ImGui's copy
    )

    # Link against GLFW and OpenGL
    target_link_libraries(imgui_unix PRIVATE glfw)
    
    if(APPLE)
        target_link_libraries(imgui_unix PRIVATE "-framework OpenGL")
    else()
        target_link_libraries(imgui_unix PRIVATE GL)
    endif()

    set(IMGUI_LIBRARY imgui_unix)
endif()
