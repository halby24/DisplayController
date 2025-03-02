#include "ConfigManager.h"
#include "StringUtils.h"
#include <fstream>
#include <filesystem>
#include <shlobj.h>
#include <windows.h>

namespace
{
    std::string GetAppDataPath()
    {
        PWSTR path;
        if (FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &path)))
        {
            throw ConfigException(" LocalAppDataフォルダのパスの取得に失敗しました ");
        }
        std::wstring widePath(path);
        CoTaskMemFree(path);
        std::filesystem::path basePath = StringUtils::WideToUtf8(widePath);
        return (basePath / "DisplayController" / "Settings").string();
    }
}

ConfigManager &ConfigManager::Instance()
{
    static ConfigManager instance;
    return instance;
}

std::string ConfigManager::GetConfigPath() const
{
    std::filesystem::path configPath = GetAppDataPath();
    return (configPath / CONFIG_FILENAME).string();
}

void ConfigManager::EnsureConfigDirectoryExists() const
{
    std::filesystem::path dirPath = GetAppDataPath();
    if (!std::filesystem::exists(dirPath))
    {
        if (!std::filesystem::create_directories(dirPath))
        {
            throw ConfigException("設定ファイル用ディレクトリの作成に失敗しました");
        }
    }
}

void ConfigManager::CreateDefaultConfig()
{
    m_config = {
        {"switchbot", {{"token", "}, {"secret", "}, {"devices", nlohmann::json::array()}}},
        {"brightness_daemon", {{"update_interval_ms", 5000}, {"min_brightness", 0}, {"max_brightness", 100}}}};
    Save();
}

CalibrationSettings ConfigManager::GetDeviceCalibration(const std::string &deviceName) const
{
    if (!m_isLoaded)
    {
        throw ConfigException("設定が読み込まれていません");
    }

    const auto &devices = m_config["switchbot"]["devices"];
    for (const auto &device : devices)
    {
        if (device["name"] == deviceName)
        {
            CalibrationSettings settings;
            if (device.contains("calibration"))
            {
                const auto &calibration = device["calibration"];
                if (calibration.contains("min_raw_value"))
                {
                    settings.minRawValue = calibration["min_raw_value"].get<int>();
                }
                if (calibration.contains("max_raw_value"))
                {
                    settings.maxRawValue = calibration["max_raw_value"].get<int>();
                }
            }

            if (!settings.IsValid())
            {
                throw ConfigException("不正なキャリブレーション設定です: " + deviceName);
            }
            return settings;
        }
    }
    throw ConfigException("デバイスが見つかりません: " + deviceName);
}

void ConfigManager::SetDeviceCalibration(const std::string &deviceName, const CalibrationSettings &settings)
{
    if (!m_isLoaded)
    {
        throw ConfigException("設定が読み込まれていません");
    }

    if (!settings.IsValid())
    {
        throw ConfigException("不正なキャリブレーション設定です");
    }

    auto &devices = m_config["switchbot"]["devices"];
    for (auto &device : devices)
    {
        if (device["name"] == deviceName)
        {
            device["calibration"] = {
                {"min_raw_value", settings.minRawValue},
                {"max_raw_value", settings.maxRawValue}};
            Save();
            return;
        }
    }
    throw ConfigException("デバイスが見つかりません: " + deviceName);
}

void ConfigManager::ValidateConfig() const
{
    if (!m_config.contains("switchbot"))
    {
        throw ConfigException("設定ファイルにswitchbotセクションがありません");
    }

    const auto &switchbot = m_config["switchbot"];
    if (!switchbot.contains("token"))
    {
        throw ConfigException("SwitchBot APIトークンが設定されていません");
    }
    if (!switchbot.contains("secret"))
    {
        throw ConfigException("SwitchBot APIシークレットが設定されていません");
    }
    if (!switchbot.contains("devices"))
    {
        throw ConfigException("devicesセクションがありません");
    }
    if (!switchbot["devices"].is_array())
    {
        throw ConfigException("devicesは配列である必要があります");
    }

    const auto &devices = switchbot["devices"];
    for (const auto &device : devices)
    {
        if (!device.contains("id"))
        {
            throw ConfigException("デバイス設定にIDが指定されていません");
        }
        if (!device.contains("name"))
        {
            throw ConfigException("デバイス設定に名前が指定されていません");
        }
        if (!device.contains("type"))
        {
            throw ConfigException("デバイス設定にタイプが指定されていません");
        }

        if (!device["id"].is_string() || device["id"].get<std::string>().empty())
        {
            throw ConfigException("デバイスIDは空でない文字列である必要があります");
        }
        if (!device["name"].is_string() || device["name"].get<std::string>().empty())
        {
            throw ConfigException("デバイス名は空でない文字列である必要があります");
        }
        if (!device["type"].is_string() || device["type"].get<std::string>().empty())
        {
            throw ConfigException("デバイスタイプは空でない文字列である必要があります");
        }

        if (device.contains("description") && !device["description"].is_string())
        {
            throw ConfigException("デバイスの説明は文字列である必要があります");
        }
    }

    const auto &token = switchbot["token"].get<std::string>();
    const auto &secret = switchbot["secret"].get<std::string>();

    if (token.empty())
    {
        throw ConfigException("APIトークンが空です");
    }
    if (secret.empty())
    {
        throw ConfigException("APIシークレットが空です");
    }

    if (!m_config.contains("brightness_daemon"))
    {
        throw ConfigException("設定ファイルにbrightness_daemonセクションがありません");
    }

    const auto &brightness = m_config["brightness_daemon"];
    if (!brightness.contains("update_interval_ms"))
    {
        throw ConfigException("update_interval_msが設定されていません");
    }
    if (!brightness.contains("min_brightness"))
    {
        throw ConfigException("min_brightnessが設定されていません");
    }
    if (!brightness.contains("max_brightness"))
    {
        throw ConfigException("max_brightnessが設定されていません");
    }

    int interval = brightness["update_interval_ms"].get<int>();
    if (interval < 1000)
    {
        throw ConfigException("update_interval_msは1000以上である必要があります");
    }

    int minBrightness = brightness["min_brightness"].get<int>();
    if (minBrightness < 0 || minBrightness > 100)
    {
        throw ConfigException("min_brightnessは0から100の範囲である必要があります");
    }

    int maxBrightness = brightness["max_brightness"].get<int>();
    if (maxBrightness < 0 || maxBrightness > 100)
    {
        throw ConfigException("max_brightnessは0から100の範囲である必要があります");
    }

    if (minBrightness > maxBrightness)
    {
        throw ConfigException("min_brightnessはmax_brightness以下である必要があります");
    }
}

void ConfigManager::Load()
{
    try
    {
        std::string configPath = GetConfigPath();
        if (!std::filesystem::exists(configPath))
        {
            EnsureConfigDirectoryExists();
            CreateDefaultConfig();
            return;
        }

        std::ifstream file(configPath);
        if (!file.is_open())
        {
            throw ConfigException("設定ファイルを開けませんでした");
        }

        file >> m_config;
        ValidateConfig();
        m_isLoaded = true;
    }
    catch (const nlohmann::json::exception &e)
    {
        throw ConfigException("設定ファイルの解析に失敗しました : " + std::string(e.what()));
    }
}

void ConfigManager::Save()
{
    try
    {
        EnsureConfigDirectoryExists();
        std::ofstream file(GetConfigPath());
        if (!file.is_open())
        {
            throw ConfigException("設定ファイルを書き込み用に開けませんでした");
        }

        file << m_config.dump(2);
        m_isLoaded = true;
    }
    catch (const nlohmann::json::exception &e)
    {
        throw ConfigException("設定ファイルの書き込みに失敗しました : " + std::string(e.what()));
    }
}

std::string ConfigManager::GetSwitchBotToken() const
{
    if (!m_isLoaded)
    {
        throw ConfigException("設定が読み込まれていません");
    }
    return m_config["switchbot"]["token"];
}

std::string ConfigManager::GetSwitchBotSecret() const
{
    if (!m_isLoaded)
    {
        throw ConfigException("設定が読み込まれていません");
    }
    return m_config["switchbot"]["secret"];
}

std::string ConfigManager::GetDeviceId(const std::string &name) const
{
    if (!m_isLoaded)
    {
        throw ConfigException("設定が読み込まれていません");
    }

    const auto &devices = m_config["switchbot"]["devices"];
    for (const auto &device : devices)
    {
        if (device["name"] == name)
        {
            return device["id"];
        }
    }
    throw ConfigException("デバイスが見つかりません : " + name);
}

std::vector<nlohmann::json> ConfigManager::GetDevicesByType(const std::string &type) const
{
    if (!m_isLoaded)
    {
        throw ConfigException("設定が読み込まれていません");
    }

    std::vector<nlohmann::json> result;
    const auto &devices = m_config["switchbot"]["devices"];
    for (const auto &device : devices)
    {
        if (device["type"] == type)
        {
            result.push_back(device);
        }
    }
    return result;
}

nlohmann::json ConfigManager::GetFirstDeviceByType(const std::string &type) const
{
    if (!m_isLoaded)
    {
        throw ConfigException("設定が読み込まれていません");
    }

    const auto &devices = m_config["switchbot"]["devices"];
    for (const auto &device : devices)
    {
        if (device["type"] == type)
        {
            return device;
        }
    }
    throw ConfigException("指定されたタイプのデバイスが見つかりません : " + type);
}

bool ConfigManager::HasDevice(const std::string &name) const
{
    if (!m_isLoaded)
    {
        return false;
    }

    const auto &devices = m_config["switchbot"]["devices"];
    for (const auto &device : devices)
    {
        if (device["name"] == name)
        {
            return true;
        }
    }
    return false;
}

bool ConfigManager::HasDeviceType(const std::string &type) const
{
    if (!m_isLoaded)
    {
        return false;
    }

    const auto &devices = m_config["switchbot"]["devices"];
    for (const auto &device : devices)
    {
        if (device["type"] == type)
        {
            return true;
        }
    }
    return false;
}

void ConfigManager::SetSwitchBotToken(const std::string &token)
{
    if (token.empty())
    {
        throw ConfigException(" APIトークンを指定してください ");
    }
    m_config["switchbot"]["token"] = token;
    Save();
}

void ConfigManager::SetSwitchBotSecret(const std::string &secret)
{
    if (secret.empty())
    {
        throw ConfigException(" APIシークレットを指定してください ");
    }
    m_config["switchbot"]["secret"] = secret;
    Save();
}

void ConfigManager::AddDevice(const std::string &id, const std::string &name, const std::string &type, const std::string &description)
{
    if (id.empty())
    {
        throw ConfigException("デバイスIDを指定してください");
    }
    if (name.empty())
    {
        throw ConfigException("デバイス名を指定してください");
    }
    if (type.empty())
    {
        throw ConfigException("デバイスタイプを指定してください");
    }

    auto &devices = m_config["switchbot"]["devices"];
    for (auto &device : devices)
    {
        if (device["name"] == name)
        {
            throw ConfigException("同じ名前のデバイスが既に存在します : " + name);
        }
        if (device["id"] == id)
        {
            throw ConfigException("同じIDのデバイスが既に存在します : " + id);
        }
    }

    nlohmann::json device = {
        {"id", id},
        {"name", name},
        {"type", type}};

    if (!description.empty())
    {
        device["description"] = description;
    }

    devices.push_back(device);
    Save();
}

int ConfigManager::GetUpdateInterval() const
{
    if (!m_isLoaded)
    {
        throw ConfigException("設定が読み込まれていません");
    }
    return m_config["brightness_daemon"]["update_interval_ms"].get<int>();
}

void ConfigManager::SetUpdateInterval(int interval_ms)
{
    if (interval_ms < 1000)
    {
        throw ConfigException(" update_interval_msは1000以上である必要があります ");
    }
    m_config["brightness_daemon"]["update_interval_ms"] = interval_ms;
    Save();
}

int ConfigManager::GetMinBrightness() const
{
    if (!m_isLoaded)
    {
        throw ConfigException("設定が読み込まれていません");
    }
    return m_config["brightness_daemon"]["min_brightness"].get<int>();
}

void ConfigManager::SetMinBrightness(int value)
{
    if (value < 0 || value > 100)
    {
        throw ConfigException(" min_brightnessは0から100の範囲である必要があります ");
    }
    if (value > GetMaxBrightness())
    {
        throw ConfigException(" min_brightnessはmax_brightness以下である必要があります ");
    }
    m_config["brightness_daemon"]["min_brightness"] = value;
    Save();
}

int ConfigManager::GetMaxBrightness() const
{
    if (!m_isLoaded)
    {
        throw ConfigException("設定が読み込まれていません");
    }
    return m_config["brightness_daemon"]["max_brightness"].get<int>();
}

void ConfigManager::SetMaxBrightness(int value)
{
    if (value < 0 || value > 100)
    {
        throw ConfigException("max_brightnessは0から100の範囲である必要があります");
    }
    if (value < GetMinBrightness())
    {
        throw ConfigException("max_brightnessはmin_brightness以上である必要があります");
    }
    m_config["brightness_daemon"]["max_brightness"] = value;
    Save();
}
