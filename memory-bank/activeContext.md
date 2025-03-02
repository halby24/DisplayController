# アクティブコンテキスト

## 現在の作業：プラグインDLLのビルド時コピー機能の実装

### 実装の概要
- BrightnessDaemonのビルド出力ディレクトリ下にpluginsディレクトリを作成
- プラグインDLLを自動的にそのディレクトリにコピー
- プラグイン単体でのビルド時にもコピーが実行されるように設定

### 技術的な詳細
1. ビルド出力ディレクトリの設定
```cmake
set(RUNTIME_PLUGINS_DIR "${CMAKE_BINARY_DIR}/bin/$<CONFIG>/plugins")
```

2. プラグインディレクトリの作成
```cmake
add_custom_command(
    TARGET <target> POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${RUNTIME_PLUGINS_DIR}"
)
```

3. DLLのコピー処理
```cmake
add_custom_command(
    TARGET <target> POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:PluginName>"
        "${RUNTIME_PLUGINS_DIR}"
)
```

### 変更されたファイル
1. CMakeLists.txt
   - BrightnessDaemonのビルド後処理を追加
   - 両方のプラグインDLLをコピー

2. plugins/DummyLightSensor/CMakeLists.txt
   - 単体ビルド時のコピー処理を追加

3. plugins/SwitchBotLightSensor/CMakeLists.txt
   - 単体ビルド時のコピー処理を追加

### コミット予定の変更
```
feat: プラグインDLLのビルド時コピー機能を追加

- BrightnessDaemonのビルド出力ディレクトリ下にpluginsディレクトリを作成
- プラグインDLLを自動的にそのディレクトリにコピー
- プラグイン単体でのビルド時にもコピーが実行されるように設定
