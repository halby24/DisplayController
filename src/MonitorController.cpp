#include "MonitorController.h"
#include <memory>
#include <physicalmonitorenumerationapi.h>
#include <highlevelmonitorconfigurationapi.h>
#include <shlobj_core.h>
#include <fstream>
#include <algorithm>
#pragma comment(lib, "Dxva2.lib")
#pragma comment(lib, "Shell32.lib")

MonitorController::MonitorController()
{
    // 設定ファイルのベースディレクトリを設定
    wchar_t appDataPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, appDataPath))) {
        m_settingsPath = std::filesystem::path(appDataPath) / L"DisplayController" / L"Settings";
        std::filesystem::create_directories(m_settingsPath);
    }
}

MonitorController::~MonitorController() noexcept = default;

// IMonitorManager implementation
std::vector<MonitorId> MonitorController::EnumerateMonitors()
{
    std::vector<MonitorId> monitors;
    if (!EnumDisplayMonitors(nullptr, nullptr,
        [](HMONITOR hMonitor, HDC, LPRECT, LPARAM dwData) -> BOOL {
            auto monitors = reinterpret_cast<std::vector<MonitorId>*>(dwData);
            monitors->push_back(hMonitor);
            return TRUE;
        },
        reinterpret_cast<LPARAM>(&monitors)))
    {
        throw WindowsApiException("Failed to enumerate monitors: " + std::to_string(GetLastError()));
    }
    return monitors;
}

bool MonitorController::GetMonitorInfo(MonitorId id, std::wstring& name, bool& isPrimary)
{
    MONITORINFOEXW monitorInfo = { sizeof(MONITORINFOEXW) };
    if (!GetMonitorInfoW(id, &monitorInfo)) {
        return false;
    }

    name = monitorInfo.szDevice;
    isPrimary = (monitorInfo.dwFlags & MONITORINFOF_PRIMARY) != 0;
    return true;
}

// IBrightnessMapper implementation
int MonitorController::MapBrightness(MonitorId id, int normalizedBrightness)
{
    auto it = m_mappingConfigs.find(id);
    if (it == m_mappingConfigs.end()) {
        // デフォルトのマッピング（線形）を使用
        return normalizedBrightness;
    }

    const auto& config = it->second;

    // 範囲チェック
    normalizedBrightness = std::clamp(normalizedBrightness, 0, 100);

    // マッピングポイントがない場合は線形マッピング
    if (config.mappingPoints.empty()) {
        return config.minBrightness +
            (config.maxBrightness - config.minBrightness) * normalizedBrightness / 100;
    }

    // カスタムマッピングポイントを使用
    auto points = config.mappingPoints;
    std::sort(points.begin(), points.end());

    // 最小値以下または最大値以上の場合
    if (normalizedBrightness <= points.front().first) {
        return points.front().second;
    }
    if (normalizedBrightness >= points.back().first) {
        return points.back().second;
    }

    // 区間を見つけて線形補間
    for (size_t i = 1; i < points.size(); ++i) {
        if (normalizedBrightness <= points[i].first) {
            const auto& p1 = points[i - 1];
            const auto& p2 = points[i];
            float t = static_cast<float>(normalizedBrightness - p1.first) /
                     static_cast<float>(p2.first - p1.first);
            return p1.second + static_cast<int>(t * (p2.second - p1.second));
        }
    }

    return normalizedBrightness; // フォールバック
}

void MonitorController::SetMappingConfig(MonitorId id, const MappingConfig& config)
{
    m_mappingConfigs[id] = config;
}

MappingConfig MonitorController::GetMappingConfig(MonitorId id)
{
    auto it = m_mappingConfigs.find(id);
    if (it != m_mappingConfigs.end()) {
        return it->second;
    }
    return MappingConfig(); // デフォルト設定を返す
}

// IMonitorController implementation
bool MonitorController::SetBrightness(MonitorId id, int brightness)
{
    if (brightness < 0 || brightness > 100) {
        return false;
    }

    try {
        // Get physical monitor handle using RAII
        std::unique_ptr<void, decltype(&DestroyPhysicalMonitor)> handle(
            GetPhysicalMonitorHandle(id),
            DestroyPhysicalMonitor
        );

        // Get brightness range
        DWORD minBrightness = 0, currentBrightness = 0, maxBrightness = 0;
        if (!GetMonitorBrightness(handle.get(), &minBrightness, &currentBrightness, &maxBrightness)) {
            return false;
        }

        // マッピングされた輝度値を計算
        int mappedBrightness = MapBrightness(id, brightness);

        // Convert percentage to actual brightness value
        DWORD newBrightness = minBrightness +
            static_cast<DWORD>((maxBrightness - minBrightness) * mappedBrightness / 100.0);

        // Set new brightness
        return SetMonitorBrightness(handle.get(), newBrightness);
    }
    catch (const WindowsApiException&) {
        return false;
    }
}

int MonitorController::GetBrightness(MonitorId id)
{
    try {
        // Get physical monitor handle using RAII
        std::unique_ptr<void, decltype(&DestroyPhysicalMonitor)> handle(
            GetPhysicalMonitorHandle(id),
            DestroyPhysicalMonitor
        );

        // Get current brightness
        DWORD minBrightness = 0, currentBrightness = 0, maxBrightness = 0;
        if (!GetMonitorBrightness(handle.get(), &minBrightness, &currentBrightness, &maxBrightness)) {
            return 0;
        }

        // Convert to percentage (0-100)
        if (maxBrightness > minBrightness) {
            return static_cast<int>((currentBrightness - minBrightness) * 100 / (maxBrightness - minBrightness));
        }

        return 0;
    }
    catch (const WindowsApiException&) {
        return 0;
    }
}

bool MonitorController::SetUnifiedBrightness(int normalizedBrightness)
{
    if (normalizedBrightness < 0 || normalizedBrightness > 100) {
        return false;
    }

    bool success = true;
    auto monitors = EnumerateMonitors();
    for (const auto& id : monitors) {
        if (!SetBrightness(id, normalizedBrightness)) {
            success = false;
        }
    }
    return success;
}

// Helper functions
std::vector<MonitorController::MonitorInfo> MonitorController::GetMonitors()
{
    std::vector<MonitorInfo> monitors;
    if (!EnumDisplayMonitors(nullptr, nullptr, MonitorEnumProc, reinterpret_cast<LPARAM>(&monitors))) {
        throw WindowsApiException("Failed to enumerate monitors: " + std::to_string(GetLastError()));
    }
    return monitors;
}

BOOL CALLBACK MonitorController::MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
    auto monitors = reinterpret_cast<std::vector<MonitorInfo>*>(dwData);

    MONITORINFOEXW monitorInfo = { sizeof(MONITORINFOEXW) };
    if (!GetMonitorInfoW(hMonitor, &monitorInfo)) {
        return TRUE; // Continue enumeration even if we fail to get info for one monitor
    }

    MonitorInfo info;
    info.deviceName = monitorInfo.szDevice;
    info.isPrimary = (monitorInfo.dwFlags & MONITORINFOF_PRIMARY) != 0;
    info.bounds = monitorInfo.rcMonitor;
    info.id = hMonitor;

    monitors->push_back(info);
    return TRUE;
}

HANDLE MonitorController::GetPhysicalMonitorHandle(MonitorId id)
{
    DWORD numberOfPhysicalMonitors = 0;
    if (!GetNumberOfPhysicalMonitorsFromHMONITOR(id, &numberOfPhysicalMonitors)) {
        throw WindowsApiException("Failed to get number of physical monitors: " + std::to_string(GetLastError()));
    }

    if (numberOfPhysicalMonitors == 0) {
        throw WindowsApiException("No physical monitors found");
    }

    std::vector<PHYSICAL_MONITOR> physicalMonitors(numberOfPhysicalMonitors);
    if (!GetPhysicalMonitorsFromHMONITOR(id, numberOfPhysicalMonitors, physicalMonitors.data())) {
        throw WindowsApiException("Failed to get physical monitors: " + std::to_string(GetLastError()));
    }

    return physicalMonitors[0].hPhysicalMonitor;
}

MonitorController::MonitorCapabilities MonitorController::GetMonitorCapabilities(MonitorId id)
{
    MonitorCapabilities caps = {};
    caps.supportsBrightness = false;
    caps.supportsContrast = false;
    caps.supportsColorTemperature = false;
    caps.technologyType = "Unknown";
    caps.colorTemperature = 0;
    caps.displaySize = { 0, 0 };

    try {
        // Get physical monitor handle using RAII
        std::unique_ptr<void, decltype(&DestroyPhysicalMonitor)> handle(
            GetPhysicalMonitorHandle(id),
            DestroyPhysicalMonitor
        );

        // Test brightness control
        DWORD minValue = 0, currentValue = 0, maxValue = 0;
        if (GetMonitorBrightness(handle.get(), &minValue, &currentValue, &maxValue)) {
            caps.supportsBrightness = true;
        }

        // Test contrast control
        if (GetMonitorContrast(handle.get(), &minValue, &currentValue, &maxValue)) {
            caps.supportsContrast = true;
        }

        // Get display size using DPI
        MONITORINFOEXW monitorInfo = { sizeof(MONITORINFOEXW) };
        if (GetMonitorInfoW(id, &monitorInfo)) {
            HDC hdc = CreateDCW(L"DISPLAY", monitorInfo.szDevice, nullptr, nullptr);
            if (hdc) {
                caps.displaySize.cx = GetDeviceCaps(hdc, HORZSIZE);  // Physical width (mm)
                caps.displaySize.cy = GetDeviceCaps(hdc, VERTSIZE);  // Physical height (mm)
                DeleteDC(hdc);
            }
        }
    }
    catch (const WindowsApiException&) {
        // If we fail to get the physical monitor handle, return the empty capabilities
    }

    return caps;
}

void MonitorController::GetDetailedMonitorInfo(MonitorInfo& info)
{
    // 詳細情報の取得は必要に応じて実装
    // 基本的な情報のみを設定
    info.manufacturerName = L"Unknown";
    info.productCode = L"Unknown";
    info.serialNumber = L"Unknown";
    info.friendlyName = L"Unknown Monitor";
}

std::wstring MonitorController::GetSettingsFilePath(const MonitorInfo& info) const
{
    // デバイス名を使用してユニークなファイル名を生成
    std::wstring filename = L"monitor_" + std::to_wstring(reinterpret_cast<uintptr_t>(info.id)) + L".json";
    return (m_settingsPath / filename).wstring();
}

void MonitorController::SaveMonitorSettings(const MonitorInfo& info, const MonitorSettings& settings)
{
    std::wstring filepath = GetSettingsFilePath(info);
    std::ofstream file(filepath);
    if (!file) {
        throw DisplayControllerException("Failed to open settings file for writing");
    }

    // JSON形式で設定を保存
    file << "{\n";
    file << "  \"brightness\": " << settings.brightness << ",\n";
    file << "  \"contrast\": " << settings.contrast << ",\n";
    file << "  \"colorTemperature\": " << settings.colorTemperature << "\n";
    file << "}\n";
}

MonitorController::MonitorSettings MonitorController::LoadMonitorSettings(const MonitorInfo& info)
{
    std::wstring filepath = GetSettingsFilePath(info);
    std::ifstream file(filepath);
    MonitorSettings settings = {};

    if (file) {
        std::string line;
        std::string json;
        while (std::getline(file, line)) {
            json += line;
        }

        // 簡易的なJSONパース（実際の実装ではJSON parserライブラリを使用することを推奨）
        size_t pos;
        if ((pos = json.find("\"brightness\":")) != std::string::npos) {
            settings.brightness = std::stoi(json.substr(pos + 12));
        }
        if ((pos = json.find("\"contrast\":")) != std::string::npos) {
            settings.contrast = std::stoi(json.substr(pos + 10));
        }
        if ((pos = json.find("\"colorTemperature\":")) != std::string::npos) {
            settings.colorTemperature = std::stoul(json.substr(pos + 17));
        }
    }

    return settings;
}
