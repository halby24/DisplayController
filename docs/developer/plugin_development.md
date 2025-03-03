# プラグイン開発ガイド

このガイドでは、DisplayController用の新しい光センサープラグインの開発方法について説明します。

## プラグインの基本構造

新しいプラグインを作成するには、以下のファイル構造を推奨します：

```
plugins/YourSensorName/
├── CMakeLists.txt
├── include/
│   ├── YourSensorPlugin.h
│   └── YourSensor.h
└── src/
    ├── YourSensorPlugin.cpp
    └── YourSensor.cpp
```

## 必要なインターフェースの実装

### 1. プラグインクラス

```cpp
// YourSensorPlugin.h
#include "ILightSensorPlugin.h"

class YourSensorPlugin : public ILightSensorPlugin {
public:
    const char* GetPluginName() const override;
    const char* GetPluginVersion() const override;
    std::unique_ptr<ILightSensor> CreateSensor(
        const json& config
    ) override;
};

// エクスポート関数
extern "C" {
    PLUGIN_API ILightSensorPlugin* CreatePlugin();
    PLUGIN_API void DestroyPlugin(ILightSensorPlugin* plugin);
}
```

### 2. センサークラス

```cpp
// YourSensor.h
#include "ILightSensor.h"

class YourSensor : public ILightSensor {
public:
    explicit YourSensor(const json& config);
    float GetLightLevel() override;
    bool IsConnected() override;
private:
    // プラグイン固有の実装
};
```

## CMakeの設定

```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.15)
project(YourSensorPlugin)

# DLLとしてビルド
add_library(${PROJECT_NAME} SHARED
    src/YourSensorPlugin.cpp
    src/YourSensor.cpp
)

# インクルードディレクトリの設定
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src  # コアのヘッダーファイル
)

# PLUGIN_APIマクロの定義
target_compile_definitions(${PROJECT_NAME} PRIVATE
    PLUGIN_EXPORTS
)
```

## プラグインの実装例

### プラグインクラスの実装

```cpp
// YourSensorPlugin.cpp
#include "YourSensorPlugin.h"
#include "YourSensor.h"

const char* YourSensorPlugin::GetPluginName() const {
    return "YourSensorName";
}

const char* YourSensorPlugin::GetPluginVersion() const {
    return "1.0.0";
}

std::unique_ptr<ILightSensor> YourSensorPlugin::CreateSensor(
    const json& config
) {
    return std::make_unique<YourSensor>(config);
}

// エクスポート関数の実装
extern "C" {
    PLUGIN_API ILightSensorPlugin* CreatePlugin() {
        return new YourSensorPlugin();
    }

    PLUGIN_API void DestroyPlugin(ILightSensorPlugin* plugin) {
        delete plugin;
    }
}
```

### センサークラスの実装

```cpp
// YourSensor.cpp
#include "YourSensor.h"

YourSensor::YourSensor(const json& config) {
    // 設定の解析と初期化
}

float YourSensor::GetLightLevel() {
    // センサーから明るさレベルを取得
    // 0.0 - 1.0の範囲で返す
}

bool YourSensor::IsConnected() {
    // センサーの接続状態を確認
}
```

## 設定ファイルの形式

プラグインの設定は以下の形式でconfig.jsonに記述します：

```json
{
    "sensor": {
        "plugin": "YourSensorName",
        "config": {
            // プラグイン固有の設定
            "option1": "value1",
            "option2": "value2"
        }
    }
}
```

## エラーハンドリング

1. 設定エラー
   - 必須パラメータの欠落
   - 無効な値
   - 型の不一致

```cpp
void ValidateConfig(const json& config) {
    if (!config.contains("required_param")) {
        throw std::runtime_error("Missing required parameter");
    }
    // その他の検証
}
```

2. 実行時エラー
   - センサー接続エラー
   - 読み取りエラー
   - タイムアウト

```cpp
float GetLightLevel() {
    try {
        // センサーからの読み取り処理
    } catch (const SensorError& e) {
        // エラーログの記録
        return 0.0f; // デフォルト値
    }
}
```

## テスト

1. ユニットテスト
   ```cpp
   TEST_CASE("YourSensor basic functionality") {
       json config = {{"param", "value"}};
       YourSensor sensor(config);

       REQUIRE(sensor.IsConnected());
       REQUIRE(sensor.GetLightLevel() >= 0.0f);
       REQUIRE(sensor.GetLightLevel() <= 1.0f);
   }
   ```

2. モックの使用
   ```cpp
   class MockSensor : public YourSensor {
   public:
       MOCK_METHOD(float, GetLightLevel, (), (override));
       MOCK_METHOD(bool, IsConnected, (), (override));
   };
   ```

## ベストプラクティス

1. リソース管理
   - RAIIの使用
   - スマートポインタの活用
   - 適切なクリーンアップ

2. スレッド安全性
   - 共有リソースの保護
   - スレッドセーフな初期化
   - 適切な同期機構の使用

3. パフォーマンス
   - 効率的なポーリング
   - キャッシングの活用
   - リソースの再利用

4. エラー回復
   - グレースフルな失敗
   - 自動再接続
   - 適切なフォールバック

## デバッグとトラブルシューティング

1. ログ出力
   ```cpp
   #include <spdlog/spdlog.h>

   void YourSensor::Initialize() {
       spdlog::debug("Initializing sensor with config: {}", config.dump());
       // 初期化処理
   }
   ```

2. デバッグビルド
   ```cmake
   if(CMAKE_BUILD_TYPE STREQUAL "Debug")
       target_compile_definitions(${PROJECT_NAME} PRIVATE DEBUG_MODE)
   endif()
   ```

## 配布とインストール

1. プラグインのパッケージング
   - 必要なDLLの同梱
   - 依存関係の管理
   - ドキュメントの準備

2. インストール手順
   - プラグインディレクトリへのコピー
   - 設定ファイルの配置
   - 権限の設定
