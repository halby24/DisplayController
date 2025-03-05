#include <gtest/gtest.h>
#include "PluginLoader.h"
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

class PluginLoaderTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // テスト用のプラグインディレクトリを作成
        fs::create_directories("test_plugins");
    }

    void TearDown() override
    {
        // テストディレクトリを削除
        fs::remove_all("test_plugins");
    }

    // テスト用の設定JSONを作成
    json CreateTestConfig(const std::string &token = "test-token",
                          const std::string &deviceId = "test-device")
    {
        return json{
            {"token", token},
            {"deviceId", deviceId}};
    }
};

auto currentDir = fs::current_path();
auto pluginsDir = (currentDir / "../../Debug/plugins").make_preferred();

TEST_F(PluginLoaderTest, LoadNonExistentDirectory)
{
    PluginLoader loader;
    EXPECT_THROW(loader.LoadPlugins("non_existent_dir"), std::runtime_error);
}

TEST_F(PluginLoaderTest, LoadEmptyDirectory)
{
    PluginLoader loader;
    size_t count = loader.LoadPlugins("test_plugins");
    EXPECT_EQ(count, 0);
}

TEST_F(PluginLoaderTest, CreateSensorWithInvalidPlugin)
{
    PluginLoader loader;
    EXPECT_THROW(
        loader.CreateSensor("NonExistentPlugin", CreateTestConfig()),
        std::runtime_error);
}

TEST_F(PluginLoaderTest, GetLoadedPluginNames)
{
    PluginLoader loader;
    loader.LoadPlugins("test_plugins");
    auto names = loader.GetLoadedPluginNames();
    EXPECT_TRUE(names.empty());
}

// プラグインの動的ロードのテスト
// 注: このテストは実際のプラグインDLLが必要
TEST_F(PluginLoaderTest, LoadAndCreateDummyPlugin)
{
    PluginLoader loader;
    size_t count = loader.LoadPlugins(pluginsDir.string());
    EXPECT_EQ(count, 1);

    auto names = loader.GetLoadedPluginNames();
    EXPECT_EQ(names.size(), 1);
    EXPECT_EQ(names[0], "DummyLightSensor");

    // センサーの作成テスト
    auto sensor = loader.CreateSensor("DummyLightSensor", json::object());
    EXPECT_NE(sensor, nullptr);
    EXPECT_EQ(sensor->GetLightLevel(), 50); // デフォルト値
}

// 無効なプラグインファイルのテスト
TEST_F(PluginLoaderTest, LoadInvalidPlugin)
{
    // 無効なDLLファイルを作成
    std::ofstream invalid_dll("test_plugins/invalid.dll");
    invalid_dll << "This is not a valid DLL file";
    invalid_dll.close();

    PluginLoader loader;
    size_t count = loader.LoadPlugins("test_plugins");
    EXPECT_EQ(count, 0); // 無効なプラグインは読み込まれないはず
}

// プラグインの設定テスト
TEST_F(PluginLoaderTest, PluginConfiguration)
{
    PluginLoader loader;
    loader.LoadPlugins(pluginsDir.string());

    // カスタム設定でセンサーを作成
    json config{{"defaultValue", 75}};
    auto sensor = loader.CreateSensor("DummyLightSensor", config);
    EXPECT_NE(sensor, nullptr);
    EXPECT_EQ(sensor->GetLightLevel(), 75); // カスタム値

    // 無効な設定値でのテスト
    json invalid_config{{"defaultValue", 150}}; // 範囲外の値
    EXPECT_THROW(
        loader.CreateSensor("DummyLightSensor", invalid_config),
        std::runtime_error);
}

// configファイルの形式変更のテスト
TEST_F(PluginLoaderTest, ConfigFileFormat)
{
    auto configPath = (currentDir / "../../../docs/user/samples/config.json.sample").make_preferred().string();

    // テスト用のconfigファイルを作成
    // config.json.sampleの内容を読み込み、config.jsonとして書き出す
    std::ifstream config_sample_file(configPath);
    std::string config_content((std::istreambuf_iterator<char>(config_sample_file)),
                             std::istreambuf_iterator<char>());
    config_sample_file.close();

    std::ofstream config_file("test_plugins/config.json");
    config_file << config_content;
    config_file.close();

    PluginLoader loader;
    loader.LoadPlugins(pluginsDir.string());

    // センサーの作成テスト
    json config{{"defaultValue", 75}};
    auto sensor = loader.CreateSensor("DummyLightSensor", config);
    EXPECT_NE(sensor, nullptr);
    EXPECT_EQ(sensor->GetLightLevel(), 75); // カスタム値
}
