#include "MonitorController.h"
#include <windows.h>
#include <memory>
#include <physicalmonitorenumerationapi.h>
#include <highlevelmonitorconfigurationapi.h>
#include <shlobj_core.h>
#include <fstream>
#include <algorithm>
#pragma comment(lib, "Dxva2.lib")
#pragma comment(lib, "Shell32.lib")

// Windows API関数の宣言
extern "C" {
    BOOL WINAPI SetMonitorBrightness(HANDLE hMonitor, DWORD dwNewBrightness);
    BOOL WINAPI DestroyPhysicalMonitor(HANDLE hMonitor);
}

MonitorController::MonitorController()
{
    // 設定ファイルのベースディレクトリを設定
    wchar_t appDataPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, appDataPath))) {
        m_settingsPath = std::filesystem::path(appDataPath) / L"DisplayController" / L"Settings";
        std::filesystem::create_directories(m_settingsPath);
        LoadMappingConfigs(); // 設定ファイルからマッピング設定を読み込む
    }
}

MonitorController::~MonitorController() noexcept = default;

// IMonitorManager implementation
std::vector<MonitorId> MonitorController::EnumerateMonitors()
{
    std::vector<MonitorId> monitors;
    if (!::EnumDisplayMonitors(nullptr, nullptr,
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
    if (!::GetMonitorInfoW(id, reinterpret_cast<LPMONITORINFO>(&monitorInfo))) {
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
    SaveMappingConfig(id, config); // 設定をファイルに保存
}

std::wstring MonitorController::GetMappingConfigFilePath(MonitorId id) const
{
    // モニターIDを使用してユニークなファイル名を生成
    std::wstring filename = L"mapping_" + std::to_wstring(reinterpret_cast<uintptr_t>(id)) + L".json";
    return (m_settingsPath / filename).wstring();
}

void MonitorController::SaveMappingConfig(MonitorId id, const MappingConfig& config)
{
    try {
        std::filesystem::path filepath = GetMappingConfigFilePath(id);

        // nlohmann::jsonオブジェクトを作成
        nlohmann::json j;
        j["minBrightness"] = config.minBrightness;
        j["maxBrightness"] = config.maxBrightness;

        // マッピングポイントの配列を作成
        nlohmann::json points = nlohmann::json::array();
        for (const auto& point : config.mappingPoints) {
            points.push_back({
                {"input", point.first},
                {"output", point.second}
            });
        }
        j["mappingPoints"] = points;

        // JSONをファイルに書き込み（整形して見やすく）
        std::ofstream file(filepath);
        if (!file) {
            throw DisplayControllerException("Failed to open mapping config file for writing: " + filepath.string());
        }

        file << j.dump(2); // インデント2でフォーマット
        file.flush();

        if (!file) {
            throw DisplayControllerException("Failed to write mapping config to file: " + filepath.string());
        }
    }
    catch (const std::exception& e) {
        throw DisplayControllerException(std::string("Failed to save mapping config: ") + e.what());
    }
}
void MonitorController::LoadMappingConfigs()
{
    try {
        // 既存のモニターを列挙
        auto monitors = EnumerateMonitors();

        for (const auto& id : monitors) {
            std::filesystem::path filepath = GetMappingConfigFilePath(id);
            if (!std::filesystem::exists(filepath)) {
                continue; // 設定ファイルが存在しない場合はスキップ
            }

            std::ifstream file(filepath);
            if (!file) {
                continue; // ファイルが開けない場合はスキップ
            }

            try {
                // JSONをパース
                nlohmann::json j = nlohmann::json::parse(file);

                MappingConfig config;

                // 基本設定の読み込み
                if (j.contains("minBrightness")) {
                    config.minBrightness = j["minBrightness"].get<int>();
                }
                if (j.contains("maxBrightness")) {
                    config.maxBrightness = j["maxBrightness"].get<int>();
                }

                // マッピングポイントの読み込み
                if (j.contains("mappingPoints") && j["mappingPoints"].is_array()) {
                    for (const auto& point : j["mappingPoints"]) {
                        if (point.contains("input") && point.contains("output")) {
                            config.mappingPoints.emplace_back(
                                point["input"].get<int>(),
                                point["output"].get<int>()
                            );
                        }
                    }
                }

                m_mappingConfigs[id] = config;
            }
            catch (const nlohmann::json::exception&) {
                // JSONパースエラーの場合は該当モニターの設定をスキップ
                continue;
            }
            catch (const std::exception&) {
                // その他のエラーの場合も該当モニターの設定をスキップ
                continue;
            }
        }
    }
    catch (const std::exception&) {
        // エラーが発生した場合は設定の読み込みを中止
    }
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
        if (!::GetMonitorBrightness(handle.get(), &minBrightness, &currentBrightness, &maxBrightness)) {
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
        if (!::GetMonitorBrightness(handle.get(), &minBrightness, &currentBrightness, &maxBrightness)) {
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
    if (!::GetMonitorInfoW(hMonitor, reinterpret_cast<LPMONITORINFO>(&monitorInfo))) {
        return TRUE; // Continue enumeration even if we fail to get info for one monitor
    }

    MonitorInfo info;
    info.deviceName = monitorInfo.szDevice;
    info.isPrimary = (monitorInfo.dwFlags & MONITORINFOF_PRIMARY) != 0;
    info.bounds = monitorInfo.rcMonitor;
    info.id = hMonitor;

    // 物理サイズを取得
    HDC hdc = ::CreateDCW(L"DISPLAY", monitorInfo.szDevice, nullptr, nullptr);
    if (hdc) {
        info.physicalSize.cx = ::GetDeviceCaps(hdc, HORZSIZE);  // 物理的な幅 (mm)
        info.physicalSize.cy = ::GetDeviceCaps(hdc, VERTSIZE);  // 物理的な高さ (mm)
        ::DeleteDC(hdc);
    } else {
        info.physicalSize = {0, 0};
    }

    monitors->push_back(info);
    return TRUE;
}

HANDLE MonitorController::GetPhysicalMonitorHandle(MonitorId id)
{
    DWORD numberOfPhysicalMonitors = 0;
    if (!::GetNumberOfPhysicalMonitorsFromHMONITOR(id, &numberOfPhysicalMonitors)) {
        throw WindowsApiException("Failed to get number of physical monitors: " + std::to_string(::GetLastError()));
    }

    if (numberOfPhysicalMonitors == 0) {
        throw WindowsApiException("No physical monitors found");
    }

    std::vector<PHYSICAL_MONITOR> physicalMonitors(numberOfPhysicalMonitors);
    if (!::GetPhysicalMonitorsFromHMONITOR(id, numberOfPhysicalMonitors, physicalMonitors.data())) {
        throw WindowsApiException("Failed to get physical monitors: " + std::to_string(::GetLastError()));
    }

    HANDLE handle = physicalMonitors[0].hPhysicalMonitor;

    // Clean up the remaining handles if any
    for (DWORD i = 1; i < numberOfPhysicalMonitors; ++i) {
        DestroyPhysicalMonitor(physicalMonitors[i].hPhysicalMonitor);
    }

    return handle;
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
        if (::GetMonitorBrightness(handle.get(), &minValue, &currentValue, &maxValue)) {
            caps.supportsBrightness = true;
        }

        // Test contrast control
        if (::GetMonitorContrast(handle.get(), &minValue, &currentValue, &maxValue)) {
            caps.supportsContrast = true;
        }

        // Get display size using DPI
        MONITORINFOEXW monitorInfo = { sizeof(MONITORINFOEXW) };
        if (::GetMonitorInfoW(id, reinterpret_cast<LPMONITORINFO>(&monitorInfo))) {
            HDC hdc = ::CreateDCW(L"DISPLAY", monitorInfo.szDevice, nullptr, nullptr);
            if (hdc) {
                caps.displaySize.cx = ::GetDeviceCaps(hdc, HORZSIZE);  // Physical width (mm)
                caps.displaySize.cy = ::GetDeviceCaps(hdc, VERTSIZE);  // Physical height (mm)
                ::DeleteDC(hdc);
            }
        }
    }
    catch (const WindowsApiException&) {
        // If we fail to get the physical monitor handle, return the empty capabilities
    }

    return caps;
}

bool MonitorController::GetMonitorEDID(const std::wstring& deviceName, std::vector<BYTE>& edidData)
{
    HDEVINFO deviceInfo = SetupDiGetClassDevsW(&GUID_DEVCLASS_MONITOR, nullptr, nullptr, DIGCF_PRESENT);
    if (deviceInfo == INVALID_HANDLE_VALUE) {
        return false;
    }

    std::unique_ptr<void, decltype(&SetupDiDestroyDeviceInfoList)> deviceInfoGuard(
        deviceInfo, SetupDiDestroyDeviceInfoList);

    SP_DEVINFO_DATA deviceInfoData = { sizeof(SP_DEVINFO_DATA) };
    for (DWORD i = 0; SetupDiEnumDeviceInfo(deviceInfo, i, &deviceInfoData); i++) {
        DWORD propertyType;
        BYTE buffer[2048];
        DWORD size = sizeof(buffer);

        if (SetupDiGetDeviceRegistryPropertyW(deviceInfo, &deviceInfoData, SPDRP_HARDWAREID,
            &propertyType, buffer, size, &size)) {
            std::wstring hardwareId = reinterpret_cast<wchar_t*>(buffer);
            if (hardwareId.find(deviceName) != std::wstring::npos) {
                // EDIDデータを取得
                if (SetupDiGetDeviceRegistryPropertyW(deviceInfo, &deviceInfoData, SPDRP_PHYSICAL_DEVICE_OBJECT_NAME,
                    &propertyType, buffer, size, &size)) {
                    edidData.assign(buffer, buffer + size);
                    return true;
                }
            }
        }
    }
    return false;
}

bool MonitorController::ParseEDIDManufacturerInfo(const std::vector<BYTE>& edidData,
    std::wstring& manufacturer, std::wstring& productCode)
{
    if (edidData.size() < 128) {
        return false;
    }

    // 製造元IDを解析（3文字のASCIIコード）
    unsigned short manufacturerId = (edidData[8] << 8) | edidData[9];
    wchar_t manufacturerCode[4] = {
        static_cast<wchar_t>((manufacturerId >> 10 & 0x1F) + 'A' - 1),
        static_cast<wchar_t>((manufacturerId >> 5 & 0x1F) + 'A' - 1),
        static_cast<wchar_t>((manufacturerId & 0x1F) + 'A' - 1),
        L'\0'
    };
    manufacturer = manufacturerCode;

    // 製品コードを16進数で取得
    unsigned short productCodeValue = (edidData[11] << 8) | edidData[10];
    std::wstringstream ss;
    ss << std::hex << std::uppercase << std::setfill(L'0') << std::setw(4) << productCodeValue;
    productCode = ss.str();

    return true;
}

std::wstring MonitorController::ConvertSizeToInches(const SIZE& sizeInMm)
{
    // 対角線の長さをインチに変換
    double diagonalMm = std::sqrt(
        static_cast<double>(sizeInMm.cx) * sizeInMm.cx +
        static_cast<double>(sizeInMm.cy) * sizeInMm.cy
    );
    double diagonalInches = diagonalMm / 25.4; // mmからインチへの変換

    // 整数に丸める
    int inches = static_cast<int>(std::round(diagonalInches));
    return std::to_wstring(inches) + L"inch";
}

std::wstring MonitorController::GetMonitorRoleInfo(const MonitorInfo& info)
{
    std::wstring role;
    if (info.isPrimary) {
        role = L"Primary";
    }
    return role;
}

std::wstring MonitorController::GenerateHumanReadableName(const MonitorInfo& info)
{
    std::wstringstream nameStream;

    // 製造元名とモデル名を追加
    if (info.manufacturerName != L"Unknown") {
        nameStream << info.manufacturerName;
        if (info.productCode != L"Unknown") {
            nameStream << L" " << info.productCode;
        }
    } else {
        nameStream << L"Display " << std::to_wstring(m_nameCounters[L"Generic"]++);
    }

    // サイズ情報を追加
    if (info.physicalSize.cx > 0 && info.physicalSize.cy > 0) {
        nameStream << L" " << ConvertSizeToInches(info.physicalSize);
    }

    // 役割情報を追加
    std::wstring role = GetMonitorRoleInfo(info);
    if (!role.empty()) {
        nameStream << L" " << role;
    }

    // 重複チェックと番号付加
    std::wstring baseName = nameStream.str();
    std::wstring finalName = baseName;
    int counter = 2;
    while (m_nameCounters[finalName] > 0) {
        finalName = baseName + L" (" + std::to_wstring(counter++) + L")";
    }
    m_nameCounters[finalName]++;

    return finalName;
}

void MonitorController::GetDetailedMonitorInfo(MonitorInfo& info)
{
    // 物理サイズを取得
    auto caps = GetMonitorCapabilities(info.id);
    info.physicalSize = caps.displaySize;

    // EDID情報を取得
    std::vector<BYTE> edidData;
    if (GetMonitorEDID(info.deviceName, edidData)) {
        ParseEDIDManufacturerInfo(edidData, info.manufacturerName, info.productCode);
    } else {
        info.manufacturerName = L"Unknown";
        info.productCode = L"Unknown";
    }

    // シリアル番号は現時点では未実装
    info.serialNumber = L"Unknown";

    // フレンドリーネームを生成
    info.friendlyName = GenerateHumanReadableName(info);
    info.humanReadableName = info.friendlyName;
}

std::wstring MonitorController::GetSettingsFilePath(const MonitorInfo& info) const
{
    // デバイス名を使用してユニークなファイル名を生成
    std::wstring filename = L"monitor_" + std::to_wstring(reinterpret_cast<uintptr_t>(info.id)) + L".json";
    return (m_settingsPath / filename).wstring();
}

void MonitorController::SaveMonitorSettings(const MonitorInfo& info, const MonitorSettings& settings)
{
    try {
        std::filesystem::path filepath = GetSettingsFilePath(info);

        // nlohmann::jsonオブジェクトを作成
        nlohmann::json j = {
            {"brightness", settings.brightness},
            {"contrast", settings.contrast},
            {"colorTemperature", settings.colorTemperature}
        };

        // JSONをファイルに書き込み（整形して見やすく）
        std::ofstream file(filepath);
        if (!file) {
            throw DisplayControllerException("Failed to open settings file for writing: " + filepath.string());
        }

        file << j.dump(2); // インデント2でフォーマット
        file.flush();

        if (!file) {
            throw DisplayControllerException("Failed to write settings to file: " + filepath.string());
        }
    }
    catch (const std::exception& e) {
        throw DisplayControllerException(std::string("Failed to save monitor settings: ") + e.what());
    }
}

MonitorController::MonitorSettings MonitorController::LoadMonitorSettings(const MonitorInfo& info)
{
    MonitorSettings settings = {};
    try {
        std::filesystem::path filepath = GetSettingsFilePath(info);
        if (!std::filesystem::exists(filepath)) {
            return settings; // ファイルが存在しない場合はデフォルト設定を返す
        }

        std::ifstream file(filepath);
        if (!file) {
            throw DisplayControllerException("Failed to open settings file for reading: " + filepath.string());
        }

        try {
            // JSONをパース
            nlohmann::json j = nlohmann::json::parse(file);

            // 設定を読み込み
            if (j.contains("brightness")) {
                settings.brightness = j["brightness"].get<int>();
            }
            if (j.contains("contrast")) {
                settings.contrast = j["contrast"].get<int>();
            }
            if (j.contains("colorTemperature")) {
                settings.colorTemperature = j["colorTemperature"].get<DWORD>();
            }
        }
        catch (const nlohmann::json::exception& e) {
            throw DisplayControllerException(std::string("Failed to parse settings JSON: ") + e.what());
        }
    }
    catch (const std::exception& e) {
        // エラーをログに記録するなどの処理を追加可能
        // ここではデフォルト設定を返す
        return settings;
    }

    return settings;
}
