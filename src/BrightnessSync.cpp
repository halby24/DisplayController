#include "BrightnessSync.h"
#include <fstream>
#include <shlobj_core.h>
#include <iostream>

BrightnessSync::BrightnessSync(MonitorController& monitorController)
    : m_monitorController(monitorController)
    , m_syncEnabled(false)
    , m_isSyncing(false)
{
    try {
        // MonitorControllerからWMIサービスを取得
        IWbemServices* wmiServices = monitorController.GetWbemServices();
        if (!wmiServices) {
            throw std::runtime_error("Failed to get WMI services from MonitorController");
        }

        // 既存のWMIサービスを使用してSystemBrightnessMonitorを初期化
        m_systemMonitor = std::make_unique<SystemBrightnessMonitor>(wmiServices);
        m_systemMonitor->SetBrightnessChangeCallback(
            [this](int brightness) { OnSystemBrightnessChanged(brightness); }
        );

        LoadSyncState();
        if (m_syncEnabled) {
            m_systemMonitor->StartMonitoring();
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to initialize BrightnessSync: " << e.what() << std::endl;
        throw;
    }
}

BrightnessSync::~BrightnessSync()
{
    if (m_syncEnabled) {
        SaveSyncState();
    }
}

void BrightnessSync::EnableSync(bool enable)
{
    if (m_syncEnabled == enable) return;

    m_syncEnabled = enable;
    if (enable) {
        m_systemMonitor->StartMonitoring();
        // 初期同期：現在のシステム輝度をモニターに反映
        int systemBrightness = m_systemMonitor->GetCurrentBrightness();
        if (systemBrightness >= 0) {
            SyncToMonitor(systemBrightness);
        }
    } else {
        m_systemMonitor->StopMonitoring();
    }

    SaveSyncState();
}

bool BrightnessSync::IsSyncEnabled() const
{
    return m_syncEnabled;
}

void BrightnessSync::SyncToSystem(HMONITOR hMonitor, int brightness)
{
    if (!m_syncEnabled || m_isSyncing) return;

    m_isSyncing = true;
    try {
        int systemBrightness = MonitorToSystemBrightness(brightness);
        m_systemMonitor->SetBrightness(systemBrightness);
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to sync brightness to system: " << e.what() << std::endl;
    }
    m_isSyncing = false;
}

void BrightnessSync::SyncToMonitor(int systemBrightness)
{
    if (!m_syncEnabled || m_isSyncing) return;

    m_isSyncing = true;
    try {
        int monitorBrightness = SystemToMonitorBrightness(systemBrightness);
        auto monitors = m_monitorController.GetMonitors();
        for (const auto& monitor : monitors) {
            // モニターの現在の輝度を取得
            int currentBrightness = m_monitorController.GetBrightness(monitor.hMonitor);
            if (currentBrightness != monitorBrightness) {
                // TODO: SetBrightness メソッドを MonitorController に追加する必要があります
                // m_monitorController.SetBrightness(monitor.hMonitor, monitorBrightness);
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to sync brightness to monitors: " << e.what() << std::endl;
    }
    m_isSyncing = false;
}

void BrightnessSync::OnSystemBrightnessChanged(int newBrightness)
{
    if (!m_syncEnabled || m_isSyncing) return;
    SyncToMonitor(newBrightness);
}

int BrightnessSync::MonitorToSystemBrightness(int monitorBrightness) const
{
    // モニターの輝度値（0-100）をシステムの輝度値（0-100）に変換
    // 現在は1:1のマッピングを使用
    return std::clamp(monitorBrightness, 0, 100);
}

int BrightnessSync::SystemToMonitorBrightness(int systemBrightness) const
{
    // システムの輝度値（0-100）をモニターの輝度値（0-100）に変換
    // 現在は1:1のマッピングを使用
    return std::clamp(systemBrightness, 0, 100);
}

std::filesystem::path BrightnessSync::GetSettingsPath() const
{
    wchar_t appDataPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, appDataPath))) {
        return std::filesystem::path(appDataPath) / L"DisplayController" / L"brightness_sync.json";
    }
    return std::filesystem::path();
}

void BrightnessSync::SaveSyncState()
{
    auto settingsPath = GetSettingsPath();
    if (settingsPath.empty()) return;

    // 親ディレクトリが存在することを確認
    std::filesystem::create_directories(settingsPath.parent_path());

    std::ofstream file(settingsPath);
    if (!file) return;

    // 設定をJSON形式で保存
    file << "{\n";
    file << "  \"syncEnabled\": " << (m_syncEnabled ? "true" : "false") << "\n";
    file << "}\n";
}

void BrightnessSync::LoadSyncState()
{
    auto settingsPath = GetSettingsPath();
    if (settingsPath.empty()) return;

    std::ifstream file(settingsPath);
    if (!file) return;

    std::string line;
    std::string json;
    while (std::getline(file, line)) {
        json += line;
    }

    // 簡易的なJSONパース
    if (json.find("\"syncEnabled\": true") != std::string::npos) {
        m_syncEnabled = true;
    } else {
        m_syncEnabled = false;
    }
}
