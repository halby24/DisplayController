#include <windows.h>
#include <shellapi.h>
#include <fstream>
#include "BrightnessManager.h"
#include "PluginLoader.h"
#include "ConfigManager.h"
#include <common/StringUtils.h>
#include <memory>
#include <string>
#include <filesystem>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// タスクトレイアイコンの定数
#define WM_APP_NOTIFY (WM_APP + 1)
#define ID_TRAYICON 1
#define ID_MENU_EXIT 1001
#define ID_MENU_TOGGLE 1002
#define ID_MENU_TOGGLE_CONSOLE 1003
#define ID_MENU_OPEN_CONFIG 1004

// グローバル変数
HINSTANCE g_hInstance;
HWND g_hwnd;
NOTIFYICONDATAW g_nid;
std::unique_ptr<BrightnessManager> g_brightnessManager;
std::unique_ptr<PluginLoader> g_pluginLoader;
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

// モニター設定の自動追加
void CheckAndAddMonitorConfigs() {
    try {
        auto& config = ConfigManager::Instance();
        auto monitors = g_brightnessManager->GetMonitorController().GetMonitors();

        for (const auto& monitor : monitors) {
            // モニターの詳細情報を取得
            MonitorController::MonitorInfo info = monitor;
            g_brightnessManager->GetMonitorController().GetDetailedMonitorInfo(info);

            // 設定が存在しない場合は追加
            if (!config.HasMonitor(StringUtils::WideToUtf8(info.humanReadableName))) {
                StringUtils::OutputMessage("新しいモニターを検出: " + StringUtils::WideToUtf8(info.humanReadableName));

                // デフォルトの輝度範囲設定を作成
                MonitorBrightnessRange range;
                range.min = 0;
                range.max = 100;

                // 設定を追加
                config.AddMonitor(StringUtils::WideToUtf8(info.humanReadableName), range);
                StringUtils::OutputMessage("モニター設定を追加しました: " + StringUtils::WideToUtf8(info.humanReadableName));
            }
        }
    }
    catch (const std::exception& e) {
        std::string error = "モニター設定の自動追加に失敗しました: " + std::string(e.what());
        ShowErrorMessage(error, "警告", MB_OK | MB_ICONWARNING);
        StringUtils::OutputMessage(error);
    }
}

// コンソール管理
void InitializeConsole() {
    if (!AllocConsole()) {
        ShowErrorMessage("コンソールの作成に失敗しました");
        return;
    }

    FILE *fp;
    if (freopen_s(&fp, "CONOUT$", "w", stdout) != 0 ||
        freopen_s(&fp, "CONOUT$", "w", stderr) != 0) {
        ShowErrorMessage("標準出力のリダイレクトに失敗しました");
        return;
    }

    ShowWindow(GetConsoleWindow(), SW_HIDE);
    g_isConsoleVisible = false;
}

void ToggleConsoleWindow() {
    HWND consoleWindow = GetConsoleWindow();
    if (consoleWindow == NULL) {
        ShowErrorMessage("コンソールウィンドウが見つかりません");
        return;
    }

    if (g_isConsoleVisible) {
        ShowWindow(consoleWindow, SW_HIDE);
        g_isConsoleVisible = false;
    }
    else {
        ShowWindow(consoleWindow, SW_SHOW);
        g_isConsoleVisible = true;
    }
}

// センサーの作成
std::unique_ptr<ILightSensor> CreateLightSensor() {
    try {
        auto& config = ConfigManager::Instance();
        config.Load();

        std::filesystem::path pluginDir = std::filesystem::current_path() / "plugins";
        if (!std::filesystem::exists(pluginDir)) {
            std::filesystem::create_directory(pluginDir);
        }

        g_pluginLoader = std::make_unique<PluginLoader>();
        size_t loadedCount = g_pluginLoader->LoadPlugins(pluginDir.string());
        StringUtils::OutputMessage("プラグインを読み込みました: " + std::to_string(loadedCount) + "個");

        if (config.HasDeviceType("Light Sensor")) {
            // Light Sensorタイプのデバイスを取得
            auto devices = config.GetDevicesByType("Light Sensor");
            if (devices.empty()) {
                throw std::runtime_error("Light Sensorタイプのデバイスが見つかりません");
            }

            // 最初のデバイスを使用
            auto device = devices[0];
            const std::string& deviceName = device["name"].get<std::string>();

            // プラグインを探索
            for (const auto& plugin : {"SwitchBotLightSensor", "DummyLightSensor"}) {
                try {
                    // デバイス名でプラグインの設定を取得してみる
                    // 成功すれば、そのプラグインに属しているデバイス
                    config.GetPluginConfig(plugin, "id", deviceName);
                    StringUtils::OutputMessage("Light Sensorプラグインを使用: " + std::string(plugin));
                    return g_pluginLoader->CreateSensor(plugin, device);
                } catch (const ConfigException&) {
                    // このプラグインには属していない
                    continue;
                }
            }

            throw std::runtime_error("デバイス " + deviceName + " に対応するプラグインが見つかりません");
        }
        else {
            StringUtils::OutputMessage("Light Sensorタイプのデバイスが設定されていません。設定ファイルにLight Sensorデバイスを追加してください。一時的にダミーセンサーを使用します。");
            ShowErrorMessage("Light Sensorタイプのデバイスが設定されていません。設定ファイルにLight Sensorデバイスを追加してください。一時的にダミーセンサーを使用します。", "警告", MB_OK | MB_ICONWARNING);
            return g_pluginLoader->CreateSensor("DummyLightSensor", json::object());
        }
    }
    catch (const std::exception& e) {
        std::string error = "センサーの初期化に失敗しました: " + std::string(e.what()) + " 一時的にダミーセンサーを使用します。";
        ShowErrorMessage(error, "エラー", MB_OK | MB_ICONWARNING);
        StringUtils::OutputMessage(error);
        return g_pluginLoader->CreateSensor("DummyLightSensor", json::object());
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    g_hInstance = hInstance;

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"BrightnessDaemonClass";
    RegisterClassExW(&wc);

    InitializeWindow();
    InitializeTrayIcon();
    InitializeConsole();

    g_brightnessManager = std::make_unique<BrightnessManager>(CreateLightSensor());

    try {
        auto& config = ConfigManager::Instance();

        // 設定のバックアップを作成
        config.CreateBackup();
        StringUtils::OutputMessage("設定ファイルのバックアップを作成しました");

        // モニター設定の自動追加
        CheckAndAddMonitorConfigs();

        // 基本設定の適用
        g_brightnessManager->SetUpdateInterval(std::chrono::milliseconds(config.GetUpdateInterval()));
        g_brightnessManager->SetBrightnessRange(config.GetMinBrightness(), config.GetMaxBrightness());
        StringUtils::OutputMessage("設定を読み込みました: 更新間隔=" + std::to_string(config.GetUpdateInterval()) + "ms, 輝度範囲=" + std::to_string(config.GetMinBrightness()) + "-" + std::to_string(config.GetMaxBrightness()) + "%");

        // 起動時同期設定の適用
        if (config.GetSyncOnStartup()) {
            ToggleSync(); // 同期を開始
            StringUtils::OutputMessage("起動時同期設定が有効です。同期を開始します。");
        }
    }
    catch (const ConfigException& e) {
        std::string error = "設定の読み込みに失敗しました: " + std::string(e.what());
        ShowErrorMessage(error, "警告", MB_OK | MB_ICONWARNING);

        try {
            // 設定の復元を試みる
            auto& config = ConfigManager::Instance();
            config.RestoreFromBackup();
            StringUtils::OutputMessage("バックアップから設定を復元しました");
        }
        catch (const std::exception& e) {
            std::string error = "バックアップからの復元に失敗しました: " + std::string(e.what()) + " デフォルト値を使用します。";
            ShowErrorMessage(error, "警告", MB_OK | MB_ICONWARNING);
            StringUtils::OutputMessage(error);
        }
    }

    StringUtils::OutputMessage("BrightnessDaemon initialized successfully.");

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    Cleanup();
    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_APP_NOTIFY:
        switch (LOWORD(lParam)) {
        case WM_RBUTTONUP: {
            POINT pt;
            GetCursorPos(&pt);
            ShowContextMenu(hwnd, pt);
            return 0;
        }
        }
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_MENU_EXIT:
            DestroyWindow(hwnd);
            return 0;

        case ID_MENU_TOGGLE:
            ToggleSync();
            return 0;

        case ID_MENU_TOGGLE_CONSOLE:
            ToggleConsoleWindow();
            return 0;
  case ID_MENU_OPEN_CONFIG: {
   std::wstring configFilePath = StringUtils::Utf8ToWide(ConfigManager::Instance().GetConfigFilePath());
   ShellExecuteW(NULL, L"open", configFilePath.c_str(), NULL, NULL, SW_SHOWNORMAL);
   return 0;
  }
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

void InitializeWindow() {
    g_hwnd = CreateWindowExW(
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

    if (g_hwnd == NULL) {
        ShowErrorMessage("ウィンドウの作成に失敗しました");
        exit(1);
    }
}

void InitializeTrayIcon() {
    g_nid = {};
    g_nid.cbSize = sizeof(NOTIFYICONDATAW);
    g_nid.hWnd = g_hwnd;
    g_nid.uID = ID_TRAYICON;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_APP_NOTIFY;
    g_nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcscpy_s(g_nid.szTip, sizeof(g_nid.szTip) / sizeof(WCHAR), L"BrightnessDaemon");

    if (!Shell_NotifyIconW(NIM_ADD, &g_nid)) {
        ShowErrorMessage("タスクトレイアイコンの作成に失敗しました");
        exit(1);
    }
}

void ShowContextMenu(HWND hwnd, POINT pt) {
    HMENU hMenu = CreatePopupMenu();
    if (hMenu) {
        std::wstring syncText = g_isSyncEnabled ? L"同期を停止" : L"同期を開始";
        std::wstring consoleText = g_isConsoleVisible ? L"コンソールを隠す" : L"コンソールを表示";
        std::wstring exitText = L"終了";

        InsertMenuW(hMenu, -1, MF_BYPOSITION | MF_STRING, ID_MENU_TOGGLE, syncText.c_str());
        InsertMenuW(hMenu, -1, MF_BYPOSITION | MF_STRING, ID_MENU_TOGGLE_CONSOLE, consoleText.c_str());
        InsertMenuW(hMenu, -1, MF_BYPOSITION | MF_STRING, ID_MENU_EXIT, exitText.c_str());
        InsertMenuW(hMenu, -1, MF_BYPOSITION | MF_STRING, ID_MENU_OPEN_CONFIG, L"設定フォルダを開く");

        SetForegroundWindow(hwnd);
        TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hwnd, NULL);
        DestroyMenu(hMenu);
    }
}

void ToggleSync() {
    if (g_isSyncEnabled) {
        g_brightnessManager->StopSync();
        g_isSyncEnabled = false;
    }
    else {
        g_brightnessManager->StartSync();
        g_isSyncEnabled = true;
    }
}

void Cleanup() {
    Shell_NotifyIconW(NIM_DELETE, &g_nid);

    if (g_brightnessManager) {
        g_brightnessManager->StopSync();
    }

    g_pluginLoader.reset();

    if (g_consoleHook) {
        UnhookWindowsHookEx(g_consoleHook);
        g_consoleHook = nullptr;
    }
    if (g_consoleWindow) {
        FreeConsole();
        g_consoleWindow = nullptr;
    }
}
