# 輝度自動調整デーモン設計書

## 概要
SwitchBotセンサーから取得した照度データに基づいて、ディスプレイの輝度を自動調整する常駐アプリケーション。

## コンポーネント構成

### 1. タスクトレイアプリケーション（BrightnessDaemon）
- Windows通知領域に常駐
- アプリケーションの状態表示
- 設定メニューの提供
- 終了処理の制御

### 2. 照度センサーインターフェース（ILightSensor）
```cpp
class ILightSensor {
public:
    virtual ~ILightSensor() = default;
    virtual int GetLightLevel() = 0;  // 0-100の範囲で照度を返す
};

// 開発用ダミー実装
class DummyLightSensor : public ILightSensor {
public:
    int GetLightLevel() override;  // ダミーデータを返す
};
```

### 3. 輝度同期マネージャー（BrightnessManager）
```cpp
class BrightnessManager {
public:
    BrightnessManager(std::unique_ptr<ILightSensor> sensor);
    void StartSync();  // 同期処理の開始
    void StopSync();   // 同期処理の停止
    void UpdateBrightness();  // 手動での更新トリガー

private:
    std::unique_ptr<ILightSensor> m_sensor;
    std::unique_ptr<MonitorController> m_controller;
    bool m_isRunning;
    std::thread m_syncThread;

    void SyncLoop();  // 定期的な同期処理
    int CalculateBrightness(int lightLevel);  // 照度から輝度値を計算
};
```

## 処理フロー

1. アプリケーション起動
   - タスクトレイアイコンの初期化
   - BrightnessManagerの初期化（ダミーセンサーを使用）
   - 同期処理の開始

2. 定期的な輝度同期
   - 5秒ごとに照度を取得
   - 照度に基づいて適切な輝度を計算
   - MonitorControllerを使用して輝度を設定

3. ユーザーインタラクション
   - タスクトレイアイコンの右クリックでメニュー表示
   - 同期の一時停止/再開
   - アプリケーションの終了

4. 終了処理
   - 同期処理の停止
   - タスクトレイアイコンの削除
   - リソースの解放

## 設定項目

- 同期間隔（デフォルト: 5秒）
- 最小輝度（デフォルト: 20%）
- 最大輝度（デフォルト: 100%）
- 照度-輝度変換カーブ調整

## エラーハンドリング

- センサー読み取りエラー
  - エラーログの記録
  - 前回の有効な値を使用
  - 一定回数以上のエラーでユーザーに通知

- モニター制御エラー
  - エラーログの記録
  - ユーザーに通知
  - 自動リトライ

## 将来の拡張性

1. 実際のSwitchBotセンサー実装の追加
2. 複数のセンサーサポート
3. 時間帯による輝度調整ルール
4. ユーザー設定のGUI
5. 輝度履歴のログ機能

## 実装フェーズ

### フェーズ1（現在の目標）
- タスクトレイアプリケーションの基本機能
- ダミーセンサーの実装
- 基本的な輝度同期機能

### フェーズ2
- SwitchBot APIの実装
- 設定保存機能
- 詳細なエラーハンドリング

### フェーズ3
- GUIベースの設定画面
- 高度な輝度制御ルール
- ログ機能
