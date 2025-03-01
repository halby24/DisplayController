#include <windows.h>
#include <shellapi.h>
#include <fstream>
#include "BrightnessManager.h"
#include "DummyLightSensor.h"
#include "SwitchBotLightSensor.h"
#include <memory>
#include <string>
#include <nlohmann/json.hpp>

// タスクトレイアイコンの定数
#define WM_APP_NOTIFY (WM_APP + 1)
#define ID_TRAYICON 1
#define ID_MENU_EXIT 1001
#define ID_MENU_TOGGLE 1002

// グローバル変数
HINSTANCE g_hInstance;
HWND g_hwnd;
NOTIFYICONDATA g_nid;
std::unique_ptr<BrightnessManager> g_brightnessManager;
bool g_isSyncEnabled = false;

// 関数プロトタイプ
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void InitializeWindow();
void InitializeTrayIcon();
void ShowContextMenu(HWND hwnd, POINT pt);
void ToggleSync();
void Cleanup();
std::unique_ptr<ILightSensor> CreateLightSensor();

// 設定ファイルから認証情報を読み込む
std::pair<std::string, std::string> LoadSwitchBotConfig()
{
    try {
        std::ifstream file("switchbot_config.json");
        if (!file.is_open()) {
            MessageBox(NULL, L"設定ファイルが見つかりません", L"警告", MB_OK | MB_ICONWARNING);
            return {"", ""};
        }

        nlohmann::json config = nlohmann::json::parse(file);
        return {
            config.value("token", ""),
            config.value("deviceId", "")
        };
    }
    catch (const std::exception& e) {
        std::string error = "設定ファイルの読み込みに失敗しました: ";
        error += e.what();
        MessageBoxA(NULL, error.c_str(), "エラー", MB_OK | MB_ICONERROR);
        return {"", ""};
    }
}

// センサーの作成
std::unique_ptr<ILightSensor> CreateLightSensor()
{
    auto [token, deviceId] = LoadSwitchBotConfig();
    if (!token.empty() && !deviceId.empty()) {
        try {
            return std::make_unique<SwitchBotLightSensor>(token, deviceId);
        }
        catch (const std::exception& e) {
            std::string error = "SwitchBotの初期化に失敗しました: ";
            error += e.what();
            MessageBoxA(NULL, error.c_str(), "エラー", MB_OK | MB_ICONWARNING);
        }
    }

    // 設定が無効な場合やエラーが発生した場合はダミーセンサーを使用
    MessageBox(NULL, L"ダミーセンサーを使用します", L"情報", MB_OK | MB_ICONINFORMATION);
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

    // BrightnessManagerの初期化
    g_brightnessManager = std::make_unique<BrightnessManager>(CreateLightSensor());

    // メッセージループ
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    Cleanup();
    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
    case WM_APP_NOTIFY:
        switch (LOWORD(lParam)) {
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
        switch (LOWORD(wParam)) {
        case ID_MENU_EXIT:
            DestroyWindow(hwnd);
            return 0;

        case ID_MENU_TOGGLE:
            ToggleSync();
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
        NULL
    );

    if (g_hwnd == NULL) {
        MessageBox(NULL, L"ウィンドウの作成に失敗しました", L"エラー", MB_OK | MB_ICONERROR);
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
    g_nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);  // デフォルトアイコンを使用
    wcscpy_s(g_nid.szTip, L"BrightnessDaemon");

    if (!Shell_NotifyIcon(NIM_ADD, &g_nid)) {
        MessageBox(NULL, L"タスクトレイアイコンの作成に失敗しました", L"エラー", MB_OK | MB_ICONERROR);
        exit(1);
    }
}

void ShowContextMenu(HWND hwnd, POINT pt)
{
    HMENU hMenu = CreatePopupMenu();
    if (hMenu) {
        InsertMenu(hMenu, -1, MF_BYPOSITION | MF_STRING, ID_MENU_TOGGLE,
            g_isSyncEnabled ? L"同期を停止" : L"同期を開始");
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
    if (g_isSyncEnabled) {
        g_brightnessManager->StopSync();
        g_isSyncEnabled = false;
    }
    else {
        g_brightnessManager->StartSync();
        g_isSyncEnabled = true;
    }
}

void Cleanup()
{
    // タスクトレイアイコンの削除
    Shell_NotifyIcon(NIM_DELETE, &g_nid);

    // BrightnessManagerの停止
    if (g_brightnessManager) {
        g_brightnessManager->StopSync();
    }
}
