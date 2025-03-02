# プラグインアーキテクチャ設計

## 概要
BrightnessDaemonをプラグインベースのアーキテクチャに移行し、具体的なセンサーデバイスへの依存を排除します。各デバイスドライバーはプラグインとして実装され、共通のインターフェースを通じてBrightnessDaemonと通信します。

## 目的
1. 疎結合な設計の実現
2. 新しいデバイスの追加を容易にする
3. テスト容易性の向上
4. メンテナンス性の向上

## アーキテクチャ概要

### コアシステム（BrightnessDaemon）
```
DisplayController/
├── src/
│   ├── BrightnessDaemon.cpp      # プラグインローダーを使用
│   ├── PluginLoader.h            # プラグイン読み込み機能
│   ├── PluginLoader.cpp
│   └── ILightSensor.h           # プラグインインターフェース
```

### プラグインシステム
```
plugins/
├── SwitchBotLightSensor/        # SwitchBotプラグイン
├── DummyLightSensor/           # テスト用プラグイン
└── future_plugins/             # 将来的なプラグイン
```

## インターフェース設計

### プラグインインターフェース
```cpp
// ILightSensorPlugin.h
class ILightSensorPlugin {
public:
    virtual ~ILightSensorPlugin() = default;

    // プラグイン情報
    virtual const char* GetPluginName() const = 0;
    virtual const char* GetPluginVersion() const = 0;

    // ファクトリーメソッド
    virtual std::unique_ptr<ILightSensor> CreateSensor(
        const json& config
    ) = 0;
};

// プラグインのエクスポート関数
extern "C" {
    PLUGIN_API ILightSensorPlugin* CreatePlugin();
    PLUGIN_API void DestroyPlugin(ILightSensorPlugin* plugin);
}
```

### プラグインローダー
```cpp
class PluginLoader {
public:
    // プラグインの読み込み
    std::vector<std::unique_ptr<ILightSensorPlugin>> LoadPlugins(
        const std::string& pluginDir
    );

    // 設定に基づいてセンサーを作成
    std::unique_ptr<ILightSensor> CreateSensor(
        const std::string& pluginName,
        const json& config
    );

private:
    std::vector<void*> m_loadedLibraries;
    std::vector<std::unique_ptr<ILightSensorPlugin>> m_plugins;
};
```

## 設定ファイル形式
```json
{
    "sensor": {
        "plugin": "SwitchBotLightSensor",
        "config": {
            "token": "your-api-token",
            "deviceId": "your-device-id"
        }
    }
}
```

## プラグイン実装例（SwitchBot）
```cpp
class SwitchBotPlugin : public ILightSensorPlugin {
public:
    const char* GetPluginName() const override {
        return "SwitchBotLightSensor";
    }

    const char* GetPluginVersion() const override {
        return "1.0.0";
    }

    std::unique_ptr<ILightSensor> CreateSensor(
        const json& config
    ) override {
        return std::make_unique<SwitchBotLightSensor>(
            config["token"].get<std::string>(),
            config["deviceId"].get<std::string>()
        );
    }
};

// プラグインのエクスポート
extern "C" {
    PLUGIN_API ILightSensorPlugin* CreatePlugin() {
        return new SwitchBotPlugin();
    }

    PLUGIN_API void DestroyPlugin(ILightSensorPlugin* plugin) {
        delete plugin;
    }
}
```

## 実装手順

1. コアシステムの変更
   - プラグインローダーの実装
   - BrightnessDaemonの修正
   - 設定システムの更新

2. プラグインシステムの実装
   - プラグインインターフェースの定義
   - ビルドシステムの設定
   - プラグインのパッケージング方法の確立

3. 既存コードの移行
   - SwitchBotLightSensorをプラグインとして再実装
   - DummyLightSensorをプラグインとして再実装

4. テスト
   - プラグインローダーのユニットテスト
   - プラグイン読み込みの結合テスト
   - エラーケースのテスト

## セキュリティ考慮事項
1. プラグインの署名検証
2. プラグインの権限管理
3. プラグイン間の分離
4. 設定ファイルのバリデーション

## エラーハンドリング
1. プラグイン読み込みエラー
2. 設定エラー
3. プラグイン実行時エラー
4. リソース解放の保証

## 将来の拡張性
1. プラグインの動的な更新
2. プラグインの依存関係管理
3. プラグインの設定UI
4. プラグインのマーケットプレイス
