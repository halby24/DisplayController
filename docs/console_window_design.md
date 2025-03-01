# コンソールウィンドウ機能の設計

## 概要
BrightnessDaemonにコンソールウィンドウ機能を追加し、実行時のログやデバッグ情報を表示できるようにします。

## 機能要件
1. タスクトレイメニューからコンソールウィンドウの表示/非表示を切り替え可能
2. コンソールウィンドウに標準出力と標準エラー出力を表示
3. アプリケーション終了時にコンソールウィンドウも適切に終了

## 技術設計

### コンソールウィンドウの管理
```cpp
// コンソールウィンドウの状態管理
bool g_isConsoleVisible = false;

// コンソールウィンドウの表示/非表示を切り替える関数
void ToggleConsoleWindow()
{
    HWND consoleWindow = GetConsoleWindow();
    if (g_isConsoleVisible) {
        ShowWindow(consoleWindow, SW_HIDE);
        g_isConsoleVisible = false;
    } else {
        ShowWindow(consoleWindow, SW_SHOW);
        g_isConsoleVisible = true;
    }
}
```

### メニュー項目の追加
```cpp
#define ID_MENU_TOGGLE_CONSOLE 1003  // 新しいメニューID

// コンテキストメニューに項目を追加
InsertMenu(hMenu, -1, MF_BYPOSITION | MF_STRING, ID_MENU_TOGGLE_CONSOLE,
    g_isConsoleVisible ? L"コンソールを隠す" : L"コンソールを表示");
```

### 標準出力のリダイレクト
```cpp
void InitializeConsole()
{
    // コンソールウィンドウの作成
    AllocConsole();

    // 標準出力のリダイレクト
    FILE* fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    freopen_s(&fp, "CONOUT$", "w", stderr);

    // 初期状態では非表示
    ShowWindow(GetConsoleWindow(), SW_HIDE);
}
```

## 実装手順
1. BrightnessDaemon.cppにコンソール管理機能を追加
2. メニュー項目とイベントハンドラの実装
3. 標準出力のリダイレクト機能の実装
4. クリーンアップ処理の追加

## エラーハンドリング
- コンソールウィンドウの作成失敗時の処理
- 標準出力リダイレクト失敗時の処理
- ウィンドウ表示/非表示切り替え失敗時の処理

## 注意事項
- コンソールウィンドウの初期状態は非表示
- アプリケーション終了時に適切にクリーンアップを実施
- 長時間の出力によるパフォーマンス影響の考慮
