# 輝度キャリブレーション設計

## 概要
ライトセンサーごとの入力値のばらつきを補正するため、デバイス単位で想定される値の最大・最小値をconfigで指定し、それを0～100にリマップする機能を追加します。

## 設定ファイル変更
config.jsonの構造を以下のように拡張します：

```json
{
  "switchbot": {
    "token": "YOUR_SWITCHBOT_API_TOKEN",
    "secret": "YOUR_SWITCHBOT_API_SECRET",
    "devices": [
      {
        "id": "YOUR_DEVICE_ID",
        "name": "Light Sensor 1",
        "type": "Light Sensor",
        "description": "リビングの照度センサー",
        "calibration": {
          "min_raw_value": 100,  // デバイスから期待される最小生値
          "max_raw_value": 800   // デバイスから期待される最大生値
        }
      }
    ]
  }
}
```

## 実装変更

### ConfigManager拡張
- デバイスごとのキャリブレーション設定を取得するメソッドを追加
```cpp
struct CalibrationSettings {
    int minRawValue;
    int maxRawValue;
};

class ConfigManager {
public:
    CalibrationSettings GetDeviceCalibration(const std::string& deviceName);
};
```

### SwitchBotLightSensor変更
- NormalizeLightLevel()メソッドを拡張し、キャリブレーション設定を使用
```cpp
class SwitchBotLightSensor {
private:
    CalibrationSettings m_calibration;

    int NormalizeLightLevel(int rawLevel) {
        // キャリブレーション設定に基づいて0-100にマッピング
        int clampedRaw = std::clamp(rawLevel,
            m_calibration.minRawValue,
            m_calibration.maxRawValue);

        return static_cast<int>(
            (static_cast<double>(clampedRaw - m_calibration.minRawValue) /
            (m_calibration.maxRawValue - m_calibration.minRawValue)) * 100
        );
    }
};
```

## エラー処理
- キャリブレーション設定が存在しない場合のフォールバック処理
- 最小値が最大値より大きい場合の検証
- 生値が想定範囲を大きく外れた場合の警告ログ

## 移行計画
1. config.jsonスキーマの更新
2. ConfigManagerの拡張
3. SwitchBotLightSensorの更新
4. テストケースの追加
5. ドキュメントの更新

## 検証項目
- 異なるキャリブレーション設定での正規化の動作確認
- 設定値の境界値テスト
- エラーケースの確認
- パフォーマンスへの影響確認
