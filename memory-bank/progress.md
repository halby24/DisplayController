# 進捗状況

## 2025/03/02 - プラグインDLLのビルド時コピー機能の実装

### 完了した作業
1. CMakeListsファイルの修正
   - ルートCMakeListsファイル：
     - BrightnessDaemonのビルド後にプラグインディレクトリを作成
     - DummyLightSensorとSwitchBotLightSensorのDLLを自動的にコピー
   - DummyLightSensorのCMakeListsファイル：
     - 単体ビルド時のDLLコピー処理を追加
   - SwitchBotLightSensorのCMakeListsファイル：
     - 単体ビルド時のDLLコピー処理を追加

### 次のステップ
1. 変更のコミット
   - コミットメッセージ：「feat: プラグインDLLのビルド時コピー機能を追加」
   - 変更ファイル：
     - CMakeLists.txt
     - plugins/DummyLightSensor/CMakeLists.txt
     - plugins/SwitchBotLightSensor/CMakeLists.txt
