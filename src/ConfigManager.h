#ifndef DISPLAYCONTROLLER_CONFIG_MANAGER_H
#define DISPLAYCONTROLLER_CONFIG_MANAGER_H

#ifdef DISPLAYCONTROLLERLIB_EXPORTS
#define DISPLAYCONTROLLERLIB_API __declspec(dllexport)
#else
#define DISPLAYCONTROLLERLIB_API __declspec(dllimport)
#endif

#include <string>
#include <memory>
#include <nlohmann/json.hpp>
#include <common/StringUtils.h>

// キャリブレーション設定を管理する構造体
struct DISPLAYCONTROLLERLIB_API CalibrationSettings
{
    int minRawValue = 0;    // デフォルト値
    int maxRawValue = 1000; // デフォルト値

    bool IsValid() const
    {
        return minRawValue >= 0 && maxRawValue > minRawValue;
    }
};

class DISPLAYCONTROLLERLIB_API ConfigManager
{
public:
    static ConfigManager &Instance();

    // 設定の読み込み
    void Load();

    // 設定の保存
    void Save();

    // プラグイン設定の取得
    std::string GetPluginConfig(const std::string &pluginName, const std::string &key, const std::string &deviceName = "") const;

    // デバイス管理
    std::vector<nlohmann::json> GetDevicesByType(const std::string &type) const;
    nlohmann::json GetFirstDeviceByType(const std::string &type) const;
    bool HasDevice(const std::string &name) const;
    bool HasDeviceType(const std::string &type) const;

    // デバイスの追加
    void AddDevice(const std::string &pluginName, const std::string &id, const std::string &name, const std::string &type, const std::string &description = "");

    // 輝度制御の設定
    int GetUpdateInterval() const;
    void SetUpdateInterval(int interval_ms);
    int GetMinBrightness() const;
    void SetMinBrightness(int value);
    int GetMaxBrightness() const;
    void SetMaxBrightness(int value);

    // キャリブレーション設定の取得と設定
    CalibrationSettings GetDeviceCalibration(const std::string &deviceName) const;
    void SetDeviceCalibration(const std::string &deviceName, const CalibrationSettings &settings);

private:
    ConfigManager() = default;
    ~ConfigManager() = default;
    ConfigManager(const ConfigManager &) = delete;
    ConfigManager &operator=(const ConfigManager &) = delete;

    std::string GetConfigPath() const;
    void ValidateConfig() const;
    void CreateDefaultConfig();
    void EnsureConfigDirectoryExists() const;

    nlohmann::json m_config;
    bool m_isLoaded = false;
    static constexpr const char *CONFIG_FILENAME = "config.json";
};

class ConfigException : public std::runtime_error
{
public:
    explicit ConfigException(const std::string &message)
        : std::runtime_error(message)
    {
        // エラーメッセージはすでにUTF-8なので変換は不要
    }
};

#endif // DISPLAYCONTROLLER_CONFIG_MANAGER_H
