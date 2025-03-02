cmake_minimum_required(VERSION 3.15)
project(SwitchBotLightSensor VERSION 1.0.0)

# プラグインのビルド設定
add_library(SwitchBotLightSensor SHARED
    ${CMAKE_CURRENT_SOURCE_DIR}/src/SwitchBotPlugin.cpp
    ${CMAKE_SOURCE_DIR}/SwitchBotLightSensor/src/SwitchBotLightSensor.cpp
    ${CMAKE_SOURCE_DIR}/SwitchBotLightSensor/src/HttpClient.cpp
)

# プラグインのエクスポートマクロを定義
target_compile_definitions(SwitchBotLightSensor PRIVATE
    PLUGIN_EXPORTS
)

# インクルードディレクトリの設定
target_include_directories(SwitchBotLightSensor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/SwitchBotLightSensor/include
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
)

# C++17を使用
target_compile_features(SwitchBotLightSensor PRIVATE cxx_std_17)

# インストール設定
install(TARGETS SwitchBotLightSensor
    RUNTIME DESTINATION plugins
    LIBRARY DESTINATION plugins
)
