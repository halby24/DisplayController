# BrightnessDaemon タスクトレイメニューに設定再読み込みボタンを追加する計画

## 概要

BrightnessDaemon のタスクトレイメニューに「設定再読み込み」ボタンを追加し、ユーザーがGUIから設定ファイルを再読み込みできるようにする。

## 計画

1. **`src/BrightnessDaemon.cpp` ファイルの修正:**
    - メニュー項目IDの定義 (`#define` セクション) に `ID_MENU_RELOAD_CONFIG` を追加します。例: `#define ID_MENU_RELOAD_CONFIG 1005`
    - `ShowContextMenu` 関数に、「設定再読み込み」メニュー項目を追加するコードを追加します。例: `InsertMenuW(hMenu, -1, MF_BYPOSITION | MF_STRING, ID_MENU_RELOAD_CONFIG, L"設定再読み込み");`
    - `WindowProc` 関数の `WM_COMMAND` メッセージ処理に、`ID_MENU_RELOAD_CONFIG` のケースを追加します。このケースでは、`ConfigManager::Instance().Load()` を呼び出して設定ファイルを再読み込みます。

2. **コードモードへの切り替え:**
    - 上記の修正を実装するために、コードモードに切り替える必要があります。

## Mermaidダイアグラム

```mermaid
graph LR
    A[ユーザー操作: タスクトレイメニューから「設定再読み込み」を選択] --> B{WM_COMMANDメッセージ受信};
    B --> C{wParam == ID_MENU_RELOAD_CONFIG?};
    C -- yes --> D[ConfigManager::Instance().Load() を呼び出し、設定ファイルを再読み込み];
    C -- no --> E[他のメニュー項目の処理];
    D --> F[設定再読み込み完了];
