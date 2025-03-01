# SwitchBot Light Sensor DLL設計

## 概要
SwitchBot APIを使用して照度センサーの値を取得するDLLを作成します。このDLLはILightSensorインターフェースを実装し、DisplayControllerに組み込んで使用します。

## 要件
1. ILightSensorインターフェースの実装
   - GetLightLevel()メソッドで0-100の範囲の照度値を返す
2. SwitchBot APIとの通信
   - トークン認証
   - HTTPリクエストの送信
   - JSONレスポンスの解析
3. エラーハンドリング
   - 通信エラー
   - 認証エラー
   - データ解析エラー
4. 設定管理
   - APIトークン
   - デバイスID

## API仕様

### エンドポイント
```
GET https://api.switch-bot.com/v1.1/devices/{deviceId}/status
```

### 認証ヘッダー
```
Authorization: Bearer {token}
t: {timestamp}
sign: {signature}
nonce: {nonce}
```

### レスポンス形式
```json
{
    "statusCode": 100,
    "body": {
        "deviceId": "500291B269BE",
        "deviceType": "WoLight",
        "brightness": 50
    },
    "message": "success"
}
```

## クラス設計

### SwitchBotLightSensor
```cpp
class SWITCHBOT_API SwitchBotLightSensor : public ILightSensor {
private:
    std::string m_token;
    std::string m_deviceId;
    std::unique_ptr<HttpClient> m_httpClient;

public:
    SwitchBotLightSensor(const std::string& token, const std::string& deviceId);
    virtual ~SwitchBotLightSensor() override;

    virtual int GetLightLevel() override;

private:
    json GetDeviceStatus();
    int NormalizeLightLevel(int rawLevel);
    std::string GenerateSignature();
    std::string GenerateNonce();
};
```

### HttpClient
```cpp
class HttpClient {
public:
    HttpClient(const std::string& token);
    ~HttpClient();

    json Get(const std::string& endpoint);

private:
    std::string m_token;
    void SetupHeaders(void* request, const std::string& signature, const std::string& nonce);
    std::string GetTimestamp();
};
```

### 例外クラス
```cpp
class SwitchBotException : public std::runtime_error {
public:
    explicit SwitchBotException(const std::string& message);
    int GetErrorCode() const;
private:
    int m_errorCode;
};
```

## プロジェクト構造
```
SwitchBotLightSensor/
├── include/
│   ├── SwitchBotLightSensor.h
│   ├── HttpClient.h
│   └── SwitchBotException.h
├── src/
│   ├── SwitchBotLightSensor.cpp
│   ├── HttpClient.cpp
│   └── SwitchBotException.cpp
├── test/
│   ├── SwitchBotLightSensorTest.cpp
│   └── HttpClientTest.cpp
└── CMakeLists.txt
```

## 依存関係
- nlohmann/json: JSONパース用
- libcurl: HTTP通信用
- Windows SDK: DLL作成用
- OpenSSL: 署名生成用

## エラー処理
- SwitchBotApiException: API通信に関するエラー
  - 認証エラー (401)
  - デバイスが見つからない (404)
  - サーバーエラー (500)
- ConfigurationException: 設定に関するエラー
  - トークンが無効
  - デバイスIDが無効
- DeviceNotFoundException: デバイスが見つからない場合のエラー

## 設定ファイル (switchbot_config.json)
```json
{
    "token": "your-api-token",
    "deviceId": "your-device-id",
    "retryCount": 3,
    "retryInterval": 1000
}
```

## ビルド設定
- Visual Studio 2022
- C++17以上
- DLLエクスポート設定
  - SWITCHBOT_API マクロの定義
  - dllexport/dllimport の適切な使用
- 依存ライブラリの静的リンク

## テスト計画
1. ユニットテスト
   - モックHTTPクライアントを使用したテスト
   - 値の正規化テスト
   - エラーケースのテスト
   - 署名生成のテスト
   - 設定ファイル読み込みテスト

2. 結合テスト
   - 実際のSwitchBot APIとの通信テスト
   - DisplayControllerとの統合テスト
   - エラー回復テスト

## セキュリティ考慮事項
- APIトークンの安全な保管
- HTTPS通信の使用
- エラーメッセージでの機密情報の非表示
- 署名検証の実装
- 通信タイムアウトの設定

## 実装手順
1. プロジェクト構造の作成
   - Visual Studioプロジェクトの設定
   - CMakeファイルの作成
2. 依存ライブラリの導入
   - vcpkgを使用したライブラリ管理
3. HttpClientクラスの実装
   - CURL初期化
   - ヘッダー設定
   - エラーハンドリング
4. SwitchBotLightSensorクラスの実装
   - ILightSensorインターフェースの実装
   - API通信の実装
   - 値の正規化
5. テストコードの作成
   - GoogleTestの設定
   - モッククラスの作成
6. DisplayControllerとの統合
   - DLLのエクスポート
   - インターフェースの確認

## パフォーマンス考慮事項
- キャッシュの実装（短時間での複数回呼び出し対策）
- 接続プールの使用
- エラー時のバックオフ戦略
- メモリリークの防止
