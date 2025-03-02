cmake_minimum_required(VERSION 3.15)
project(SwitchBotLightSensor VERSION 1.0.0)

# C++17を使用
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# DLLエクスポート用の定義
add_definitions(-DSWITCHBOT_EXPORTS)

# ソースファイル
set(SOURCES
    src/SwitchBotLightSensor.cpp
    src/HttpClient.cpp
    ../src/ConfigManager.cpp
    ../src/StringUtils.cpp
)

# ヘッダーファイル
set(HEADERS
    include/SwitchBotLightSensor.h
    include/HttpClient.h
    include/SwitchBotException.h
    ../src/ConfigManager.h
    ../src/StringUtils.h
)

# DLLターゲットの作成
add_library(${PROJECT_NAME} SHARED
    ${SOURCES}
    ${HEADERS}
)

# インクルードディレクトリの設定
target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

# 依存ライブラリの検索
find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# 依存ライブラリのリンク
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        CURL::libcurl
        OpenSSL::SSL
        OpenSSL::Crypto
        nlohmann_json::nlohmann_json
)

# インストール設定
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(FILES ${HEADERS}
    DESTINATION include/${PROJECT_NAME}
)

# テストの設定
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(test)
endif()
