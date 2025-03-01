cmake_minimum_required(VERSION 3.15)

# Configure vcpkg
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    # First try VCPKG_ROOT environment variable
    if(DEFINED ENV{VCPKG_ROOT})
        set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
            CACHE STRING "")

    # Then try standard installation locations
    elseif(EXISTS "C:/dev/vcpkg/scripts/buildsystems/vcpkg.cmake")
        set(CMAKE_TOOLCHAIN_FILE "C:/dev/vcpkg/scripts/buildsystems/vcpkg.cmake"
            CACHE STRING "")
    elseif(EXISTS "C:/vcpkg/scripts/buildsystems/vcpkg.cmake")
        set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake"
            CACHE STRING "")
    endif()

    if(DEFINED CMAKE_TOOLCHAIN_FILE)
        message(STATUS "Using vcpkg toolchain file: ${CMAKE_TOOLCHAIN_FILE}")
    else()
        message(FATAL_ERROR "Could not find vcpkg toolchain file. Please set VCPKG_ROOT environment variable or install vcpkg in a standard location.")
    endif()
endif()

# Set build directory if not already set
if(NOT CMAKE_BINARY_DIR)
    set(CMAKE_BINARY_DIR "${CMAKE_SOURCE_DIR}/build")
endif()

project(DisplayController)

# Configure vcpkg package preferences
set(CMAKE_PREFIX_PATH "${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}" ${CMAKE_PREFIX_PATH})

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

# Find required packages
find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json REQUIRED)

# Add library source files
set(LIB_SOURCES
    src/MonitorController.cpp
    src/MonitorController.h
)

# Add daemon source files
set(DAEMON_SOURCES
    src/BrightnessDaemon.cpp
    src/BrightnessManager.cpp
    src/BrightnessManager.h
    src/DummyLightSensor.cpp
    src/DummyLightSensor.h
    src/ILightSensor.h
)

# Add SwitchBot Light Sensor
add_subdirectory(SwitchBotLightSensor)

# Create library as DLL
add_library(DisplayControllerLib SHARED ${LIB_SOURCES})
target_include_directories(DisplayControllerLib
    PRIVATE
    src
)
target_link_libraries(DisplayControllerLib PUBLIC nlohmann_json::nlohmann_json)

# Define DLL export macro
target_compile_definitions(DisplayControllerLib
    PRIVATE
    DISPLAYCONTROLLERLIB_EXPORTS
)

# Set source file properties for UTF-8
set_property(SOURCE ${LIB_SOURCES} PROPERTY COMPILE_FLAGS "/utf-8")

# Create main executable
add_executable(DisplayController src/main.cpp)
target_link_libraries(DisplayController PRIVATE DisplayControllerLib)

# Create daemon executable
add_executable(BrightnessDaemon WIN32 ${DAEMON_SOURCES})
target_link_libraries(BrightnessDaemon
    PRIVATE
    DisplayControllerLib
    nlohmann_json::nlohmann_json
    SwitchBotLightSensor
)
target_include_directories(BrightnessDaemon
    PRIVATE
    src
    SwitchBotLightSensor/include
)

# Add Windows specific settings for BrightnessDaemon
if(WIN32 AND MSVC)
    target_compile_options(BrightnessDaemon PRIVATE
        /utf-8 # Force UTF-8 encoding
        /W4 # Warning level 4
    )

    # Add Windows SDK include paths
    target_include_directories(BrightnessDaemon PRIVATE
        "${WIN_SDK_DIR}/Include/${WIN_SDK_VER}/um"
        "${WIN_SDK_DIR}/Include/${WIN_SDK_VER}/shared"
        "${WIN_SDK_DIR}/Include/${WIN_SDK_VER}/ucrt"
        "${VS_INSTALL_DIR}/VC/Tools/MSVC/14.42.33440/include"
    )

    # Add Windows SDK libraries
    target_link_libraries(BrightnessDaemon PRIVATE
        "kernel32.lib"
        "user32.lib"
        "gdi32.lib"
        "shell32.lib"
        "ole32.lib"
        "oleaut32.lib"
        "uuid.lib"
        "comdlg32.lib"
        "advapi32.lib"
    )
endif()

# Windows specific settings
if(WIN32 AND MSVC)
    # Windows specific compiler flags
    target_compile_options(DisplayController PRIVATE
        /utf-8 # Force UTF-8 encoding
        /W4 # Warning level 4
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
    wbemuuid # WMI support
)
