#include "SwitchBotPlugin.h"
#include <stdexcept>
#include <sstream>

std::unique_ptr<ILightSensor> SwitchBotPlugin::CreateSensor(
    const json& config
) {
    try {
        // 必須パラメータの検証
        if (!config.contains("token")) {
            throw std::runtime_error(
                "設定にtokenが指定されていません"
            );
        }
        if (!config.contains("deviceId")) {
            throw std::runtime_error(
                "設定にdeviceIdが指定されていません"
            );
        }

        // パラメータの型チェック
        if (!config["token"].is_string()) {
            throw std::runtime_error(
                "tokenは文字列で指定してください"
            );
        }
        if (!config["deviceId"].is_string()) {
            throw std::runtime_error(
                "deviceIdは文字列で指定してください"
            );
        }

        // パラメータの取得
        const std::string& token = config["token"].get<std::string>();
        const std::string& deviceId = config["deviceId"].get<std::string>();

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

        // 接続テスト
        try {
            sensor->GetLightLevel();
        }
        catch (const std::exception& e) {
            std::ostringstream oss;
            oss << "センサーの接続テストに失敗しました: " << e.what();
            throw std::runtime_error(oss.str());
        }

        return sensor;
    }
    catch (const std::exception& e) {
        std::ostringstream oss;
        oss << "SwitchBotLightSensorの作成に失敗しました: " << e.what();
        throw std::runtime_error(oss.str());
    }
}
