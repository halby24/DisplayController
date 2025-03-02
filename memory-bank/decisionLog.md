# 設計上の決定記録

## 2025/03/02 - プラグインDLLのビルド時コピー処理の追加

### 背景
- BrightnessDaemonの実行ファイルと同じディレクトリにプラグインDLLが必要
- 現状はインストール時のみプラグインがコピーされる設定になっている

### 決定事項
1. ビルド時にプラグインDLLを自動的にコピーする機能を追加
   - BrightnessDaemonのビルド出力ディレクトリ下にpluginsディレクトリを作成
   - プラグインDLLを自動的にそのディレクトリにコピー
2. プラグイン単体でのビルド時にもコピー処理が実行されるように設定
   - 各プラグインのCMakeListsファイルにコピー処理を追加

### 実装方針
1. ルートCMakeListsファイルの変更
   - add_custom_commandとadd_custom_targetを使用してビルド後処理を追加
   - BrightnessDaemonのビルド後に実行されるように依存関係を設定

2. プラグインCMakeListsファイルの変更
   - 各プラグインのビルド後にDLLをコピーする処理を追加
   - BrightnessDaemonの出力ディレクトリを考慮したパス設定

### 影響範囲
- CMakeListsファイル（ルート）
- CMakeListsファイル（DummyLightSensor）
- CMakeListsファイル（SwitchBotLightSensor）
