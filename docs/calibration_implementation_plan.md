# 輝度キャリブレーション実装計画

## 概要
ライトセンサーごとの入力値のばらつきを補正するため、デバイス単位でのキャリブレーション機能を実装します。

## フェーズ1: 設定ファイル拡張

### ConfigManager拡張
```cpp
// ConfigManager.h
struct CalibrationSettings {
    int minRawValue = 0;      // デフォルト値
    int maxRawValue = 1000;   // デフォルト値

    bool IsValid() const {
        return minRawValue >= 0 && maxRawValue > minRawValue;
    }
};

class ConfigManager {
public:
    // 既存のメソッドは変更なし

    // 新規追加メソッド
    CalibrationSettings GetDeviceCalibration(const std::string& deviceName) const;
    void SetDeviceCalibration(const std::string& deviceName, const CalibrationSettings& settings);
};
```

### 設定ファイル形式の拡張
```json
{
  "switchbot": {
    "devices": [
      {
        "id": "device_id",
        "name": "Light Sensor 1",
        "type": "Light Sensor",
        "calibration": {
          "min_raw_value": 100,
          "max_raw_value": 800
        }
      }
    ]
  }
}
```

## フェーズ2: ConfigManager実装の更新

### 実装手順
1. ValidateConfig()の拡張
   - キャリブレーション設定の存在確認
   - 値の範囲チェック（min < max）
   - 型チェック（数値であること）

2. CreateDefaultConfig()の更新
   - デフォルトのキャリブレーション設定を追加

3. GetDeviceCalibration()の実装
   - デバイス名からキャリブレーション設定を取得
   - 設定が存在しない場合はデフォルト値を返す
   - 不正な値の場合は例外をスロー

4. SetDeviceCalibration()の実装
   - 入力値の検証
   - 設定の保存
   - 自動的なファイル更新

## フェーズ3: SwitchBotLightSensor更新

### 実装手順
1. キャリブレーション設定の初期化
   ```cpp
   class SwitchBotLightSensor {
   private:
       CalibrationSettings m_calibration;

       void InitializeCalibration() {
           try {
               m_calibration = m_config.GetDeviceCalibration(m_deviceName);
           } catch (const ConfigException& e) {
               // デフォルト値を使用
               StringUtils::OutputWarningMessage(
                   "キャリブレーション設定が見つかりません。デフォルト値を使用します。"
               );
           }
       }
   };
   ```

2. NormalizeLightLevel()の更新
   ```cpp
   int NormalizeLightLevel(int rawLevel) {
       // キャリブレーション設定に基づいて0-100にマッピング
       int clampedRaw = std::clamp(
           rawLevel,
           m_calibration.minRawValue,
           m_calibration.maxRawValue
       );

       return static_cast<int>(
           (static_cast<double>(clampedRaw - m_calibration.minRawValue) /
           (m_calibration.maxRawValue - m_calibration.minRawValue)) * 100
       );
   }
   ```

## フェーズ4: テスト実装

### テストケース
1. 設定ファイルの検証
   - キャリブレーション設定の読み込み
   - デフォルト値の確認
   - 不正な値の検出

2. 値の正規化
   - 最小値→0%の確認
   - 最大値→100%の確認
   - 中間値の正確な変換
   - 範囲外の値のクランプ

3. エラー処理
   - 不正な設定値の検出
   - 設定不在時のデフォルト動作
   - 例外処理の確認

## 移行計画

1. 既存の設定ファイルの互換性維持
   - キャリブレーション設定が存在しない場合はデフォルト値を使用
   - 設定ファイルの自動アップグレード

2. デプロイ手順
   - 設定ファイルのバックアップ
   - 新機能の段階的な有効化
   - ログ出力の強化（一時的）

## 検証項目

1. 機能検証
   - 異なるセンサーでの動作確認
   - 設定値の永続化
   - エラー処理の動作

2. パフォーマンス確認
   - 設定ファイル読み込み時間
   - メモリ使用量
   - CPU使用率

3. 安定性確認
   - 長時間動作での安定性
   - エッジケースでの動作
   - エラー回復の確認
