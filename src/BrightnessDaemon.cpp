#include <windows.h>
#include <shellapi.h>
#include <fstream>
#include "BrightnessManager.h"
#include "DummyLightSensor.h"
#include "SwitchBotLightSensor.h"
#include "ConfigManager.h"
#include "StringUtils.h"
#include <memory>
#include <string>

// タスクトレイアイコンの定数
#define WM_APP_NOTIFY (WM_APP + 1)
#define ID_TRAYICON 1
#define ID_MENU_EXIT 1001
#define ID_MENU_TOGGLE 1002
#define ID_MENU_TOGGLE_CONSOLE 1003

// グローバル変数
HINSTANCE g_hInstance;
HWND g_hwnd;
NOTIFYICONDATA g_nid;
std::unique_ptr<BrightnessManager> g_brightnessManager;
bool g_isSyncEnabled = false;
bool g_isConsoleVisible = false;
HHOOK g_consoleHook = nullptr;  // コンソールウィンドウのフック
HWND g_consoleWindow = nullptr; // コンソールウィンドウのハンドル

// エラーメッセージを表示する関数
void ShowErrorMessage(const std::string& message, const std::string& title = "エラー", UINT type = MB_OK | MB_ICONERROR) {
    std::wstring wMessage = StringUtils::Utf8ToWide(message);
    std::wstring wTitle = StringUtils::Utf8ToWide(title);
    MessageBoxW(NULL, wMessage.c_str(), wTitle.c_str(), type);
}

// 関数プロトタイプ
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void InitializeWindow();
void InitializeTrayIcon();
void InitializeConsole();
void ShowContextMenu(HWND hwnd, POINT pt);
void ToggleSync();
void ToggleConsoleWindow();
void Cleanup();
std::unique_ptr<ILightSensor> CreateLightSensor();

// コンソール管理
void InitializeConsole()
{
    // コンソールウィンドウの作成
    if (!AllocConsole())
    {
        ShowErrorMessage("コンソールの作成に失敗しました");
        return;
    }

    // 標準出力のリダイレクト
    FILE *fp;
    if (freopen_s(&fp, "CONOUT$", "w", stdout) != 0 ||
        freopen_s(&fp, "CONOUT$", "w", stderr) != 0)
    {
        ShowErrorMessage("標準出力のリダイレクトに失敗しました");
        return;
    }

    // 初期状態では非表示
    ShowWindow(GetConsoleWindow(), SW_HIDE);
    g_isConsoleVisible = false;
}

void ToggleConsoleWindow()
{
    HWND consoleWindow = GetConsoleWindow();
    if (consoleWindow == NULL)
    {
        ShowErrorMessage("コンソールウィンドウが見つかりません");
        return;
    }

    if (g_isConsoleVisible)
    {
        ShowWindow(consoleWindow, SW_HIDE);
        g_isConsoleVisible = false;
    }
    else
    {
        ShowWindow(consoleWindow, SW_SHOW);
        g_isConsoleVisible = true;
    }
}

// センサーの作成
std::unique_ptr<ILightSensor> CreateLightSensor()
{
    try
    {
        auto &config = ConfigManager::Instance();
        config.Load();

        // Light Sensorタイプのデバイスを検索
        if (config.HasDeviceType("Light Sensor"))
        {
            auto device = config.GetFirstDeviceByType("Light Sensor");
            StringUtils::OutputMessage("Light Sensorデバイスを使用: " + device["name"].get<std::string>());
            return std::make_unique<SwitchBotLightSensor>(device["name"].get<std::string>());
        }
        else
        {
            std::string message =
                "Light Sensorタイプのデバイスが設定されていません。\n"
                "設定ファイルにLight Sensorデバイスを追加してください。\n"
                "一時的にダミーセンサーを使用します。";
            ShowErrorMessage(message, "警告", MB_OK | MB_ICONWARNING);
            StringUtils::OutputMessage(message);
        }
    }
    catch (const ConfigException &e)
    {
        std::string error =
            std::string("設定の読み込みに失敗しました: ") + e.what() + "\n"
            "設定ファイルを確認してください。\n"
            "一時的にダミーセンサーを使用します。";
        ShowErrorMessage(error, "エラー", MB_OK | MB_ICONWARNING);
        StringUtils::OutputMessage(error);
    }
    catch (const std::exception &e)
    {
        std::string error =
            std::string("SwitchBotの初期化に失敗しました: ") + e.what() + "\n"
            "デバイスの接続状態を確認してください。\n"
            "一時的にダミーセンサーを使用します。";
        ShowErrorMessage(error, "エラー", MB_OK | MB_ICONWARNING);
        StringUtils::OutputMessage(error);
    }

    // エラー発生時はダミーセンサーを使用
    return std::make_unique<DummyLightSensor>();
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    g_hInstance = hInstance;

    // ウィンドウクラスの登録
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"BrightnessDaemonClass";
    RegisterClassEx(&wc);

    // ウィンドウの作成（非表示）
    InitializeWindow();

    // タスクトレイアイコンの初期化
    InitializeTrayIcon();

    // コンソールの初期化
    InitializeConsole();

    // BrightnessManagerの初期化
    g_brightnessManager = std::make_unique<BrightnessManager>(CreateLightSensor());

    // 設定の適用
    try {
        auto& config = ConfigManager::Instance();
        g_brightnessManager->SetUpdateInterval(std::chrono::milliseconds(config.GetUpdateInterval()));
        g_brightnessManager->SetBrightnessRange(config.GetMinBrightness(), config.GetMaxBrightness());
        StringUtils::OutputMessage("設定を読み込みました: "
            "更新間隔=" + std::to_string(config.GetUpdateInterval()) + "ms, "
            "輝度範囲=" + std::to_string(config.GetMinBrightness()) + "-" +
            std::to_string(config.GetMaxBrightness()) + "%");
    }
    catch (const ConfigException& e) {
        ShowErrorMessage(std::string("設定の読み込みに失敗しました: ") + e.what() + "\n"
            "デフォルト値を使用します。", "警告", MB_OK | MB_ICONWARNING);
    }

    // 初期化完了のログ出力
    StringUtils::OutputMessage("BrightnessDaemon initialized successfully.");

    // メッセージループ
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    Cleanup();
    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_APP_NOTIFY:
        switch (LOWORD(lParam))
        {
        case WM_RBUTTONUP:
        {
            POINT pt;
            GetCursorPos(&pt);
            ShowContextMenu(hwnd, pt);
            return 0;
        }
        }
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case ID_MENU_EXIT:
            DestroyWindow(hwnd);
            return 0;

        case ID_MENU_TOGGLE:
            ToggleSync();
            return 0;

        case ID_MENU_TOGGLE_CONSOLE:
            ToggleConsoleWindow();
            return 0;
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void InitializeWindow()
{
    g_hwnd = CreateWindowEx(
        0,
        L"BrightnessDaemonClass",
        L"BrightnessDaemon",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, CW_USEDEFAULT,
        NULL,
        NULL,
        g_hInstance,
        NULL);

    if (g_hwnd == NULL)
    {
        ShowErrorMessage("ウィンドウの作成に失敗しました");
        exit(1);
    }
}

void InitializeTrayIcon()
{
    g_nid = {};
    g_nid.cbSize = sizeof(NOTIFYICONDATA);
    g_nid.hWnd = g_hwnd;
    g_nid.uID = ID_TRAYICON;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_APP_NOTIFY;
    g_nid.hIcon = LoadIcon(NULL, IDI_APPLICATION); // デフォルトアイコンを使用
    wcscpy_s(g_nid.szTip, L"BrightnessDaemon");

    if (!Shell_NotifyIcon(NIM_ADD, &g_nid))
    {
        ShowErrorMessage("タスクトレイアイコンの作成に失敗しました");
        exit(1);
    }
}

void ShowContextMenu(HWND hwnd, POINT pt)
{
    HMENU hMenu = CreatePopupMenu();
    if (hMenu)
    {
        InsertMenu(hMenu, -1, MF_BYPOSITION | MF_STRING, ID_MENU_TOGGLE,
                   g_isSyncEnabled ? L"同期を停止" : L"同期を開始");
        InsertMenu(hMenu, -1, MF_BYPOSITION | MF_STRING, ID_MENU_TOGGLE_CONSOLE,
                   g_isConsoleVisible ? L"コンソールを隠す" : L"コンソールを表示");
        InsertMenu(hMenu, -1, MF_BYPOSITION | MF_STRING, ID_MENU_EXIT, L"終了");

        // メニューの表示
        SetForegroundWindow(hwnd);
        TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN,
                       pt.x, pt.y, 0, hwnd, NULL);
        DestroyMenu(hMenu);
    }
}

void ToggleSync()
{
    if (g_isSyncEnabled)
    {
        g_brightnessManager->StopSync();
        g_isSyncEnabled = false;
    }
    else
    {
        g_brightnessManager->StartSync();
        g_isSyncEnabled = true;
    }
}

void Cleanup()
{
    // タスクトレイアイコンの削除
    Shell_NotifyIcon(NIM_DELETE, &g_nid);

    // BrightnessManagerの停止
    if (g_brightnessManager)
    {
        g_brightnessManager->StopSync();
    }

    // コンソールのクリーンアップ
    if (g_consoleHook)
    {
        UnhookWindowsHookEx(g_consoleHook);
        g_consoleHook = nullptr;
    }
    if (g_consoleWindow)
    {
        FreeConsole();
        g_consoleWindow = nullptr;
    }
}
