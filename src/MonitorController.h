#ifndef DISPLAYCONTROLLER_MONITOR_CONTROLLER_H
#define DISPLAYCONTROLLER_MONITOR_CONTROLLER_H

#include <windows.h>
#include <vector>
#include <string>
#include <stdexcept>
#include <wbemidl.h>
#include <comdef.h>
#include <filesystem>

// 前方宣言
class BrightnessSync;

class DisplayControllerException : public std::runtime_error
{
public:
    explicit DisplayControllerException(const std::string& message)
        : std::runtime_error(message) {}
};

class WindowsApiException : public DisplayControllerException
{
public:
    explicit WindowsApiException(const std::string& message)
        : DisplayControllerException(message) {}
};

class MonitorController
{
    friend class BrightnessSync;
public:
    struct MonitorInfo
    {
        std::wstring deviceName;
        bool isPrimary;
        RECT bounds;
        HMONITOR hMonitor;

        // 詳細な識別情報
        std::wstring manufacturerName;
        std::wstring productCode;
        std::wstring serialNumber;
        std::wstring friendlyName;
    };

    // モニター設定
    struct MonitorSettings
    {
        int brightness;
        int contrast;
        DWORD colorTemperature;
    };

    MonitorController();
    ~MonitorController() noexcept;

    std::vector<MonitorInfo> GetMonitors();
    int GetBrightness(HMONITOR hMonitor);
    bool SetBrightness(HMONITOR hMonitor, int brightness);

    // システム輝度同期
    void EnableBrightnessSync(bool enable);
    bool IsBrightnessSyncEnabled() const;

    // Monitor capabilities
    struct MonitorCapabilities {
        bool supportsBrightness;
        bool supportsContrast;
        bool supportsColorTemperature;
        std::string technologyType;
        DWORD colorTemperature;
        SIZE displaySize;  // in millimeters
    };

    MonitorCapabilities GetMonitorCapabilities(HMONITOR hMonitor);

    // 詳細情報の取得と設定の管理
    void GetDetailedMonitorInfo(MonitorInfo& info);
    void SaveMonitorSettings(const MonitorInfo& info, const MonitorSettings& settings);
    MonitorSettings LoadMonitorSettings(const MonitorInfo& info);
    std::wstring GetSettingsFilePath(const MonitorInfo& info) const;

private:
    // 輝度同期
    std::unique_ptr<BrightnessSync> m_brightnessSync;
    static BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData);
    HANDLE GetPhysicalMonitorHandle(HMONITOR hMonitor);

    // WMI関連
    void InitializeWMI();
    void CleanupWMI();
    IWbemServices* GetWbemServices() const { return m_pWbemServices; }
    IWbemServices* m_pWbemServices;
    bool m_wmiInitialized;

    // 設定ファイルのベースディレクトリ
    std::filesystem::path m_settingsPath;
};

#endif // DISPLAYCONTROLLER_MONITOR_CONTROLLER_H
