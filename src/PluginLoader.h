#ifndef DISPLAYCONTROLLER_PLUGINLOADER_H
#define DISPLAYCONTROLLER_PLUGINLOADER_H

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include "ILightSensorPlugin.h"
#include "ILightSensor.h"

using json = nlohmann::json;

/**
 * @brief プラグインローダークラス
 *
 * このクラスは照度センサープラグインの動的ロードと管理を担当します。
 * プラグインの読み込み、インスタンス化、破棄を管理します。
 */
class PluginLoader {
public:
    PluginLoader();
    ~PluginLoader();

    /**
     * @brief プラグインディレクトリからプラグインを読み込む
     * @param pluginDir プラグインが格納されているディレクトリのパス
     * @return 読み込まれたプラグインの数
     * @throws std::runtime_error プラグインの読み込みに失敗した場合
     */
    size_t LoadPlugins(const std::string& pluginDir);

    /**
     * @brief 指定された名前のプラグインからセンサーを作成
     * @param pluginName プラグイン名
     * @param config プラグイン固有の設定
     * @return 作成されたセンサーのインスタンス
     * @throws std::runtime_error プラグインが見つからない場合や作成に失敗した場合
     */
    std::unique_ptr<ILightSensor> CreateSensor(
        const std::string& pluginName,
        const json& config
    );

    /**
     * @brief 読み込まれているプラグインの名前一覧を取得
     * @return プラグイン名の一覧
     */
    std::vector<std::string> GetLoadedPluginNames() const;

private:
    // プラグインのライブラリハンドルを保持
    struct PluginInfo {
        void* handle;                      // DLLハンドル
        ILightSensorPlugin* plugin;        // プラグインインスタンス
        std::string path;                  // プラグインファイルパス
    };

    // プラグイン名とプラグイン情報のマップ
    std::unordered_map<std::string, PluginInfo> m_plugins;

    /**
     * @brief 単一のプラグインを読み込む
     * @param pluginPath プラグインファイルのパス
     * @throws std::runtime_error プラグインの読み込みに失敗した場合
     */
    void LoadPlugin(const std::string& pluginPath);

    /**
     * @brief プラグインをアンロード
     * @param info アンロードするプラグインの情報
     */
    void UnloadPlugin(PluginInfo& info);

    // コピー禁止
    PluginLoader(const PluginLoader&) = delete;
    PluginLoader& operator=(const PluginLoader&) = delete;
};

#endif // DISPLAYCONTROLLER_PLUGINLOADER_H
