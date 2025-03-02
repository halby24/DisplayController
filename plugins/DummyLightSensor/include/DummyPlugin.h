#ifndef DUMMY_PLUGIN_H
#define DUMMY_PLUGIN_H

#include "ILightSensorPlugin.h"
#include "DummyLightSensor.h"
#include <memory>
#include <string>

/**
 * @brief ダミー照度センサープラグイン
 *
 * このクラスは、テストやフォールバック用のダミー照度センサーを
 * プラグインとして提供します。実際のハードウェアを使用せず、
 * シミュレートされた照度値を返します。
 */
class DummyPlugin : public ILightSensorPlugin {
public:
    /**
     * @brief プラグイン名を取得
     * @return プラグインの一意な名前
     */
    const char* GetPluginName() const override {
        return "DummyLightSensor";
    }

    /**
     * @brief プラグインのバージョンを取得
     * @return バージョン文字列
     */
    const char* GetPluginVersion() const override {
        return "1.0.0";
    }

    /**
     * @brief ダミー照度センサーのインスタンスを作成
     * @param config プラグイン設定（オプション: "defaultValue": 0-100の整数）
     * @return ILightSensorインターフェースを実装したセンサーインスタンス
     * @throws std::runtime_error 設定が無効な場合
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
        return new DummyPlugin();
    }

    /**
     * @brief プラグインのインスタンスを破棄
     * @param plugin 破棄するプラグインのインスタンス
     */
    PLUGIN_API void DestroyPlugin(ILightSensorPlugin* plugin) {
        delete plugin;
    }
}

#endif // DUMMY_PLUGIN_H
