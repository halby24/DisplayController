cmake_minimum_required(VERSION 3.10)
project(DisplayController)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows specific settings
if(WIN32)
    # Force Unicode character set
    add_definitions(-DUNICODE -D_UNICODE)

    # Add Windows SDK dependencies
    set(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION "10.0")
    set(CMAKE_SYSTEM_VERSION "10.0")
endif()

# Add source files
set(SOURCES
    src/main.cpp
    src/MonitorController.cpp
)

# Create executable
add_executable(DisplayController ${SOURCES})

# Include directories
target_include_directories(DisplayController PRIVATE src)

# Windows specific settings
if(WIN32 AND MSVC)
    # Windows specific compiler flags
    target_compile_options(DisplayController PRIVATE
        /utf-8     # Force UTF-8 encoding
        /W4        # Warning level 4
    )

    # Find Visual Studio installation
    if(DEFINED ENV{VSINSTALLDIR})
        set(VS_INSTALL_DIR $ENV{VSINSTALLDIR})
    else()
        set(VS_INSTALL_DIR "C:/Program Files/Microsoft Visual Studio/2022/Community")
    endif()

    # Find Windows SDK installation
    if(DEFINED ENV{WindowsSdkDir})
        set(WIN_SDK_DIR $ENV{WindowsSdkDir})
        set(WIN_SDK_VER $ENV{WindowsSDKVersion})
    else()
        set(WIN_SDK_DIR "C:/Program Files (x86)/Windows Kits/10")
        set(WIN_SDK_VER "10.0.22621.0")
    endif()

    # Add Windows SDK and MSVC include paths
    target_include_directories(DisplayController PRIVATE
        "${WIN_SDK_DIR}/Include/${WIN_SDK_VER}/um"
        "${WIN_SDK_DIR}/Include/${WIN_SDK_VER}/shared"
        "${WIN_SDK_DIR}/Include/${WIN_SDK_VER}/ucrt"
        "${VS_INSTALL_DIR}/VC/Tools/MSVC/14.42.33440/include"
    )

    # Add Windows SDK libraries
    target_link_libraries(DisplayController PRIVATE
        "kernel32.lib"
        "user32.lib"
        "gdi32.lib"
        "winspool.lib"
        "shell32.lib"
        "ole32.lib"
        "oleaut32.lib"
        "uuid.lib"
        "comdlg32.lib"
        "advapi32.lib"
    )
endif()

# Link libraries
target_link_libraries(DisplayController PRIVATE
    wbemuuid     # WMI support
)
