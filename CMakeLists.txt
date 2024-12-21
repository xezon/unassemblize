cmake_minimum_required(VERSION 3.16)

# Use packagename_ROOT for FindPackage.
if(POLICY CMP0074)
    cmake_policy(SET CMP0074 NEW)
endif()

# Disable default MSVC warning level so we can set it ourselves.
if(POLICY CMP0092)
    cmake_policy(SET CMP0092 NEW)
endif()

# Disable default MSVC runtime hardcoding.
if(POLICY CMP0091)
    cmake_policy(SET CMP0091 NEW)
endif()

# Platform detection
if(WIN32 OR "${CMAKE_SYSTEM}" MATCHES "Windows")
    set(WINDOWS TRUE)
endif()

project(unassemblize LANGUAGES C CXX)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
set(CMAKE_CXX_STANDARD 17)

# Platform-specific configurations
if(WINDOWS)
    if(MSVC)
        # Build with multiple processes: speeds up build on multi core processors
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
    endif()
    # Spawn no console by default.
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
elseif(APPLE OR UNIX)
    # Find OpenGL
    find_package(OpenGL REQUIRED)
    
    if(APPLE)
        add_definitions(-DGL_SILENCE_DEPRECATION)
    endif()
endif()

# Set up a format target to do automated clang format checking.
find_package(ClangFormat)
include(ClangFormat)
include(FetchContent)

# Configure dependency options
set(ZYDIS_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(ZYDIS_FEATURE_ENCODER OFF CACHE BOOL "" FORCE)

# Configure LIEF options before declaring
set(LIEF_EXAMPLES OFF CACHE BOOL "" FORCE)
set(LIEF_INSTALL OFF CACHE BOOL "" FORCE)
set(LIEF_C_API OFF CACHE BOOL "" FORCE)
set(LIEF_PYTHON_API OFF CACHE BOOL "" FORCE)
set(LIEF_TESTS OFF CACHE BOOL "" FORCE)
set(LIEF_ART OFF CACHE BOOL "" FORCE)
set(LIEF_DEX OFF CACHE BOOL "" FORCE)
set(LIEF_VDEX OFF CACHE BOOL "" FORCE)

# GLFW configuration and fetching
if(APPLE OR UNIX)
    FetchContent_Declare(glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.3.9
    )

    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

    FetchContent_MakeAvailable(glfw)
endif()

# Declare dependencies
FetchContent_Declare(
    zydis
    GIT_REPOSITORY https://github.com/zyantific/zydis.git
    GIT_TAG        1ba75aeefae37094c7be8eba07ff81d4fe0f1f20
    SOURCE_DIR     ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/zydis
)

FetchContent_Declare(
    lief
    GIT_REPOSITORY https://github.com/lief-project/LIEF.git
    GIT_TAG        2d9855fc7f9d4ce6325245f8b75c98eb7663db60
    SOURCE_DIR     ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/lief
)

# Make them available after configuration
FetchContent_MakeAvailable(zydis lief)

FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        bc889afb4c5bf1c0d8ee29ef35eaaf4c8bef8a5d
    SOURCE_DIR     ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/nlohmann_json
)
FetchContent_MakeAvailable(json)

FetchContent_Declare(fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG        0c9fce2ffefecfdce794e1859584e25877b7b592
    SOURCE_DIR     ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/fmt
)
FetchContent_MakeAvailable(fmt)

FetchContent_Declare(cxxopts
    GIT_REPOSITORY https://github.com/jarro2783/cxxopts.git
    GIT_TAG        3bf268481da8208d171d8908e6491459de3651d7
    SOURCE_DIR     ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/cxxopts
)
FetchContent_MakeAvailable(cxxopts)

FetchContent_Declare(readerwriterqueue
    GIT_REPOSITORY https://github.com/xezon/readerwriterqueue
    GIT_TAG        master
    SOURCE_DIR     ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/readerwriterqueue
)
FetchContent_MakeAvailable(readerwriterqueue)

# TODO: Can we use FetchContent_MakeAvailable without getting the test suite for it?
FetchContent_Populate(span DOWNLOAD_EXTRACT_TIMESTAMP
    GIT_REPOSITORY https://github.com/xezon/span
    GIT_TAG        master
    SOURCE_DIR     ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/span)

set(GIT_PRE_CONFIGURE_FILE "gitinfo.cpp.in")
set(GIT_POST_CONFIGURE_FILE "${CMAKE_CURRENT_BINARY_DIR}/gitinfo.cpp")

include(GitWatcher)
include(imgui)
include(imguifiledialog)
if(WINDOWS)
include(FindDiaSDK)
endif()

add_executable(unassemblize)

# Common sources for all platforms
target_sources(unassemblize PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}/gitinfo.cpp
    gitinfo.h
    cmake/FindDiaSDK.cmake
    cmake/imgui.cmake
    cmake/imguifiledialog.cmake
    src/asmmatcher.cpp
    src/asmmatcher.h
    src/asmmatchertypes.cpp
    src/asmmatchertypes.h
    src/asmprinter.cpp
    src/asmprinter.h
    src/commontypes.h
    src/executable.cpp
    src/executable.h
    src/executabletypes.cpp
    src/executabletypes.h
    src/filecontentstorage.cpp
    src/filecontentstorage.h
    src/function.cpp
    src/function.h
    src/functiontypes.cpp
    src/functiontypes.h
    src/main.cpp
    src/options.cpp
    src/options.h
    src/pdbreader.cpp
    src/pdbreader.h
    src/pdbreadertypes.cpp
    src/pdbreadertypes.h
    src/runner.cpp
    src/runner.h
    src/runnerasync.cpp
    src/runnerasync.h
    src/runneroptions.h
    src/util.cpp
    src/util.h
    src/version.cpp
    src/version.h
    src/workqueue.cpp
    src/workqueue.h
    src/imguiclient/asyncworkstate.h
    src/imguiclient/imconfig.h # TODO: Is this file actually used?
    src/imguiclient/imguiapp.cpp
    src/imguiclient/imguiapp.h
    src/imguiclient/processedstate.cpp
    src/imguiclient/processedstate.h
    src/imguiclient/programcomparisondescriptor.cpp
    src/imguiclient/programcomparisondescriptor.h
    src/imguiclient/programfilecommon.h
    src/imguiclient/programfiledescriptor.cpp
    src/imguiclient/programfiledescriptor.h
    src/imguiclient/programfilerevisiondescriptor.cpp
    src/imguiclient/programfilerevisiondescriptor.h
    src/imguiclient/utility/imgui_misc.cpp
    src/imguiclient/utility/imgui_misc.h
    src/imguiclient/utility/imgui_scoped.h
    src/imguiclient/utility/imgui_text_filter.cpp
    src/imguiclient/utility/imgui_text_filter.h
    src/util/bitarray.h
    src/util/nocopy.h
)

# Platform-specific configurations
if(WINDOWS)
    target_sources(unassemblize PRIVATE
        src/imguiclient/imguiwin32.cpp
        src/imguiclient/imguiwin32.h
    )
    
    target_link_libraries(unassemblize PRIVATE
        d3d9.lib # TODO: figure out if any "FindDirectX" logic is needed here.
        imgui_win32
        ${DIASDK_LIBRARIES}
    )

    target_include_directories(unassemblize PRIVATE
        wincompat
        ${DIASDK_INCLUDE_DIRS}
    )
elseif(APPLE OR UNIX)
    target_sources(unassemblize PRIVATE
        src/imguiclient/imguiglfw.cpp
        src/imguiclient/imguiglfw.h
    )
    
    target_link_libraries(unassemblize PRIVATE
        imgui_unix
        glfw
        ${OPENGL_LIBRARIES}
    )
    
    if(APPLE)
        target_link_libraries(unassemblize PRIVATE
            "-framework OpenGL"
            "-framework Cocoa"
            "-framework IOKit"
            "-framework CoreVideo"
        )
    endif()
endif()

# Common libraries for all platforms
target_link_libraries(unassemblize PRIVATE
    ImGuiFileDialog
    Zydis
    LIEF::LIEF
    nlohmann_json
    fmt::fmt
    cxxopts::cxxopts
    readerwriterqueue
)

target_include_directories(unassemblize PRIVATE
    .
    src
    3rdparty/span/include
)

target_compile_definitions(unassemblize PRIVATE
    $<$<CONFIG:MinSizeRel,Release,RelWithDebInfo>:RELEASE=1> # Can we do this nicer?
)
