# SwitchBot設定のセットアップ手順

## 概要
BrightnessDaemonがSwitchBotの照度センサーと連携するために必要な設定手順を説明します。

## 1. SwitchBot APIトークンの取得

1. SwitchBotアプリをインストール
2. アプリにログイン
3. プロフィール → App Version を10回タップ
4. 開発者オプションが有効化
5. 開発者オプションからAPIトークンを取得

## 2. デバイスIDの取得

1. SwitchBotアプリで対象の照度センサーを選択
2. デバイス設定を開く
3. デバイス情報からデバイスIDをコピー

## 3. 設定ファイルの作成

1. 以下のパスに設定ファイルを作成:
   ```
   %APPDATA%\DisplayController\config.json
   ```

2. 以下の形式で設定を記述:
   ```json
   {
     "switchbot": {
       "token": "YOUR_SWITCHBOT_API_TOKEN",
       "devices": [
         {
           "id": "YOUR_DEVICE_ID",
           "name": "Light Sensor 1",
           "type": "lightSensor"
         }
       ]
     }
   }
   ```

   - `token`: SwitchBotアプリから取得したAPIトークン
   - `id`: デバイスのID
   - `name`: デバイスの識別名（任意の名前）
   - `type`: デバイスタイプ（"lightSensor"固定）

## 4. 設定の確認

1. BrightnessDaemonを起動
2. タスクトレイアイコンが表示されることを確認
3. 「同期を開始」をクリックして動作確認

## トラブルシューティング

### エラー: "設定の読み込みに失敗しました"
- 設定ファイルが正しい場所に存在するか確認
- JSONの形式が正しいか確認
- ファイルの文字コードがUTF-8であることを確認

### エラー: "SwitchBotの初期化に失敗しました"
- APIトークンが正しいか確認
- デバイスIDが正しいか確認
- インターネット接続を確認

### エラー: "Device not found"
- デバイスIDが正しいか確認
- デバイスがオンラインであることを確認
- SwitchBotアプリでデバイスが正常に動作することを確認

## 注意事項

- 設定ファイルには機密情報が含まれるため、適切なアクセス権限を設定してください
- APIトークンは第三者に開示しないでください
- 設定ファイルのバックアップを作成することを推奨します
