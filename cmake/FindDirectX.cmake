# FindDirectX.cmake
#
# Finds DirectX SDK through the Windows SDK
#
# Sets up:
# DirectX_FOUND - System has DirectX SDK
# DirectX_INCLUDE_DIRS - DirectX include directories
# DirectX_LIBRARIES - DirectX libraries
# DirectX_D3D9_LIBRARY - Path to d3d9.lib

include(FindPackageHandleStandardArgs)

# Determine architecture
if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(DIRECTX_ARCHITECTURE x64)
else()
    set(DIRECTX_ARCHITECTURE x86)
endif()

# Windows SDK paths
set(DIRECTX_ROOT_DIR
    "$ENV{WINDOWSSDKDIR}"  # Primary Windows SDK location from environment
    "C:/Program Files (x86)/Windows Kits/10"  # Windows 10+ SDK base path
    "C:/Program Files (x86)/Windows Kits/8.1"  # Windows 8.1 SDK
    "C:/Program Files (x86)/Microsoft DirectX SDK (June 2010)"  # Legacy DirectX SDK
)

# Find DirectX headers
find_path(DirectX_INCLUDE_DIR
    NAMES d3d9.h
    PATHS ${DIRECTX_ROOT_DIR}
    PATH_SUFFIXES
        "Include/*/um"      # Matches any SDK version
        "include/*/um"      # Alternative casing
        "Include/um"        # Fallback
        "include/um"        # Fallback
        "include"           # Last resort fallback
)

# Find DirectX libraries
find_library(DirectX_D3D9_LIBRARY
    NAMES d3d9
    PATHS ${DIRECTX_ROOT_DIR}
    PATH_SUFFIXES
        "Lib/*/um/${DIRECTX_ARCHITECTURE}"      # Matches any SDK version
        "lib/*/um/${DIRECTX_ARCHITECTURE}"      # Alternative casing
        "Lib/${DIRECTX_ARCHITECTURE}/um"        # Fallback
        "lib/${DIRECTX_ARCHITECTURE}/um"        # Fallback
        "lib/${DIRECTX_ARCHITECTURE}"           # Last resort fallback
)

# Handle the QUIETLY and REQUIRED arguments and set DirectX_FOUND
find_package_handle_standard_args(DirectX
    REQUIRED_VARS
        DirectX_INCLUDE_DIR
        DirectX_D3D9_LIBRARY
)

if(DirectX_FOUND)
    set(DirectX_INCLUDE_DIRS ${DirectX_INCLUDE_DIR})
    set(DirectX_LIBRARIES ${DirectX_D3D9_LIBRARY})
    
    if(NOT TARGET DirectX::D3D9)
        add_library(DirectX::D3D9 UNKNOWN IMPORTED)
        set_target_properties(DirectX::D3D9 PROPERTIES
            IMPORTED_LOCATION "${DirectX_D3D9_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${DirectX_INCLUDE_DIR}"
        )
    endif()
endif()

mark_as_advanced(
    DirectX_INCLUDE_DIR
    DirectX_D3D9_LIBRARY
)
