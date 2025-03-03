#ifndef DISPLAYCONTROLLER_MONITOR_CONTROLLER_H
#define DISPLAYCONTROLLER_MONITOR_CONTROLLER_H

// DLLエクスポート/インポートマクロ
#ifdef DISPLAYCONTROLLER_EXPORTS
    #define DISPLAYCONTROLLER_API __declspec(dllexport)
#else
    #define DISPLAYCONTROLLER_API __declspec(dllimport)
#endif

// インターフェースのDLLエクスポート設定
#define DISPLAYCONTROLLER_INTERFACE class DISPLAYCONTROLLER_API

#include <windows.h>
#include <vector>
#include <string>
#include <stdexcept>
#include <filesystem>
#include <memory>
#include <map>
#include <sstream>
#include <iomanip>
#include <setupapi.h>
#include <devguid.h>
#include <nlohmann/json.hpp>

#pragma comment(lib, "Setupapi.lib")

// 例外クラス
class DISPLAYCONTROLLER_API DisplayControllerException : public std::runtime_error {
public:
    explicit DisplayControllerException(const std::string& message)
        : std::runtime_error(message) {}
};

class DISPLAYCONTROLLER_API WindowsApiException : public DisplayControllerException {
public:
    explicit WindowsApiException(const std::string& message)
        : DisplayControllerException(message) {}
};

// モニターID型
using MonitorId = HMONITOR;

// 輝度マッピング設定
struct MappingConfig {
    int minBrightness;
    int maxBrightness;
    std::vector<std::pair<int, int>> mappingPoints;

    MappingConfig() : minBrightness(0), maxBrightness(100) {}
};

// モニター管理インターフェース
DISPLAYCONTROLLER_INTERFACE IMonitorManager {
public:
    virtual ~IMonitorManager() = default;
    virtual std::vector<MonitorId> EnumerateMonitors() = 0;
    virtual bool GetMonitorInfo(MonitorId id, std::wstring& name, bool& isPrimary) = 0;
};

// 輝度マッピングインターフェース
DISPLAYCONTROLLER_INTERFACE IBrightnessMapper {
public:
    virtual ~IBrightnessMapper() = default;
    virtual int MapBrightness(MonitorId id, int normalizedBrightness) = 0;
    virtual void SetMappingConfig(MonitorId id, const MappingConfig& config) = 0;
    virtual MappingConfig GetMappingConfig(MonitorId id) = 0;
};

// モニター制御インターフェース
DISPLAYCONTROLLER_INTERFACE IMonitorController {
public:
    virtual ~IMonitorController() = default;
    virtual bool SetBrightness(MonitorId id, int brightness) = 0;
    virtual int GetBrightness(MonitorId id) = 0;
    virtual bool SetUnifiedBrightness(int normalizedBrightness) = 0;
};

// MonitorController クラス
class DISPLAYCONTROLLER_API MonitorController : public IMonitorManager, public IBrightnessMapper, public IMonitorController {
public:
    struct MonitorInfo {
        std::wstring deviceName;
        bool isPrimary;
        RECT bounds;
        MonitorId id;

        // 詳細な識別情報
        std::wstring manufacturerName;
        std::wstring productCode;
        std::wstring serialNumber;
        std::wstring friendlyName;

        // 物理サイズ（mm）
        SIZE physicalSize;

        // 人間が識別可能な名前（例：DELL P2419H 24inch Primary）
        std::wstring humanReadableName;
    };

    // モニター設定
    struct MonitorSettings {
        int brightness;
        int contrast;
        DWORD colorTemperature;
    };

    // Monitor capabilities
    struct MonitorCapabilities {
        bool supportsBrightness;
        bool supportsContrast;
        bool supportsColorTemperature;
        std::string technologyType;
        DWORD colorTemperature;
        SIZE displaySize;  // in millimeters
    };

    MonitorController();
    ~MonitorController() noexcept override;

    // IMonitorManager の実装
    std::vector<MonitorId> EnumerateMonitors() override;
    bool GetMonitorInfo(MonitorId id, std::wstring& name, bool& isPrimary) override;

    // IBrightnessMapper の実装
    int MapBrightness(MonitorId id, int normalizedBrightness) override;
    void SetMappingConfig(MonitorId id, const MappingConfig& config) override;
    MappingConfig GetMappingConfig(MonitorId id) override;

    // IMonitorController の実装
    bool SetBrightness(MonitorId id, int brightness) override;
    int GetBrightness(MonitorId id) override;
    bool SetUnifiedBrightness(int normalizedBrightness) override;

    // 追加の機能
    std::vector<MonitorInfo> GetMonitors();
    MonitorCapabilities GetMonitorCapabilities(MonitorId id);
    void GetDetailedMonitorInfo(MonitorInfo& info);
    void SaveMonitorSettings(const MonitorInfo& info, const MonitorSettings& settings);
    MonitorSettings LoadMonitorSettings(const MonitorInfo& info);

private:
    static BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData);
    HANDLE GetPhysicalMonitorHandle(MonitorId id);
    // モニター情報取得用のヘルパー関数
    std::wstring GetSettingsFilePath(const MonitorInfo& info) const;
    std::wstring GetMappingConfigFilePath(MonitorId id) const;
    void SaveMappingConfig(MonitorId id, const MappingConfig& config);
    void LoadMappingConfigs();

    // EDID情報取得用のヘルパー関数
    bool GetMonitorEDID(const std::wstring& deviceName, std::vector<BYTE>& edidData);
    bool ParseEDIDManufacturerInfo(const std::vector<BYTE>& edidData, std::wstring& manufacturer, std::wstring& productCode);

    // 人間が識別可能な名前の生成
    std::wstring GenerateHumanReadableName(const MonitorInfo& info);
    std::wstring ConvertSizeToInches(const SIZE& sizeInMm);
    std::wstring GetMonitorRoleInfo(const MonitorInfo& info);

    // 設定ファイルのベースディレクトリ
    std::filesystem::path m_settingsPath;

    // モニターごとのマッピング設定
    std::map<MonitorId, MappingConfig> m_mappingConfigs;

    // 重複名の処理用のカウンター
    std::map<std::wstring, int> m_nameCounters;
};

#endif // DISPLAYCONTROLLER_MONITOR_CONTROLLER_H
