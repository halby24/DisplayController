# vcpkgのtoolchainを設定
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain file")
endif()

cmake_minimum_required(VERSION 3.15)
project(DisplayController VERSION 1.0.0)

# C++20を使用
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows環境でのUTF-8強制とその他のコンパイラオプション
if(MSVC)
    add_compile_options(/utf-8 /W4)
    add_compile_definitions(_UNICODE UNICODE)
endif()

# vcpkgの依存関係
find_package(CURL CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)

# 共通ライブラリの追加
add_subdirectory(src/common)

# メインプロジェクトのソース
add_library(DisplayController SHARED
    src/BrightnessDaemon.cpp
    src/BrightnessManager.cpp
    src/ConfigManager.cpp
    src/MonitorController.cpp
    src/PluginLoader.cpp
)

# DLLエクスポートマクロを定義
target_compile_definitions(DisplayController PRIVATE
    DISPLAYCONTROLLER_EXPORTS
    _UNICODE
    UNICODE
)

# インクルードディレクトリの設定
target_include_directories(DisplayController PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/common
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# リンク設定
target_link_libraries(DisplayController
    PRIVATE
        CURL::libcurl
        OpenSSL::SSL
        OpenSSL::Crypto
        DisplayControllerCommon
    PUBLIC
        nlohmann_json::nlohmann_json
)

# プラグインディレクトリの追加
add_subdirectory(plugins/SwitchBotLightSensor)
add_subdirectory(plugins/DummyLightSensor)

# テストの有効化
enable_testing()
add_subdirectory(test)

# 実行ファイルの作成
add_executable(brightness_daemon
    src/main.cpp
)

target_link_libraries(brightness_daemon PRIVATE
    DisplayController
)

# インストール設定
install(TARGETS brightness_daemon
    RUNTIME DESTINATION bin
)

# プラグインディレクトリの作成
install(DIRECTORY
    DESTINATION ${CMAKE_INSTALL_PREFIX}/plugins
)
