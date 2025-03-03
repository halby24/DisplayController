# 設計上の決定記録

## 2025/03/03 - プラグイン設定構造の再設計

### 背景
- SwitchBotがプラグインの一つとなったが、設定がルートレベルにある
- プラグイン固有の設定（token, secret）がグローバルな場所にある
- 新しいプラグインを追加する際の設計指針が不明確

### 決定事項
1. 設定ファイルの構造を階層化
   - プラグイン設定を "plugins" セクション下に移動
   - 各プラグインが独自の名前空間を持つ
   - プラグイン固有の設定を適切な場所に配置

2. 新しい設定構造の採用
   ```json
   {
     "plugins": {
       "switchbot": {
         "global_settings": {
           "token": "YOUR_TOKEN",
           "secret": "YOUR_SECRET"
         },
         "devices": []
       }
     }
   }
   ```

### 実装方針
1. ConfigManagerの拡張
   - プラグイン設定へのアクセス方法の追加
   - 後方互換性のための移行ロジック実装

2. プラグインの更新
   - 設定読み込みロジックの更新
   - エラーハンドリングの強化

### 影響範囲
- ConfigManager.h/cpp
- SwitchBotPlugin.h/cpp
- 設定ファイル構造
- テストケース

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
