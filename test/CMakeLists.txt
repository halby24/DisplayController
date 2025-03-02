cmake_minimum_required(VERSION 3.15)

# テストプロジェクトの設定
project(DisplayControllerTest)

# GoogleTestの依存関係を追加
find_package(GTest CONFIG REQUIRED)

# テスト実行ファイルの作成
add_executable(plugin_loader_test
    PluginLoaderTest.cpp
)

# インクルードディレクトリの設定
target_include_directories(plugin_loader_test PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

# リンク設定
target_link_libraries(plugin_loader_test PRIVATE
    GTest::gtest
    GTest::gtest_main
    PluginLoader  # メインプロジェクトのライブラリ
)

# C++17を使用
target_compile_features(plugin_loader_test PRIVATE cxx_std_17)

# テストの登録
include(GoogleTest)
gtest_discover_tests(plugin_loader_test)

# テスト用プラグインのコピー
add_custom_command(TARGET plugin_loader_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        $<TARGET_FILE_DIR:plugin_loader_test>/test_plugins
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:DummyLightSensor>
        $<TARGET_FILE_DIR:plugin_loader_test>/test_plugins/
)
