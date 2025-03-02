#ifndef SWITCHBOT_PLUGIN_H
#define SWITCHBOT_PLUGIN_H

#include "ILightSensorPlugin.h"
#include "SwitchBotLightSensor.h"
#include <memory>
#include <string>

/**
 * @brief SwitchBot Light Sensorプラグイン
 *
 * このクラスは、SwitchBotの照度センサーをDisplayControllerのプラグインとして
 * 提供します。ILightSensorPluginインターフェースを実装し、SwitchBotデバイスとの
 * 通信を行います。
 */
class SwitchBotPlugin : public ILightSensorPlugin {
public:
    /**
     * @brief プラグイン名を取得
     * @return プラグインの一意な名前
     */
    const char* GetPluginName() const override {
        return "SwitchBotLightSensor";
    }

    /**
     * @brief プラグインのバージョンを取得
     * @return バージョン文字列
     */
    const char* GetPluginVersion() const override {
        return "1.0.0";
    }

    /**
     * @brief 照度センサーのインスタンスを作成
     * @param config プラグイン設定（必須キー: "token", "deviceId"）
     * @return ILightSensorインターフェースを実装したセンサーインスタンス
     * @throws std::runtime_error 設定が無効な場合やセンサーの作成に失敗した場合
     */
    std::unique_ptr<ILightSensor> CreateSensor(
        const json& config
    ) override;
};

// プラグインのエクスポート関数
extern "C" {
    /**
     * @brief プラグインのインスタンスを作成
     * @return 作成されたプラグインのインスタンス
     */
    PLUGIN_API ILightSensorPlugin* CreatePlugin() {
        return new SwitchBotPlugin();
    }

    /**
     * @brief プラグインのインスタンスを破棄
     * @param plugin 破棄するプラグインのインスタンス
     */
    PLUGIN_API void DestroyPlugin(ILightSensorPlugin* plugin) {
        delete plugin;
    }
}

#endif // SWITCHBOT_PLUGIN_H
