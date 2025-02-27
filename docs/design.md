# Windows用モニター輝度制御アプリケーション設計ドキュメント

## 1. プロジェクト概要

### 1.1 目的
Windows APIを利用して、コマンドラインから複数モニターの輝度を制御できるアプリケーションを開発する。シンプルで軽量な実装により、モニターの輝度設定を効率的に管理する。

### 1.2 背景
複数のモニターを使用する環境では、各モニターの輝度を個別に調整する必要があり、手間がかかる。コマンドラインツールとして実装することで、簡単かつ効率的な輝度制御を実現する。

### 1.3 主要機能
- 接続されているモニターの一覧表示
- 各モニターの現在の輝度値の取得
- モニターごとの輝度設定
- 全モニターの一括輝度設定

## 2. 技術仕様

### 2.1 開発環境
- **言語**: C++
- **開発環境**: Visual Studio 2022
- **対象OS**: Windows 10/11
- **必要なSDK**: Windows SDK 10.0

### 2.2 使用するWindows API
- **Physical Monitor API**
  - `GetNumberOfPhysicalMonitorsFromHMONITOR`
  - `GetPhysicalMonitorsFromHMONITOR`
  - `GetMonitorBrightness`
  - `SetMonitorBrightness`
- **Display Device API**
  - `EnumDisplayMonitors`
  - `GetMonitorInfo`

## 3. アーキテクチャ設計

### 3.1 コアコンポーネント
```
MonitorController
    ├── MonitorEnumerator  // モニター検出
    ├── BrightnessController  // 輝度制御
    └── CommandProcessor   // コマンド処理
```

### 3.2 クラス構造
```cpp
class MonitorEnumerator {
    // モニターの列挙と情報取得
    vector<PhysicalMonitor> EnumerateMonitors();
    MonitorInfo GetMonitorInfo(PhysicalMonitor monitor);
};

class BrightnessController {
    // 輝度制御の実装
    int GetBrightness(PhysicalMonitor monitor);
    bool SetBrightness(PhysicalMonitor monitor, int brightness);
    bool SetAllBrightness(int brightness);
};

class CommandProcessor {
    // コマンドライン引数の処理
    void ProcessCommand(int argc, char* argv[]);
    void ShowHelp();
};
```

## 4. 機能詳細

### 4.1 モニター検出
- Windows APIを使用した物理モニターの列挙
- モニター情報（ID、名前）の取得
- 接続状態の確認

### 4.2 輝度制御
- 現在の輝度値の取得（0-100%）
- 輝度値の設定（0-100%）
- 設定値の検証と制限

### 4.3 コマンドライン操作
```
使用方法:
brightness.exe [command] [options]

コマンド:
  list              : モニター一覧の表示
  get <monitor_id>  : 指定モニターの輝度取得
  set <monitor_id> <value> : 指定モニターの輝度設定
  setall <value>    : 全モニターの輝度設定
  help              : ヘルプの表示
```

## 5. エラーハンドリング

### 5.1 想定されるエラー
- モニター非対応エラー
- 無効な輝度値
- APIコール失敗
- 無効なコマンドライン引数

### 5.2 エラー処理方針
- エラーメッセージの標準エラー出力
- エラー状態に応じたリターンコード
- 操作失敗時の安全な終了処理

## 6. 実装計画

### 6.1 フェーズ1: 基本実装
- プロジェクト環境構築
- モニター検出機能の実装
- 基本的な輝度制御の実装

### 6.2 フェーズ2: 機能拡張
- コマンドライン処理の実装
- エラーハンドリングの実装
- 動作検証とバグ修正

## 7. 制限事項

### 7.1 システム要件
- Windows 10/11
- DDC/CI対応モニター
- 管理者権限（必要な場合）

### 7.2 既知の制限
- 一部のモニターで輝度制御が機能しない可能性
- モニターの応答遅延
- 特定のグラフィックドライバーとの互換性問題

## 8. テスト計画

### 8.1 テスト項目
- モニター検出機能
- 輝度制御機能
- コマンドライン処理
- エラーハンドリング

### 8.2 テスト環境
- 単一モニター構成
- 複数モニター構成
- 異なるモニターメーカー/モデル

## 9. 参考資料

- Windows Physical Monitor API Documentation
- Display Device API Documentation
- DDC/CI Protocol Specification
