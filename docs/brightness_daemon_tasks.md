# BrightnessDaemon 設定拡張

## 概要
BrightnessDaemonのAPIリクエスト間隔を設定可能にするための変更計画です。

## 設定ファイル変更

### 新しい設定構造
```json
{
  "switchbot": {
    "token": "YOUR_SWITCHBOT_API_TOKEN",
    "secret": "YOUR_SWITCHBOT_API_SECRET",
    "devices": [
      {
        "id": "YOUR_DEVICE_ID",
        "name": "Light Sensor 1",
        "type": "lightSensor",
        "description": "リビングの照度センサー"
      }
    ]
  },
  "brightness_daemon": {
    "update_interval_ms": 5000,  // デフォルト値: 5000ms (5秒)
    "min_brightness": 0,         // デフォルト値: 0
    "max_brightness": 100        // デフォルト値: 100
  }
}
```

## 実装計画

### 1. ConfigManager拡張
- `GetUpdateInterval()` メソッドの追加
  - デフォルト値: 5000ms
  - 設定が存在しない場合はデフォルト値を返す
- `SetUpdateInterval(int interval_ms)` メソッドの追加
- ValidateConfig()の更新
  - brightness_daemon設定の検証を追加

### 2. BrightnessDaemon更新
- 初期化時にConfigManagerから更新間隔を読み込む
- BrightnessManagerに設定を適用

### 3. サンプル設定・ドキュメント更新
- config.json.sampleの更新
- 設定に関するドキュメントの更新

## テスト計画
1. 設定ファイルからの正しい読み込み
2. デフォルト値の適用
3. 無効な値のバリデーション
4. 実際の動作確認

## 注意点
- 既存の設定ファイルとの後方互換性の維持
- 無効な値のバリデーション（負の値、極端に小さい値など）
- 設定変更時の動的な反映
