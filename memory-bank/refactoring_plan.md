# DisplayController リファクタリング計画

## 概要
現在のプロジェクトで、以下のコンポーネントの名称と役割を明確化する変更を行います：

- DisplayController (shared library) → DisplayControllerLib
- brightness_daemon (CLI) → DisplayControllerCLI
- 新規: BrightnessDaemon (システムトレイアプリケーション)

## 変更内容

### 1. ライブラリのリネーム
- 現在の`DisplayController`ライブラリを`DisplayControllerLib`にリネーム
- 共有ライブラリとしての役割を維持
- 既存の機能はそのまま保持

### 2. CLIツールの変更
- 現在の`brightness_daemon`を`DisplayControllerCLI`にリネーム
- エントリーポイント: `main.cpp`
- 機能の変更なし、名称のみ変更

### 3. システムトレイアプリケーションの分離
- 新規ターゲット: `BrightnessDaemon`
- エントリーポイント: `BrightnessDaemon.cpp`のWinMain
- システムトレイアプリケーションとしての機能を担当

## CMakeLists.txtの変更計画

```cmake
# 共有ライブラリ
add_library(DisplayControllerLib SHARED
    src/BrightnessManager.cpp
    src/ConfigManager.cpp
    src/MonitorController.cpp
    src/PluginLoader.cpp
)

# CLIツール
add_executable(DisplayControllerCLI
    src/main.cpp
)

target_link_libraries(DisplayControllerCLI PRIVATE
    DisplayControllerLib
    DisplayControllerCommon
)

# システムトレイアプリケーション
add_executable(BrightnessDaemon
    src/BrightnessDaemon.cpp
)

target_link_libraries(BrightnessDaemon PRIVATE
    DisplayControllerLib
    DisplayControllerCommon
)
```

## 実装手順

1. CMakeLists.txtの更新
   - ライブラリ名の変更
   - 実行ファイルのターゲット名変更
   - 新規ターゲットの追加
   - 依存関係の更新

2. インストール設定の更新
   - 新しいターゲット名に合わせてインストールルールを更新

3. テスト
   - ビルドの確認
   - 各コンポーネントの動作確認
   - インストールの確認
