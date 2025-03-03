# API リファレンス

このドキュメントでは、DisplayControllerの主要なクラスとインターフェースのAPI仕様を説明します。

## コアインターフェース

### ILightSensor

環境光センサーの基本インターフェース。

```cpp
class ILightSensor {
public:
    virtual ~ILightSensor() = default;

    /**
     * @brief 現在の環境光レベルを取得
     * @return 0.0から1.0の範囲の環境光レベル（0.0が最も暗く、1.0が最も明るい）
     * @throws SensorError センサーからの読み取りに失敗した場合
     */
    virtual float GetLightLevel() = 0;

    /**
     * @brief センサーの接続状態を確認
     * @return true: 接続済み、false: 未接続
     */
    virtual bool IsConnected() = 0;
};
```

### ILightSensorPlugin

光センサープラグインの基本インターフェース。

```cpp
class ILightSensorPlugin {
public:
    virtual ~ILightSensorPlugin() = default;

    /**
     * @brief プラグイン名を取得
     * @return プラグインの一意な識別子
     */
    virtual const char* GetPluginName() const = 0;

    /**
     * @brief プラグインのバージョンを取得
     * @return バージョン文字列（セマンティックバージョニング推奨）
     */
    virtual const char* GetPluginVersion() const = 0;

    /**
     * @brief センサーインスタンスを作成
     * @param config プラグイン固有の設定（JSON形式）
     * @return ILightSensorのユニークポインタ
     * @throws std::runtime_error 設定が無効な場合
     */
    virtual std::unique_ptr<ILightSensor> CreateSensor(
        const json& config
    ) = 0;
};
```

## コアクラス

### BrightnessDaemon

システム全体を制御するメインクラス。

```cpp
class BrightnessDaemon {
public:
    /**
     * @brief デーモンを初期化
     * @param configPath 設定ファイルのパス
     * @throws ConfigError 設定ファイルの読み込みに失敗した場合
     */
    explicit BrightnessDaemon(const std::string& configPath);

    /**
     * @brief デーモンを開始
     * @return 0: 正常終了、1: エラー
     */
    int Run();

    /**
     * @brief デーモンを停止
     */
    void Stop();
};
```

### MonitorController

モニターの制御を担当するクラス。

```cpp
class MonitorController {
public:
    /**
     * @brief モニターの明るさを設定
     * @param brightness 0-100の範囲の明るさ値
     * @return true: 設定成功、false: 設定失敗
     */
    bool SetBrightness(int brightness);

    /**
     * @brief 現在の明るさを取得
     * @return 0-100の範囲の明るさ値
     * @throws MonitorError 取得に失敗した場合
     */
    int GetBrightness() const;

    /**
     * @brief モニター名を取得
     * @return モニターの識別名
     */
    std::string GetName() const;
};
```

### BrightnessManager

明るさ制御のロジックを担当するクラス。

```cpp
class BrightnessManager {
public:
    /**
     * @brief 明るさマネージャーを初期化
     * @param config 設定オブジェクト
     */
    explicit BrightnessManager(const Config& config);

    /**
     * @brief センサー値から明るさを計算
     * @param lightLevel センサーからの環境光レベル（0.0-1.0）
     * @return 設定すべき明るさ値（0-100）
     */
    int CalculateBrightness(float lightLevel);

    /**
     * @brief スムージングファクターを設定
     * @param factor 0.0-1.0の範囲の値（0.0: 最も滑らか、1.0: 即時反映）
     */
    void SetSmoothingFactor(float factor);
};
```

### ConfigManager

設定の管理を担当するクラス。

```cpp
class ConfigManager {
public:
    /**
     * @brief 設定を読み込み
     * @param path 設定ファイルのパス
     * @throws ConfigError 読み込みに失敗した場合
     */
    void LoadConfig(const std::string& path);

    /**
     * @brief プラグイン設定を取得
     * @param pluginName プラグイン名
     * @return プラグイン固有の設定
     * @throws std::out_of_range プラグインが見つからない場合
     */
    json GetPluginConfig(const std::string& pluginName) const;

    /**
     * @brief モニター設定を取得
     * @return モニター設定の配列
     */
    std::vector<MonitorConfig> GetMonitorConfigs() const;
};
```

### PluginLoader

プラグインの読み込みを担当するクラス。

```cpp
class PluginLoader {
public:
    /**
     * @brief プラグインを読み込み
     * @param path プラグインのパス
     * @return プラグインインスタンス
     * @throws PluginError 読み込みに失敗した場合
     */
    std::unique_ptr<ILightSensorPlugin> LoadPlugin(
        const std::string& path
    );

    /**
     * @brief 読み込み済みプラグインを取得
     * @return プラグイン名とインスタンスのマップ
     */
    const std::map<std::string, ILightSensorPlugin*>&
    GetLoadedPlugins() const;
};
```

## 例外クラス

### SensorError

センサー関連のエラーを表す例外クラス。

```cpp
class SensorError : public std::runtime_error {
public:
    explicit SensorError(const std::string& message);
    const char* what() const noexcept override;
};
```

### ConfigError

設定関連のエラーを表す例外クラス。

```cpp
class ConfigError : public std::runtime_error {
public:
    explicit ConfigError(const std::string& message);
    const char* what() const noexcept override;
};
```

### PluginError

プラグイン関連のエラーを表す例外クラス。

```cpp
class PluginError : public std::runtime_error {
public:
    explicit PluginError(const std::string& message);
    const char* what() const noexcept override;
};
```

## 設定構造体

### MonitorConfig

モニター設定を表す構造体。

```cpp
struct MonitorConfig {
    std::string name;           // モニター名
    int minBrightness;         // 最小明るさ（0-100）
    int maxBrightness;         // 最大明るさ（0-100）
    float smoothingFactor;     // スムージングファクター（0.0-1.0）
};
```

### BrightnessControlConfig

明るさ制御の設定を表す構造体。

```cpp
struct BrightnessControlConfig {
    int updateIntervalMs;      // 更新間隔（ミリ秒）
    float smoothingFactor;     // スムージングファクター
    int minBrightness;        // 最小明るさ
    int maxBrightness;        // 最大明るさ
};
