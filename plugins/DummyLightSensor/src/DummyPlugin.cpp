#include "DummyPlugin.h"
#include <stdexcept>
#include <sstream>

std::unique_ptr<ILightSensor> DummyPlugin::CreateSensor(
    const json& config
) {
    try {
        // デフォルト値の設定
        int defaultValue = 50;  // デフォルトは50%

        // オプションパラメータの処理
        if (config.contains("defaultValue")) {
            if (!config["defaultValue"].is_number()) {
                throw std::runtime_error(
                    "defaultValueは数値で指定してください"
                );
            }

            defaultValue = config["defaultValue"].get<int>();

            // 値の範囲チェック
            if (defaultValue < 0 || defaultValue > 100) {
                std::ostringstream oss;
                oss << "defaultValueは0-100の範囲で指定してください。指定値: "
                    << defaultValue;
                throw std::runtime_error(oss.str());
            }
        }

        // DummyLightSensorインスタンスの作成
        return std::make_unique<DummyLightSensor>(defaultValue);
    }
    catch (const std::exception& e) {
        std::ostringstream oss;
        oss << "DummyLightSensorの作成に失敗しました: " << e.what();
        throw std::runtime_error(oss.str());
    }
}
