enable_testing()

# GoogleTestの検索
find_package(GTest REQUIRED)

# テストターゲットの作成
add_executable(SwitchBotLightSensorTest
    SwitchBotLightSensorTest.cpp
)

# インクルードディレクトリの設定
target_include_directories(SwitchBotLightSensorTest
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

# リンク設定
target_link_libraries(SwitchBotLightSensorTest
    PRIVATE
        GTest::GTest
        GTest::Main
        SwitchBotLightSensor
)

# テストの登録
add_test(NAME SwitchBotLightSensorTest COMMAND SwitchBotLightSensorTest)
