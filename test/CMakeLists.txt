cmake_minimum_required(VERSION 3.15)

# プロジェクトルートでの直接ビルドを防止
if(CMAKE_BINARY_DIR STREQUAL CMAKE_SOURCE_DIR)
    message(FATAL_ERROR "プロジェクトルート内での直接ビルドは禁止されています。別のビルドディレクトリを使用してください。例: mkdir -p build && cd build && cmake ..")
endif()

# テストプロジェクトの設定
project(DisplayControllerTest)

# GoogleTestの依存関係を追加
find_package(GTest CONFIG REQUIRED)

# テスト実行ファイルの作成
add_executable(PluginLoaderTest
    PluginLoaderTest.cpp
    # テスト対象のソースコードを直接含める
    ${CMAKE_SOURCE_DIR}/src/PluginLoader.cpp
)

# インクルードディレクトリの設定
target_include_directories(PluginLoaderTest PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/common
)

# リンク設定
target_link_libraries(PluginLoaderTest PRIVATE
    GTest::gtest
    GTest::gtest_main
    DisplayControllerCommon # メインプロジェクトのライブラリ
)

# C++17を使用
target_compile_features(PluginLoaderTest PRIVATE cxx_std_17)

# テスト用コンパイル定義
target_compile_definitions(PluginLoaderTest PRIVATE
    # DisplayControllerLibのDLLエクスポートではなく内部ビルドとして定義
    DISPLAYCONTROLLERLIB_EXPORTS
    _UNICODE
    UNICODE
)

# テストの登録
include(GoogleTest)
gtest_discover_tests(PluginLoaderTest)

# テスト用プラグインのコピー
add_custom_command(TARGET PluginLoaderTest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
    $<TARGET_FILE_DIR:PluginLoaderTest>/test_plugins
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:DummyLightSensor>
    $<TARGET_FILE_DIR:PluginLoaderTest>/test_plugins/
)
