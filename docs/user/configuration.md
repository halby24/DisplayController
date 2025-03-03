# 設定ガイド

## 設定ファイル（config.json）
設定ファイルは以下の形式のJSONファイルです：

```json
{
  "monitors": [
    {
      "name": "DELL P2419H",
      "brightness_range": {
        "min": 0,
        "max": 100
      }
    }
  ],
  "plugins": {
    "SwitchBotLightSensor": {
      "device_id": "YOUR_DEVICE_ID",
      "token": "YOUR_TOKEN"
    }
  },
  "brightness_control": {
    "update_interval_ms": 5000,
    "smoothing_factor": 0.3
  }
}
```

## 設定項目の説明

### monitors
モニターの設定を指定します。

- `name`: モニターの名前（必須）
- `brightness_range`: 明るさの調整範囲
  - `min`: 最小値（0-100）
  - `max`: 最大値（0-100）

### plugins
使用するプラグインの設定を指定します。

#### SwitchBotLightSensor
SwitchBotライトセンサーを使用する場合の設定：

- `device_id`: SwitchBotデバイスID（必須）
- `token`: SwitchBotアクセストークン（必須）

### brightness_control
明るさ制御の動作設定：

- `update_interval_ms`: 明るさ更新間隔（ミリ秒）
- `smoothing_factor`: 明るさ変更の滑らかさ（0.0-1.0）
  - 0.0に近いほど滑らか
  - 1.0に近いほど即座に反映

## サンプル設定
完全な設定例は[samples/config.json.sample](../samples/config.json.sample)を参照してください。

## 設定の検証
設定ファイルの構文や値が正しいことを確認するため、プログラム起動時に自動的に検証が行われます。エラーがある場合は、詳細なエラーメッセージが表示されます。

一般的なエラーケースとその対処方法については、[samples/config_error_cases.json.sample](../samples/config_error_cases.json.sample)を参照してください。
