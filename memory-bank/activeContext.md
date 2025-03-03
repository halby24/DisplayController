# アクティブコンテキスト

## 現在の作業：プラグイン設定構造の再設計

### 概要
SwitchBotがプラグインの一つとなったことに伴い、設定ファイルの構造を見直し、より適切なプラグインベースの構造に移行します。

### 新しい設定構造
```json
{
  "plugins": {
    "switchbot": {
      "global_settings": {
        "token": "YOUR_SWITCHBOT_API_TOKEN",
        "secret": "YOUR_SWITCHBOT_API_SECRET"
      },
      "devices": [
        {
          "id": "YOUR_DEVICE_ID",
          "name": "Light Sensor 1",
          "type": "Light Sensor",
          "description": "リビングの照度センサー",
          "calibration": {
            "min_raw_value": 100,
            "max_raw_value": 800
          }
        }
      ]
    }
  }
}
```

### 実装計画
1. ConfigManagerの拡張
   - プラグイン設定へのアクセス方法の追加
   - 後方互換性のための移行ロジック実装

2. プラグインの更新
   - 設定読み込みロジックの更新
   - エラーハンドリングの強化

3. テスト計画
   - 新しい設定構造のバリデーション
   - 移行ロジックのテスト
   - エラーケースの検証

### 関連ドキュメント
- docs/plugin_config_restructure.md: 詳細な設計ドキュメント
- docs/plugin_architecture_design.md: プラグインアーキテクチャの基本設計
- docs/switchbot_config_design.md: 既存のSwitchBot設定設計

### 次のステップ
1. Codeモードでの実装開始
2. ConfigManagerの更新
3. プラグイン設定読み込みロジックの修正
4. テストケースの作成と実行
