# SwitchBot設定ファイル設計

## 概要
SwitchBotデバイスとの通信に必要な認証情報を安全に管理するための設定ファイル設計について説明します。

## 設定ファイルの仕様

### 1. ファイル形式
- JSON形式を採用
- UTF-8エンコーディング
- 人間が読み書き可能な形式

### 2. ファイルの場所
- Windows: `%APPDATA%\DisplayController\config.json`
- 理由：
  * ユーザー固有の設定を格納するための標準的な場所
  * システム全体ではなくユーザー単位での設定が適切
  * アプリケーションの設定ファイルを集中管理可能

### 3. ファイル構造
```json
{
  "switchbot": {
    "token": "your-api-token",
    "devices": [
      {
        "id": "device-id",
        "name": "Light Sensor 1",
        "type": "lightSensor"
      }
    ]
  }
}
```

### 4. セキュリティ考慮事項
- ファイルパーミッション：現在のユーザーのみ読み書き可能
- APIトークンの保護：平文で保存するが、ファイルパーミッションで保護
- 初回実行時：
  * 設定ディレクトリが存在しない場合は作成
  * 適切なパーミッションでファイルを作成
  * ユーザーに認証情報の入力を促す

### 5. エラーハンドリング
- ファイルが存在しない場合：
  * 初期設定ウィザードを表示
  * デフォルト設定でファイルを作成
- ファイルの読み取りエラー：
  * 適切なエラーメッセージを表示
  * 必要に応じて設定ファイルの再作成を提案
- 不正な形式：
  * JSONパースエラーを検出
  * ユーザーに設定の確認を促す

### 6. 設定の検証
- 起動時に以下を確認：
  * 必須フィールドの存在
  * APIトークンの形式
  * デバイスIDの形式
  * 設定値の妥当性

## 実装計画

1. 設定管理クラスの作成
```cpp
class ConfigManager {
public:
    static ConfigManager& Instance();

    // 設定の読み込み
    void Load();

    // 設定の保存
    void Save();

    // 設定値の取得
    std::string GetSwitchBotToken() const;
    std::string GetDeviceId(const std::string& name) const;

    // 設定値の設定
    void SetSwitchBotToken(const std::string& token);
    void AddDevice(const std::string& id, const std::string& name, const std::string& type);

private:
    ConfigManager() = default;
    std::string GetConfigPath() const;
    void ValidateConfig() const;
    void CreateDefaultConfig();
};
```

2. 実装の優先順位
   1. 基本的な設定ファイルの読み書き機能
   2. 設定の検証機能
   3. エラーハンドリング
   4. 初期設定ウィザード

## 今後の拡張性

1. 追加設定項目
   - デバイスの位置情報
   - ポーリング間隔
   - ログレベル
   - バックアップ設定

2. 機能拡張
   - 設定のインポート/エクスポート
   - 複数プロファイルのサポート
   - 設定変更の自動検知と再読み込み
