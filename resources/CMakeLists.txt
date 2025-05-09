# Add resources (optional)
if (MSVC_IDE)
    add_library(resources INTERFACE
        visualstudio/tcbspan.natvis
    )
endif()
