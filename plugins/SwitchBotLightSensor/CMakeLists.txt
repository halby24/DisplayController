cmake_minimum_required(VERSION 3.15)
project(SwitchBotLightSensor VERSION 1.0.0)

# プロジェクトルートでの直接ビルドを防止
if(CMAKE_BINARY_DIR STREQUAL CMAKE_SOURCE_DIR)
    message(FATAL_ERROR "プロジェクトルート内での直接ビルドは禁止されています。別のビルドディレクトリを使用してください。例: mkdir -p build && cd build && cmake ..")
endif()

# プラグインのビルド設定
add_library(SwitchBotLightSensor SHARED
    ${CMAKE_CURRENT_SOURCE_DIR}/src/SwitchBotPlugin.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/SwitchBotLightSensor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HttpClient.cpp
)

# インクルードディレクトリの設定
target_include_directories(SwitchBotLightSensor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/common
)

# DLLエクスポートマクロを定義
target_compile_definitions(SwitchBotLightSensor PRIVATE
    SWITCHBOT_EXPORTS
    LIGHTSENSOR_EXPORTS
    PLUGIN_EXPORTS
    _UNICODE
    UNICODE
)

# 依存ライブラリの設定
find_package(CURL CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)

# リンク設定
target_link_libraries(SwitchBotLightSensor PRIVATE
    CURL::libcurl
    nlohmann_json::nlohmann_json
    OpenSSL::SSL
    OpenSSL::Crypto
    DisplayControllerCommon
    DisplayControllerLib
)

# C++20を使用
target_compile_features(SwitchBotLightSensor PRIVATE cxx_std_20)

# Windows環境でのUTF-8強制とコンパイラ警告レベル
if(MSVC)
    target_compile_options(SwitchBotLightSensor PRIVATE /utf-8 /W4)
endif()

# ビルド時のプラグインディレクトリ設定
set(RUNTIME_DIR "${CMAKE_BINARY_DIR}/$<CONFIG>")
set(RUNTIME_PLUGINS_DIR "${RUNTIME_DIR}/plugins")

# プラグインディレクトリを作成
add_custom_command(
    TARGET SwitchBotLightSensor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${RUNTIME_PLUGINS_DIR}"
)

# プラグインDLLをコピー
add_custom_command(
    TARGET SwitchBotLightSensor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "$<TARGET_FILE:SwitchBotLightSensor>"
    "${RUNTIME_PLUGINS_DIR}"
)

add_custom_command(
    TARGET SwitchBotLightSensor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "$<TARGET_FILE_DIR:SwitchBotLightSensor>/libcurl-d.dll"
    "$<TARGET_FILE_DIR:SwitchBotLightSensor>/libcrypto-3-x64.dll"
    "$<TARGET_FILE_DIR:SwitchBotLightSensor>/zlibd1.dll"
    "${RUNTIME_DIR}"
)

# インストール設定
install(TARGETS SwitchBotLightSensor
    RUNTIME DESTINATION plugins
    LIBRARY DESTINATION plugins
)
