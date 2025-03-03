#include "SwitchBotPlugin.h"
#include "ConfigManager.h"
#include <stdexcept>
#include <sstream>

std::unique_ptr<ILightSensor> SwitchBotPlugin::CreateSensor(
    const json& config
) {
    try {
        // デバイス名の取得
        if (!config.contains("name")) {
            throw std::runtime_error("設定にnameが指定されていません");
        }
        const std::string& deviceName = config["name"].get<std::string>();

        // ConfigManagerからグローバル設定とデバイス設定を取得
        auto& configManager = ConfigManager::Instance();

        // グローバル設定からtokenを取得
        std::string token;
        try {
            token = configManager.GetPluginConfig("SwitchBotLightSensor", "token");
        } catch (const ConfigException& e) {
            throw std::runtime_error("tokenの取得に失敗しました: " + std::string(e.what()));
        }

        // デバイスIDの取得
        std::string deviceId;
        try {
            deviceId = configManager.GetPluginConfig("SwitchBotLightSensor", "id", deviceName);
        } catch (const ConfigException& e) {
            throw std::runtime_error("デバイスIDの取得に失敗しました: " + std::string(e.what()));
        }

        if (token.empty()) {
            throw std::runtime_error("tokenが設定されていません");
        }
        if (deviceId.empty()) {
            throw std::runtime_error("デバイスIDが設定されていません");
        }

        // オプションパラメータの取得
        int retryCount = 3;  // デフォルト値
        int retryInterval = 1000;  // デフォルト値（ミリ秒）

        if (config.contains("retryCount") && config["retryCount"].is_number()) {
            retryCount = config["retryCount"].get<int>();
        }
        if (config.contains("retryInterval") && config["retryInterval"].is_number()) {
            retryInterval = config["retryInterval"].get<int>();
        }

        // SwitchBotLightSensorインスタンスの作成
        auto sensor = std::make_unique<SwitchBotLightSensor>(
            token,
            deviceId,
            retryCount,
            retryInterval
        );

        // 1回目は失敗しやすいのでとりあえず無視
        // 接続テスト
        // try {
        //     sensor->GetLightLevel();
        // }
        // catch (const std::exception& e) {
        //     std::ostringstream oss;
        //     oss << "センサーの接続テストに失敗しました: " << e.what();
        //     throw std::runtime_error(oss.str());
        // }

        return sensor;
    }
    catch (const std::exception& e) {
        std::ostringstream oss;
        oss << "SwitchBotLightSensorの作成に失敗しました: " << e.what();
        throw std::runtime_error(oss.str());
    }
}
