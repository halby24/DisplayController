cmake_minimum_required(VERSION 3.10)
project(DisplayController)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add source files
set(SOURCES
    src/main.cpp
    src/MonitorController.cpp
)

# Create executable
add_executable(DisplayController ${SOURCES})

# Include directories
target_include_directories(DisplayController PRIVATE src)
