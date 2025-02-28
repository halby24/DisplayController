#include "MonitorController.h"
#include "BrightnessSync.h"
#include <memory>
#include <physicalmonitorenumerationapi.h>
#include <highlevelmonitorconfigurationapi.h>
#include <shlobj_core.h>
#include <fstream>
#pragma comment(lib, "Dxva2.lib")
#pragma comment(lib, "Shell32.lib")

MonitorController::MonitorController()
    : m_pWbemServices(nullptr)
    , m_wmiInitialized(false)
{
    // 設定ファイルのベースディレクトリを設定
    wchar_t appDataPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, appDataPath))) {
        m_settingsPath = std::filesystem::path(appDataPath) / L"DisplayController" / L"Settings";
        std::filesystem::create_directories(m_settingsPath);
    }

    InitializeWMI();

    // 輝度同期機能の初期化
    m_brightnessSync = std::make_unique<BrightnessSync>(*this);
}

MonitorController::~MonitorController()
{
    CleanupWMI();
}

void MonitorController::InitializeWMI()
{
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        throw WindowsApiException("Failed to initialize COM");
    }

    IWbemLocator* pWbemLocator = nullptr;
    hr = CoCreateInstance(CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER,
        IID_IWbemLocator, reinterpret_cast<void**>(&pWbemLocator));

    if (FAILED(hr)) {
        CoUninitialize();
        throw WindowsApiException("Failed to create WbemLocator");
    }

    hr = pWbemLocator->ConnectServer(_bstr_t(L"ROOT\\WMI"),
        nullptr, nullptr, nullptr, 0, nullptr, nullptr, &m_pWbemServices);

    pWbemLocator->Release();

    if (FAILED(hr)) {
        CoUninitialize();
        throw WindowsApiException("Failed to connect to WMI");
    }

    hr = CoSetProxyBlanket(m_pWbemServices, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE,
        nullptr, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE,
        nullptr, EOAC_NONE);

    if (FAILED(hr)) {
        m_pWbemServices->Release();
        m_pWbemServices = nullptr;
        CoUninitialize();
        throw WindowsApiException("Failed to set proxy blanket");
    }

    m_wmiInitialized = true;
}

void MonitorController::CleanupWMI()
{
    if (m_pWbemServices) {
        m_pWbemServices->Release();
        m_pWbemServices = nullptr;
    }

    if (m_wmiInitialized) {
        CoUninitialize();
        m_wmiInitialized = false;
    }
}

std::vector<MonitorController::MonitorInfo> MonitorController::GetMonitors()
{
    std::vector<MonitorInfo> monitors;
    if (!EnumDisplayMonitors(nullptr, nullptr, MonitorEnumProc, reinterpret_cast<LPARAM>(&monitors)))
    {
        DWORD error = GetLastError();
        throw WindowsApiException("Failed to enumerate monitors: " + std::to_string(error));
    }
    return monitors;
}

BOOL CALLBACK MonitorController::MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
    auto monitors = reinterpret_cast<std::vector<MonitorInfo>*>(dwData);

    MONITORINFOEXW monitorInfo = { sizeof(MONITORINFOEXW) };
    if (!GetMonitorInfoW(hMonitor, &monitorInfo))
    {
        return TRUE; // Continue enumeration even if we fail to get info for one monitor
    }

    MonitorInfo info;
    info.deviceName = monitorInfo.szDevice;
    info.isPrimary = (monitorInfo.dwFlags & MONITORINFOF_PRIMARY) != 0;
    info.bounds = monitorInfo.rcMonitor;
    info.hMonitor = hMonitor;  // Store HMONITOR

    monitors->push_back(info);
    return TRUE;
}

HANDLE MonitorController::GetPhysicalMonitorHandle(HMONITOR hMonitor)
{
    DWORD numberOfPhysicalMonitors = 0;
    if (!GetNumberOfPhysicalMonitorsFromHMONITOR(hMonitor, &numberOfPhysicalMonitors))
    {
        DWORD error = GetLastError();
        throw WindowsApiException("Failed to get number of physical monitors: " + std::to_string(error));
    }

    if (numberOfPhysicalMonitors == 0)
    {
        throw WindowsApiException("No physical monitors found");
    }

    std::vector<PHYSICAL_MONITOR> physicalMonitors(numberOfPhysicalMonitors);
    if (!GetPhysicalMonitorsFromHMONITOR(hMonitor, numberOfPhysicalMonitors, physicalMonitors.data()))
    {
        DWORD error = GetLastError();
        throw WindowsApiException("Failed to get physical monitors: " + std::to_string(error));
    }

    // We'll use the first physical monitor
    return physicalMonitors[0].hPhysicalMonitor;
}

int MonitorController::GetBrightness(HMONITOR hMonitor)
{
    // Get physical monitor handle using RAII
    std::unique_ptr<void, decltype(&DestroyPhysicalMonitor)> handle(
        GetPhysicalMonitorHandle(hMonitor),
        DestroyPhysicalMonitor
    );

    // Get current brightness
    DWORD minBrightness = 0, currentBrightness = 0, maxBrightness = 0;
    if (!GetMonitorBrightness(handle.get(), &minBrightness, &currentBrightness, &maxBrightness))
    {
        DWORD error = GetLastError();
        throw WindowsApiException("Failed to get monitor brightness: " + std::to_string(error));
    }

    // Convert to percentage (0-100)
    if (maxBrightness > minBrightness)
    {
        return static_cast<int>((currentBrightness - minBrightness) * 100 / (maxBrightness - minBrightness));
    }

    return 0;
}

MonitorController::MonitorCapabilities MonitorController::GetMonitorCapabilities(HMONITOR hMonitor)
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
            GetPhysicalMonitorHandle(hMonitor),
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
        if (GetMonitorInfoW(hMonitor, &monitorInfo)) {
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
    if (!m_wmiInitialized || !m_pWbemServices) {
        throw WindowsApiException("WMI is not initialized");
    }

    // WMIクエリを実行してモニター情報を取得
    IEnumWbemClassObject* pEnumerator = nullptr;
    HRESULT hr = m_pWbemServices->ExecQuery(
        bstr_t("WQL"),
        bstr_t("SELECT * FROM WMIMonitorID"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        nullptr,
        &pEnumerator);

    if (FAILED(hr)) {
        throw WindowsApiException("Failed to execute WMI query");
    }

    IWbemClassObject* pClassObject = nullptr;
    ULONG uReturn = 0;

    while (pEnumerator) {
        hr = pEnumerator->Next(WBEM_INFINITE, 1, &pClassObject, &uReturn);
        if (uReturn == 0) break;

        VARIANT vtProp;

        // 製造元名
        if (SUCCEEDED(pClassObject->Get(L"ManufacturerName", 0, &vtProp, 0, 0))) {
            if (vtProp.vt == (VT_UI1 | VT_ARRAY)) {
                SAFEARRAY* pArray = vtProp.parray;
                BYTE* pData;
                SafeArrayAccessData(pArray, (void**)&pData);
                info.manufacturerName = std::wstring(reinterpret_cast<wchar_t*>(pData));
                SafeArrayUnaccessData(pArray);
            }
            VariantClear(&vtProp);
        }

        // 製品コード
        if (SUCCEEDED(pClassObject->Get(L"ProductCodeID", 0, &vtProp, 0, 0))) {
            if (vtProp.vt == (VT_UI1 | VT_ARRAY)) {
                SAFEARRAY* pArray = vtProp.parray;
                BYTE* pData;
                SafeArrayAccessData(pArray, (void**)&pData);
                info.productCode = std::wstring(reinterpret_cast<wchar_t*>(pData));
                SafeArrayUnaccessData(pArray);
            }
            VariantClear(&vtProp);
        }

        // シリアル番号
        if (SUCCEEDED(pClassObject->Get(L"SerialNumberID", 0, &vtProp, 0, 0))) {
            if (vtProp.vt == (VT_UI1 | VT_ARRAY)) {
                SAFEARRAY* pArray = vtProp.parray;
                BYTE* pData;
                SafeArrayAccessData(pArray, (void**)&pData);
                info.serialNumber = std::wstring(reinterpret_cast<wchar_t*>(pData));
                SafeArrayUnaccessData(pArray);
            }
            VariantClear(&vtProp);
        }

        pClassObject->Release();
    }

    pEnumerator->Release();

    // フレンドリーネームを生成
    info.friendlyName = info.manufacturerName + L" " + info.productCode;
}

std::wstring MonitorController::GetSettingsFilePath(const MonitorInfo& info) const
{
    // シリアル番号とプロダクトコードを使用してユニークなファイル名を生成
    std::wstring filename = info.productCode + L"_" + info.serialNumber + L".json";
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

bool MonitorController::SetBrightness(HMONITOR hMonitor, int brightness)
{
    if (brightness < 0 || brightness > 100) {
        return false;
    }

    try {
        // Get physical monitor handle using RAII
        std::unique_ptr<void, decltype(&DestroyPhysicalMonitor)> handle(
            GetPhysicalMonitorHandle(hMonitor),
            DestroyPhysicalMonitor
        );

        // Get brightness range
        DWORD minBrightness = 0, currentBrightness = 0, maxBrightness = 0;
        if (!GetMonitorBrightness(handle.get(), &minBrightness, &currentBrightness, &maxBrightness)) {
            return false;
        }

        // Convert percentage to actual brightness value
        DWORD newBrightness = minBrightness +
            static_cast<DWORD>((maxBrightness - minBrightness) * brightness / 100.0);

        // Set new brightness
        if (!SetMonitorBrightness(handle.get(), newBrightness)) {
            return false;
        }

        // システム輝度との同期
        if (m_brightnessSync) {
            m_brightnessSync->SyncToSystem(hMonitor, brightness);
        }

        return true;
    }
    catch (const WindowsApiException&) {
        return false;
    }
}

void MonitorController::EnableBrightnessSync(bool enable)
{
    if (m_brightnessSync) {
        m_brightnessSync->EnableSync(enable);
    }
}

bool MonitorController::IsBrightnessSyncEnabled() const
{
    return m_brightnessSync && m_brightnessSync->IsSyncEnabled();
}
