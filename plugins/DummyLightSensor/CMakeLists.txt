cmake_minimum_required(VERSION 3.15)
project(DummyLightSensor VERSION 1.0.0)

# プラグインのビルド設定
add_library(DummyLightSensor SHARED
    src/DummyLightSensor.cpp
    src/DummyPlugin.cpp
)

# プラグインのエクスポートマクロを定義
target_compile_definitions(DummyLightSensor PRIVATE
    PLUGIN_EXPORTS
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

# C++17を使用
target_compile_features(DummyLightSensor PRIVATE cxx_std_17)

# インストール設定
install(TARGETS DummyLightSensor
    RUNTIME DESTINATION plugins
    LIBRARY DESTINATION plugins
)
