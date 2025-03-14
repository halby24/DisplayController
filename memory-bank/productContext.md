# DisplayController プロジェクト概要

## プロジェクトの目的
ディスプレイの輝度を環境光に応じて自動調整するシステム。

## コアコンポーネント
1. BrightnessDaemon
   - システムのメインプロセス
   - 輝度制御のコアロジック
   - プラグイン管理

2. LightSensorプラグインシステム
   - 照度センサーとの通信を担当
   - プラグインベースのアーキテクチャ
   - 複数のセンサータイプをサポート

3. ConfigManager
   - 設定ファイルの管理
   - プラグイン設定の取り扱い

## 技術スタック
- C++17
- Windows SDK
- CMake
- vcpkg（パッケージ管理）

## アーキテクチャの特徴
1. プラグインベースの設計
2. インターフェース駆動開発
3. 設定ファイルベースの構成管理
4. エラー処理とフォールバック機能

## メモリバンクファイルの目的
- productContext.md: プロジェクトの全体像と主要コンポーネントの説明
- activeContext.md: 現在の開発コンテキストと作業中のタスク
- progress.md: 実装の進捗状況の追跡
- decisionLog.md: アーキテクチャと設計の決定事項の記録
