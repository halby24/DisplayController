# Windowsシステム輝度連携機能の設計

## 1. 概要

### 1.1 目的
Windowsシステムの輝度設定と連携し、システムの輝度変更をモニターに反映し、モニターの輝度変更をシステムに反映する双方向の同期を実現する。

### 1.2 主要機能
- システムの輝度設定の監視
- 輝度設定の双方向同期
- 設定の永続化と自動復元

## 2. アーキテクチャ拡張

### 2.1 新規コンポーネント
```cpp
class SystemBrightnessMonitor {
    // システムの輝度設定の監視
    void StartMonitoring();
    void StopMonitoring();
    void OnSystemBrightnessChanged(int newBrightness);
};

class BrightnessSync {
    // 輝度設定の同期管理
    void SyncToSystem(int brightness);
    void SyncToMonitor(int brightness);
    void SaveSyncState();
};
```

### 2.2 既存クラスの拡張
```cpp
class MonitorController {
    // 新規メソッド
    void SetSystemBrightness(int brightness);
    int GetSystemBrightness();
    void EnableAutoSync(bool enable);

    // 新規メンバー
    std::unique_ptr<SystemBrightnessMonitor> m_brightnessMonitor;
    std::unique_ptr<BrightnessSync> m_brightnessSync;
};
```

## 3. 実装詳細

### 3.1 システム輝度の監視
- WMIイベント監視を使用してシステムの輝度変更を検知
- Power Management APIを使用して電源設定の変更を監視
- 定期的なポーリングをバックアップとして実装

### 3.2 輝度同期の仕組み
1. システム → モニター
   - システム輝度変更イベントの検知
   - 対応するモニター輝度値の計算
   - モニター輝度の設定

2. モニター → システム
   - モニター輝度変更の検知
   - システム輝度値の計算
   - システム設定の更新

### 3.3 設定の永続化
- モニターごとの同期設定の保存
- 自動同期の有効/無効状態の管理
- 起動時の設定復元

## 4. エラーハンドリング

### 4.1 想定されるエラー
- システム輝度設定へのアクセス権限エラー
- WMIイベント監視の失敗
- 同期処理中の一時的な失敗

### 4.2 エラー処理方針
- 権限エラーの適切な通知
- 監視機能の自動再開
- 同期失敗時のリトライメカニズム

## 5. コマンドライン拡張

### 5.1 新規コマンド
```
sync enable              : 自動同期を有効化
sync disable             : 自動同期を無効化
sync status             : 同期状態の表示
system-brightness get    : システムの輝度設定を取得
system-brightness set    : システムの輝度設定を変更
```

## 6. 実装フェーズ

### 6.1 フェーズ1: 基本機能
- システム輝度の取得/設定機能の実装
- WMIイベント監視の実装
- 基本的な同期機能の実装

### 6.2 フェーズ2: 拡張機能
- 設定の永続化機能の実装
- エラーハンドリングの強化
- コマンドライン機能の拡張

### 6.3 フェーズ3: 最適化
- パフォーマンスの最適化
- 信頼性の向上
- ユーザーフィードバックの反映
