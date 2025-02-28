# Windows用モニター輝度制御アプリケーション設計ドキュメント

## 1. プロジェクト概要

### 1.1 目的
複数のモニターの輝度を統一的に制御できるアプリケーションを開発する。各モニターの特性に応じて適切な輝度マッピングを行い、一貫した視覚体験を提供する。

### 1.2 背景
複数のモニターを使用する環境では、各モニターの輝度特性が異なるため、同じ輝度値を設定しても実際の明るさが異なる場合がある。このアプリケーションは、モニターごとの輝度マッピングを管理し、統一された視覚体験を実現する。

### 1.3 主要機能
- 接続されているモニターの一覧表示
- モニターごとの輝度マッピング設定
- 統一された輝度値による制御
- モニターごとの輝度設定
- 全モニターの一括輝度設定

## 2. 技術仕様

### 2.1 開発環境
- **言語**: C++
- **開発環境**: Visual Studio 2022
- **対象OS**: Windows 10/11
- **必要なSDK**: Windows SDK 10.0

### 2.2 使用するWindows API
- **Display Device API**
  - `EnumDisplayMonitors`
  - `GetMonitorInfo`
- **Physical Monitor API**
  - `GetNumberOfPhysicalMonitorsFromHMONITOR`
  - `GetPhysicalMonitorsFromHMONITOR`
  - `GetMonitorBrightness`
  - `SetMonitorBrightness`

## 3. アーキテクチャ設計

### 3.1 コアコンポーネント
```
DisplayController
    ├── MonitorManager       // モニター管理
    ├── BrightnessMapper    // 輝度マッピング
    ├── MonitorController   // 輝度制御
    └── CommandProcessor    // コマンド処理
```

### 3.2 クラス構造
```cpp
// モニター管理インターフェース
class IMonitorManager {
    virtual std::vector<MonitorInfo> EnumerateMonitors() = 0;
    virtual MonitorInfo GetMonitorInfo(MonitorId id) = 0;
};

// 輝度マッピングインターフェース
class IBrightnessMapper {
    virtual int MapBrightness(MonitorId id, int normalizedBrightness) = 0;
    virtual void SetMappingConfig(MonitorId id, const MappingConfig& config) = 0;
    virtual MappingConfig GetMappingConfig(MonitorId id) = 0;
};

// モニター制御インターフェース
class IMonitorController {
    virtual bool SetBrightness(MonitorId id, int brightness) = 0;
    virtual int GetBrightness(MonitorId id) = 0;
    virtual bool SetUnifiedBrightness(int normalizedBrightness) = 0;
};

// 設定管理
class MappingConfig {
    int minBrightness;
    int maxBrightness;
    std::vector<std::pair<int, int>> mappingPoints;
};

// コマンド処理
class CommandProcessor {
    void ProcessCommand(int argc, char* argv[]);
    void ShowHelp();
};
```

## 4. 機能詳細

### 4.1 モニター管理
- Windows APIを使用した物理モニターの列挙
- モニター情報（ID、名前、機能）の取得
- モニターの接続状態の監視

### 4.2 輝度マッピング
- モニターごとの輝度範囲設定
- カスタムマッピングポイントの定義
- 線形/非線形マッピングのサポート
- マッピング設定の永続化

### 4.3 輝度制御
- 正規化された輝度値（0-100%）の使用
- モニターごとの実際の輝度値へのマッピング
- 一括設定時の同時制御

### 4.4 コマンドライン操作
```
使用方法:
brightness.exe [command] [options]

コマンド:
  list                    : モニター一覧の表示
  get <monitor_id>        : 指定モニターの輝度取得
  set <monitor_id> <value>: 指定モニターの輝度設定
  setall <value>         : 全モニターの統一輝度設定
  config <monitor_id>     : モニターのマッピング設定
  help                   : ヘルプの表示
```

## 5. エラーハンドリング

### 5.1 想定されるエラー
- モニター非対応エラー
- 無効な輝度値
- APIコール失敗
- 設定ファイルのエラー
- 無効なコマンドライン引数

### 5.2 エラー処理方針
- 詳細なエラーメッセージの提供
- エラー状態のログ記録
- 適切なエラーコードの返却
- 失敗時の安全な状態維持

## 6. 実装計画

### 6.1 フェーズ1: 基本実装
- プロジェクト環境構築
- インターフェース定義
- 基本的なモニター制御機能

### 6.2 フェーズ2: マッピング機能
- 輝度マッピング実装
- 設定保存機能
- マッピング調整UI

### 6.3 フェーズ3: 機能拡張
- 高度なマッピング機能
- パフォーマンス最適化
- ユーザビリティ改善

## 7. 制限事項

### 7.1 システム要件
- Windows 10/11
- DDC/CI対応モニター
- 適切な権限設定

### 7.2 既知の制限
- モニター応答の遅延
- 特定モデルでの互換性問題
- マッピング精度の限界

## 8. テスト計画

### 8.1 テスト項目
- モニター検出機能
- 輝度マッピング精度
- 設定保存/読み込み
- エラー処理
- パフォーマンス

### 8.2 テスト環境
- 単一モニター構成
- 複数モニター構成
- 異なるメーカー/モデル
- 異なる輝度特性

## 9. 参考資料

- Windows Display Device API Documentation
- DDC/CI Protocol Specification
- モニター輝度特性に関する技術資料
