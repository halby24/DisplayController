# SwitchBot Light Sensor DLLの統合手順

## 概要
SwitchBot Light Sensor DLLをDisplayControllerプロジェクトに統合するための手順を説明します。

## 前提条件
1. 必要なライブラリ
   - libcurl
   - OpenSSL
   - nlohmann/json
   - GoogleTest (テスト用)

2. 環境設定
   - Visual Studio 2022
   - CMake 3.15以上
   - vcpkg (依存ライブラリの管理)

## 統合手順

1. ビルド設定
   ```bash
   # vcpkgで依存ライブラリをインストール
   vcpkg install curl:x64-windows
   vcpkg install openssl:x64-windows
   vcpkg install nlohmann-json:x64-windows
   vcpkg install gtest:x64-windows
   ```

2. プロジェクトの設定
   - DisplayControllerのCMakeLists.txtにSwitchBotLightSensorを追加
   - インクルードパスの設定
   - リンク設定の追加

3. 設定ファイルの準備
   ```json
   {
       "token": "your-api-token",
       "deviceId": "your-device-id"
   }
   ```

4. DLLの使用方法
   ```cpp
   #include "SwitchBotLightSensor.h"

   // センサーのインスタンス化
   auto sensor = std::make_unique<SwitchBotLightSensor>(token, deviceId);

   // 照度値の取得
   int lightLevel = sensor->GetLightLevel();
   ```

## エラーハンドリング
- ConfigurationException: 設定エラー
- AuthenticationException: 認証エラー
- DeviceNotFoundException: デバイスが見つからない
- SwitchBotException: その他のエラー

## テスト
1. ユニットテストの実行
   ```bash
   ctest -C Debug
   ```

2. 結合テスト
   - DisplayControllerとの統合テスト
   - 実際のSwitchBotデバイスとの通信テスト

## トラブルシューティング
1. DLLロードエラー
   - 依存DLLの配置確認
   - パス設定の確認

2. 通信エラー
   - ネットワーク接続の確認
   - APIトークンの有効性確認
   - デバイスIDの確認

3. ビルドエラー
   - 依存ライブラリの確認
   - コンパイラ設定の確認

## 注意事項
- APIトークンの安全な管理
- エラーログの適切な処理
- パフォーマンスの監視
