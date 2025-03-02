cmake_minimum_required(VERSION 3.15)
project(DummyLightSensor VERSION 1.0.0)

# プロジェクトルートでの直接ビルドを防止
if(CMAKE_BINARY_DIR STREQUAL CMAKE_SOURCE_DIR)
    message(FATAL_ERROR "プロジェクトルート内での直接ビルドは禁止されています。別のビルドディレクトリを使用してください。例: mkdir -p build && cd build && cmake ..")
endif()

# プラグインのビルド設定
add_library(DummyLightSensor SHARED
    src/DummyLightSensor.cpp
    src/DummyPlugin.cpp
)

# プラグインのエクスポートマクロとUnicodeサポートを定義
target_compile_definitions(DummyLightSensor PRIVATE
    PLUGIN_EXPORTS
    LIGHTSENSOR_EXPORTS
    _UNICODE
    UNICODE
)

# インクルードディレクトリの設定
target_include_directories(DummyLightSensor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

# 依存ライブラリの設定
find_package(nlohmann_json CONFIG REQUIRED)

# リンク設定
target_link_libraries(DummyLightSensor PRIVATE
    nlohmann_json::nlohmann_json
)

# C++20を使用
target_compile_features(DummyLightSensor PRIVATE cxx_std_20)

# Windows環境でのコンパイラオプション設定
if(MSVC)
    target_compile_options(DummyLightSensor PRIVATE /utf-8 /W4)
endif()

# ビルド時のプラグインディレクトリ設定
set(RUNTIME_PLUGINS_DIR "${CMAKE_BINARY_DIR}/$<CONFIG>/plugins")

# プラグインディレクトリを作成
add_custom_command(
    TARGET DummyLightSensor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${RUNTIME_PLUGINS_DIR}"
)

# プラグインDLLをコピー
add_custom_command(
    TARGET DummyLightSensor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:DummyLightSensor>"
        "${RUNTIME_PLUGINS_DIR}"
)

# インストール設定
install(TARGETS DummyLightSensor
    RUNTIME DESTINATION plugins
    LIBRARY DESTINATION plugins
)
