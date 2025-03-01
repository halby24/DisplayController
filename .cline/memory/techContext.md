# 技術コンテキスト

## 開発環境

### 必須コンポーネント
- Visual Studio 2022
- Windows SDK 10.0
- CMake 3.29.8
- C++ 17対応コンパイラ
- vcpkg（依存関係管理）

### ビルド環境
```bash
# vcpkgのセットアップ
vcpkg install curl:x64-windows
vcpkg install openssl:x64-windows
vcpkg install nlohmann-json:x64-windows
vcpkg install gtest:x64-windows

# ビルド手順
cmake -B build
cmake --build build
```

## 使用API・ライブラリ

### 1. Windows API
```cpp
// Physical Monitor API
GetNumberOfPhysicalMonitorsFromHMONITOR
GetPhysicalMonitorsFromHMONITOR
GetMonitorBrightness
SetMonitorBrightness

// Display Device API
EnumDisplayMonitors
GetMonitorInfo
```

### 2. libcurl
```cpp
// HTTP通信
CURL* curl = curl_easy_init();
curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
```

### 3. nlohmann/json
```cpp
// JSON処理
using json = nlohmann::json;
json config = json::parse(configStr);
std::string token = config["token"];
```

### 4. GoogleTest
```cpp
// テストフレームワーク
TEST(SwitchBotTest, GetLightLevel) {
    MockHttpClient client;
    EXPECT_CALL(client, Get(_))
        .WillOnce(Return("{"light": 50}"));
}
```

## 依存関係

### 1. 必須ライブラリ
- Windows SDK標準ライブラリ
- libcurl: HTTP通信
- OpenSSL: セキュア通信
- nlohmann/json: JSON処理
- GoogleTest: テストフレームワーク

### 2. プロジェクト内部依存
- MonitorController → Windows API
- SwitchBotLightSensor → libcurl, nlohmann/json
- テストコード → GoogleTest

## システム要件

### 1. ハードウェア要件
- OS: Windows 10/11
- アーキテクチャ: x64
- DDC/CI対応モニター
- インターネット接続（SwitchBot API用）

### 2. 権限要件
- モニター制御: 一部機能で管理者権限が必要
- ネットワーク: アウトバウンド通信許可
- ファイルシステム: 設定ファイル読み書き権限

## 技術的制約

### 1. ハードウェア制約
- DDC/CI対応モニター必須
- モニター応答遅延の考慮
- 非対応モニターのスキップ処理
- センサーデータ更新頻度の制限

### 2. セキュリティ制約
- 管理者権限チェック
- APIトークンの安全な保管
- 入力値の検証
- エラー情報の制限

### 3. ネットワーク制約
- HTTP通信タイムアウト設定
- 接続エラーのリトライ処理
- レート制限への対応

## デバッグ・テスト環境

### 1. デバッグツール
- Visual Studio デバッガー
- Windows Event Viewer
- DebugView
- libcurlデバッグオプション

### 2. テスト環境
- 単一モニター構成
- 複数モニター構成（2-3台）
- 異なるメーカーのモニター
- モックSwitchBotデバイス

## パフォーマンス要件

### 1. 応答時間
- コマンド実行: 1秒以内
- モニター操作: 0.5秒以内
- センサー値取得: 3秒以内
- エラー応答: 即時

### 2. リソース使用
- メモリ使用量: 最小限
- CPU使用率: 低負荷
- ネットワーク: 最小限の通信
- ディスク操作: 設定ファイルのみ

## エラー処理

### 1. ログ出力
- 標準エラー出力使用
- エラーコードと説明
- デバッグ情報（開発時のみ）
- センサー通信エラーのログ

### 2. リカバリー戦略
- 操作失敗時の再試行
- センサー値取得エラー時のフォールバック
- クリーンアップ処理
- 安全な終了処理
