#ifndef DISPLAYCONTROLLER_ILIGHTSENSORPLUGIN_H
#define DISPLAYCONTROLLER_ILIGHTSENSORPLUGIN_H

#include <memory>
#include <nlohmann/json.hpp>
#include "ILightSensor.h"

// DLLエクスポート/インポートマクロ
#ifdef _WIN32
    #ifdef PLUGIN_EXPORTS
        #define PLUGIN_API __declspec(dllexport)
    #else
        #define PLUGIN_API __declspec(dllimport)
    #endif
#else
    #define PLUGIN_API
#endif

using json = nlohmann::json;

/**
 * @brief 照度センサープラグインのインターフェース
 *
 * このインターフェースは、照度センサーのプラグインが実装すべき
 * メソッドを定義します。各プラグインは、このインターフェースを
 * 実装することで、BrightnessDaemonと統合することができます。
 */
class ILightSensorPlugin {
public:
    virtual ~ILightSensorPlugin() = default;

    /**
     * @brief プラグイン名を取得
     * @return プラグインの一意な名前
     */
    virtual const char* GetPluginName() const = 0;

    /**
     * @brief プラグインのバージョンを取得
     * @return バージョン文字列（例: "1.0.0"）
     */
    virtual const char* GetPluginVersion() const = 0;

    /**
     * @brief 照度センサーのインスタンスを作成
     * @param config プラグイン固有の設定（JSON形式）
     * @return ILightSensorインターフェースを実装したセンサーインスタンス
     * @throws std::runtime_error 設定が無効な場合やセンサーの作成に失敗した場合
     */
    virtual std::unique_ptr<ILightSensor> CreateSensor(
        const json& config
    ) = 0;
};

// プラグインのエクスポート関数
extern "C" {
    /**
     * @brief プラグインのインスタンスを作成
     * @return 作成されたプラグインのインスタンス
     */
    PLUGIN_API ILightSensorPlugin* CreatePlugin();

    /**
     * @brief プラグインのインスタンスを破棄
     * @param plugin 破棄するプラグインのインスタンス
     */
    PLUGIN_API void DestroyPlugin(ILightSensorPlugin* plugin);
}

#endif // DISPLAYCONTROLLER_ILIGHTSENSORPLUGIN_H
