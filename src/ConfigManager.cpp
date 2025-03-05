#include "ConfigManager.h"
#include <common/StringUtils.h>
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
    m_config = nlohmann::json{
        {"monitors", nlohmann::json::array()},
        {"plugins", {{"DummyLightSensor", {{"devices", nlohmann::json::array({{"id", ""}, {"name", "Dummy Sensor 1"}, {"type", "Light Sensor"}})}}}, {"SwitchBotLightSensor", {{"global_settings", {{"token", ""}, {"secret", ""}}}, {"devices", nlohmann::json::array({{"id", ""}, {"name", "SwitchBot Sensor 1"}, {"type", "Light Sensor"}})}}}}},
        {"brightness_daemon", {{"update_interval_ms", 5000}, {"min_brightness", 0}, {"max_brightness", 100}}}};
    Save();
}

std::vector<std::string> ConfigManager::GetMonitorNames() const
{
    if (!m_isLoaded)
    {
        throw ConfigException("設定が読み込まれていません");
    }

    std::vector<std::string> names;
    if (m_config.contains("monitors"))
    {
        const auto &monitors = m_config["monitors"];
        for (const auto &monitor : monitors)
        {
            if (monitor.contains("name"))
            {
                names.push_back(monitor["name"].get<std::string>());
            }
        }
    }
    return names;
}

std::string ConfigManager::GetConfigFilePath() const
{
    return GetConfigPath();
}

bool ConfigManager::HasMonitor(const std::string &name) const
{
    if (!m_isLoaded)
    {
        return false;
    }

    if (m_config.contains("monitors"))
    {
        const auto &monitors = m_config["monitors"];
        for (const auto &monitor : monitors)
        {
            if (monitor.contains("name") && monitor["name"] == name)
            {
                return true;
            }
        }
    }
    return false;
}

MonitorBrightnessRange ConfigManager::GetMonitorBrightnessRange(const std::string &name) const
{
    if (!m_isLoaded)
    {
        throw ConfigException("設定が読み込まれていません");
    }

    if (!m_config.contains("monitors"))
    {
        throw ConfigException("monitorsセクションが見つかりません");
    }

    const auto &monitors = m_config["monitors"];
    for (const auto &monitor : monitors)
    {
        if (monitor["name"] == name)
        {
            if (!monitor.contains("brightness_range"))
            {
                throw ConfigException("モニター " + name + " の brightness_range が設定されていません");
            }

            const auto &range = monitor["brightness_range"];
            MonitorBrightnessRange result;

            try
            {
                result.min = range["min"].get<int>();
                result.max = range["max"].get<int>();

                if (!result.IsValid())
                {
                    throw ConfigException("不正な輝度範囲です: min=" + std::to_string(result.min) + ", max=" + std::to_string(result.max));
                }

                return result;
            }
            catch (const nlohmann::json::exception &e)
            {
                throw ConfigException("輝度範囲の値が不正です: " + std::string(e.what()));
            }
        }
    }

    throw ConfigException("モニターが見つかりません: " + name);
}

void ConfigManager::SetMonitorBrightnessRange(const std::string &name, const MonitorBrightnessRange &range)
{
    if (!m_isLoaded)
    {
        throw ConfigException("設定が読み込まれていません");
    }

    if (!range.IsValid())
    {
        throw ConfigException("不正な輝度範囲です: min=" + std::to_string(range.min) + ", max=" + std::to_string(range.max));
    }

    if (!m_config.contains("monitors"))
    {
        m_config["monitors"] = nlohmann::json::array();
    }

    auto &monitors = m_config["monitors"];
    for (auto &monitor : monitors)
    {
        if (monitor["name"] == name)
        {
            monitor["brightness_range"] = {
                {"min", range.min},
                {"max", range.max}
            };
            Save();
            return;
        }
    }

    throw ConfigException("モニターが見つかりません: " + name);
}

void ConfigManager::AddMonitor(const std::string &name, const MonitorBrightnessRange &range)
{
    if (!m_isLoaded)
    {
        throw ConfigException("設定が読み込まれていません");
    }

    if (name.empty())
    {
        throw ConfigException("モニター名を指定してください");
    }

    if (!range.IsValid())
    {
        throw ConfigException("不正な輝度範囲です: min=" + std::to_string(range.min) + ", max=" + std::to_string(range.max));
    }

    if (HasMonitor(name))
    {
        throw ConfigException("同じ名前のモニターが既に存在します: " + name);
    }

    if (!m_config.contains("monitors"))
    {
        m_config["monitors"] = nlohmann::json::array();
    }

    nlohmann::json monitor = {
        {"name", name},
        {"brightness_range", {
            {"min", range.min},
            {"max", range.max}
        }}
    };

    m_config["monitors"].push_back(monitor);
    Save();
}

void ConfigManager::RemoveMonitor(const std::string &name)
{
    if (!m_isLoaded)
    {
        throw ConfigException("設定が読み込まれていません");
    }

    if (!m_config.contains("monitors"))
    {
        throw ConfigException("monitorsセクションが見つかりません");
    }

    auto &monitors = m_config["monitors"];
    for (auto it = monitors.begin(); it != monitors.end(); ++it)
    {
        if ((*it)["name"] == name)
        {
            monitors.erase(it);
            Save();
            return;
        }
    }

    throw ConfigException("モニターが見つかりません: " + name);
}

CalibrationSettings ConfigManager::GetDeviceCalibration(const std::string &deviceId) const
{
    if (!m_isLoaded)
    {
        throw ConfigException("設定が読み込まれていません");
    }

    const auto &plugins = m_config["plugins"];
    if (!plugins.contains("SwitchBotLightSensor"))
    {
        throw ConfigException(ConfigValidationResult{
            false,
            "plugins.SwitchBotLightSensor",
            "object",
            "undefined",
            "",
            "SwitchBotLightSensorプラグインが見つかりません"});
    }

    const auto &pluginConfig = plugins["SwitchBotLightSensor"];
    if (!pluginConfig.contains("devices"))
    {
        throw ConfigException(ConfigValidationResult{
            false,
            "plugins.SwitchBotLightSensor.devices",
            "array",
            "undefined",
            "",
            "devicesセクションが見つかりません"});
    }

    const auto &devices = pluginConfig["devices"];
    for (size_t i = 0; i < devices.size(); ++i)
    {
        const auto &device = devices[i];
        if (device["id"] == deviceId)
        {
            std::string devicePath = "plugins.SwitchBotLightSensor.devices[" + std::to_string(i) + "]";
            CalibrationSettings settings;

            if (device.contains("calibration"))
            {
                const auto &calibration = device["calibration"];
                std::string calibrationPath = devicePath + ".calibration";

                try
                {
                    if (calibration.contains("min_raw_value"))
                    {
                        if (calibration["min_raw_value"].is_null())
                        {
                            throw ConfigException(ConfigValidationResult{
                                false,
                                calibrationPath + ".min_raw_value",
                                "number",
                                "null",
                                "null",
                                "min_raw_valueがnullです"});
                        }
                        auto result = ValidateNumber(calibration["min_raw_value"], calibrationPath + ".min_raw_value", 0, INT_MAX);
                        if (!result.isValid)
                            throw ConfigException(result);
                        settings.minRawValue = calibration["min_raw_value"].get<int>();
                    }

                    if (calibration.contains("max_raw_value"))
                    {
                        if (calibration["max_raw_value"].is_null())
                        {
                            throw ConfigException(ConfigValidationResult{
                                false,
                                calibrationPath + ".max_raw_value",
                                "number",
                                "null",
                                "null",
                                "max_raw_valueがnullです"});
                        }
                        auto result = ValidateNumber(calibration["max_raw_value"], calibrationPath + ".max_raw_value", 0, INT_MAX);
                        if (!result.isValid)
                            throw ConfigException(result);
                        settings.maxRawValue = calibration["max_raw_value"].get<int>();
                    }
                }
                catch (const nlohmann::json::type_error &e)
                {
                    throw ConfigException(ConfigValidationResult{
                        false,
                        calibrationPath,
                        "number",
                        "invalid",
                        "",
                        "キャリブレーション設定の値が不正です: " + std::string(e.what())});
                }
            }

            if (!settings.IsValid())
            {
                throw ConfigException(ConfigValidationResult{
                    false,
                    devicePath + ".calibration",
                    "",
                    "",
                    "min_raw_value: " + std::to_string(settings.minRawValue) +
                        ", max_raw_value: " + std::to_string(settings.maxRawValue),
                    "不正なキャリブレーション設定です: " + deviceId});
            }
            return settings;
        }
    }

    throw ConfigException(ConfigValidationResult{
        false,
        "plugins.SwitchBotLightSensor.devices",
        "",
        "",
        deviceId,
        "デバイスが見つかりません: " + deviceId});
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

    auto &plugins = m_config["plugins"];
    for (auto &[pluginName, pluginConfig] : plugins.items())
    {
        if (pluginName == "SwitchBotLightSensor")
        {
            if (pluginConfig.contains("devices"))
            {
                auto &devices = pluginConfig["devices"];
                for (auto &device : devices)
                {
                    if (device["name"] == deviceName)
                    {
                        // calibrationオブジェクトが存在しない場合は新規作成
                        if (!device.contains("calibration"))
                        {
                            device["calibration"] = nlohmann::json::object();
                        }

                        auto &calibration = device["calibration"];
                        calibration["min_raw_value"] = settings.minRawValue;
                        calibration["max_raw_value"] = settings.maxRawValue;
                        Save();
                        return;
                    }
                }
            }
        }
    }
    throw ConfigException("デバイスが見つかりません: " + deviceName);
}

// バリデーションヘルパーメソッドの実装
ConfigValidationResult ConfigManager::ValidateValue(const nlohmann::json &value,
                                                    const std::string &path,
                                                    const std::string &expectedType) const
{
    ConfigValidationResult result;
    result.path = path;
    result.expectedType = expectedType;

    if (expectedType == "string" && !value.is_string())
    {
        result.isValid = false;
        result.actualType = value.type_name();
        result.value = value.dump();
        result.message = "文字列である必要があります";
    }
    else if (expectedType == "number" && !value.is_number())
    {
        result.isValid = false;
        result.actualType = value.type_name();
        result.value = value.dump();
        result.message = "数値である必要があります";
    }
    else if (expectedType == "object" && !value.is_object())
    {
        result.isValid = false;
        result.actualType = value.type_name();
        result.value = value.dump();
        result.message = "オブジェクトである必要があります";
    }
    else if (expectedType == "array" && !value.is_array())
    {
        result.isValid = false;
        result.actualType = value.type_name();
        result.value = value.dump();
        result.message = "配列である必要があります";
    }

    return result;
}

ConfigValidationResult ConfigManager::ValidateNumber(const nlohmann::json &value,
                                                     const std::string &path,
                                                     int min,
                                                     int max) const
{
    auto result = ValidateValue(value, path, "number");
    if (!result.isValid)
        return result;

    int numValue = value.get<int>();
    if (numValue < min || numValue > max)
    {
        result.isValid = false;
        result.value = std::to_string(numValue);
        result.message = std::to_string(min) + "から" + std::to_string(max) + "の範囲である必要があります";
    }

    return result;
}

ConfigValidationResult ConfigManager::ValidateString(const nlohmann::json &value,
                                                     const std::string &path,
                                                     bool allowEmpty) const
{
    auto result = ValidateValue(value, path, "string");
    if (!result.isValid)
        return result;

    std::string strValue = value.get<std::string>();
    if (!allowEmpty && strValue.empty())
    {
        result.isValid = false;
        result.value = strValue;
        result.message = "空の文字列は許可されていません";
    }

    return result;
}

ConfigValidationResult ConfigManager::ValidateObject(const nlohmann::json &value,
                                                     const std::string &path) const
{
    return ValidateValue(value, path, "object");
}

ConfigValidationResult ConfigManager::ValidateArray(const nlohmann::json &value,
                                                    const std::string &path) const
{
    return ValidateValue(value, path, "array");
}

void ConfigManager::ValidateConfig() const
{
    // monitorsセクションの検証（オプショナル）
    if (m_config.contains("monitors"))
    {
        auto result = ValidateArray(m_config["monitors"], "monitors");
        if (!result.isValid)
            throw ConfigException(result);

        const auto &monitors = m_config["monitors"];
        for (size_t i = 0; i < monitors.size(); ++i)
        {
            const auto &monitor = monitors[i];
            std::string monitorPath = "monitors[" + std::to_string(i) + "]";

            // nameフィールドの検証
            if (!monitor.contains("name"))
            {
                throw ConfigException(ConfigValidationResult{
                    false, monitorPath + ".name", "string", "undefined", "",
                    "モニター名が指定されていません"});
            }
            auto nameResult = ValidateString(monitor["name"], monitorPath + ".name");
            if (!nameResult.isValid)
                throw ConfigException(nameResult);

            // brightness_rangeの検証
            if (!monitor.contains("brightness_range"))
            {
                throw ConfigException(ConfigValidationResult{
                    false, monitorPath + ".brightness_range", "object", "undefined", "",
                    "brightness_rangeが指定されていません"});
            }
            auto rangeResult = ValidateObject(monitor["brightness_range"], monitorPath + ".brightness_range");
            if (!rangeResult.isValid)
                throw ConfigException(rangeResult);

            const auto &range = monitor["brightness_range"];
            std::string rangePath = monitorPath + ".brightness_range";

            // minの検証
            if (!range.contains("min"))
            {
                throw ConfigException(ConfigValidationResult{
                    false, rangePath + ".min", "number", "undefined", "",
                    "最小輝度が指定されていません"});
            }
            auto minResult = ValidateNumber(range["min"], rangePath + ".min", 0, 100);
            if (!minResult.isValid)
                throw ConfigException(minResult);

            // maxの検証
            if (!range.contains("max"))
            {
                throw ConfigException(ConfigValidationResult{
                    false, rangePath + ".max", "number", "undefined", "",
                    "最大輝度が指定されていません"});
            }
            auto maxResult = ValidateNumber(range["max"], rangePath + ".max", 0, 100);
            if (!maxResult.isValid)
                throw ConfigException(maxResult);

            // min <= maxの検証
            int minBrightness = range["min"].get<int>();
            int maxBrightness = range["max"].get<int>();
            if (minBrightness > maxBrightness)
            {
                throw ConfigException(ConfigValidationResult{
                    false, rangePath, "", "",
                    "min: " + std::to_string(minBrightness) + ", max: " + std::to_string(maxBrightness),
                    "最小輝度は最大輝度以下である必要があります"});
            }
        }
    }

    // プラグインセクションの検証
    if (!m_config.contains("plugins"))
    {
        throw ConfigException(ConfigValidationResult{
            false, "plugins", "object", "undefined", "",
            "設定ファイルにpluginsセクションがありません"});
    }

    const auto &plugins = m_config["plugins"];
    for (auto &[pluginName, pluginConfig] : plugins.items())
    {
        std::string pluginPath = "plugins." + pluginName;

        // プラグイン設定の基本構造を検証
        if (!pluginConfig.contains("devices") && !pluginConfig.contains("global_settings"))
        {
            throw ConfigException(ConfigValidationResult{
                false, pluginPath, "", "", "",
                "devices または global_settings セクションがありません"});
        }

        // global_settingsの検証
        if (pluginConfig.contains("global_settings"))
        {
            auto result = ValidateObject(pluginConfig["global_settings"], pluginPath + ".global_settings");
            if (!result.isValid)
                throw ConfigException(result);
        }

        // devicesの検証
        if (pluginConfig.contains("devices"))
        {
            auto result = ValidateArray(pluginConfig["devices"], pluginPath + ".devices");
            if (!result.isValid)
                throw ConfigException(result);

            const auto &devices = pluginConfig["devices"];
            for (size_t i = 0; i < devices.size(); ++i)
            {
                const auto &device = devices[i];
                std::string devicePath = pluginPath + ".devices[" + std::to_string(i) + "]";

                // 必須フィールドの存在チェック
                for (const auto &field : {"id", "name", "type"})
                {
                    if (!device.contains(field))
                    {
                        throw ConfigException(ConfigValidationResult{
                            false, devicePath + "." + field, "string", "undefined", "",
                            std::string(field) + "が指定されていません"});
                    }
                }

                // 各フィールドの型と値の検証
                auto idResult = ValidateString(device["id"], devicePath + ".id");
                if (!idResult.isValid)
                    throw ConfigException(idResult);

                auto nameResult = ValidateString(device["name"], devicePath + ".name");
                if (!nameResult.isValid)
                    throw ConfigException(nameResult);

                auto typeResult = ValidateString(device["type"], devicePath + ".type");
                if (!typeResult.isValid)
                    throw ConfigException(typeResult);

                if (device.contains("description"))
                {
                    auto descResult = ValidateString(device["description"], devicePath + ".description", true);
                    if (!descResult.isValid)
                        throw ConfigException(descResult);
                }
            }
        }
    }

    // brightness_daemonセクションの検証
    if (!m_config.contains("brightness_daemon"))
    {
        throw ConfigException(ConfigValidationResult{
            false, "brightness_daemon", "object", "undefined", "",
            "設定ファイルにbrightness_daemonセクションがありません"});
    }

    const auto &brightness = m_config["brightness_daemon"];
    std::string brightnessPath = "brightness_daemon";

    // update_interval_msの検証
    if (!brightness.contains("update_interval_ms"))
    {
        throw ConfigException(ConfigValidationResult{
            false, brightnessPath + ".update_interval_ms", "number", "undefined", "",
            "update_interval_msが設定されていません"});
    }
    auto intervalResult = ValidateNumber(brightness["update_interval_ms"], brightnessPath + ".update_interval_ms", 1000, INT_MAX);
    if (!intervalResult.isValid)
        throw ConfigException(intervalResult);

    // min_brightnessの検証
    if (!brightness.contains("min_brightness"))
    {
        throw ConfigException(ConfigValidationResult{
            false, brightnessPath + ".min_brightness", "number", "undefined", "",
            "min_brightnessが設定されていません"});
    }
    auto minResult = ValidateNumber(brightness["min_brightness"], brightnessPath + ".min_brightness", 0, 100);
    if (!minResult.isValid)
        throw ConfigException(minResult);

    // max_brightnessの検証
    if (!brightness.contains("max_brightness"))
    {
        throw ConfigException(ConfigValidationResult{
            false, brightnessPath + ".max_brightness", "number", "undefined", "",
            "max_brightnessが設定されていません"});
    }
    auto maxResult = ValidateNumber(brightness["max_brightness"], brightnessPath + ".max_brightness", 0, 100);
    if (!maxResult.isValid)
        throw ConfigException(maxResult);

    // min_brightness <= max_brightnessの検証
    int minBrightness = brightness["min_brightness"].get<int>();
    int maxBrightness = brightness["max_brightness"].get<int>();
    if (minBrightness > maxBrightness)
    {
        throw ConfigException(ConfigValidationResult{
            false, brightnessPath, "", "",
            "min_brightness: " + std::to_string(minBrightness) + ", max_brightness: " + std::to_string(maxBrightness),
            "min_brightnessはmax_brightness以下である必要があります"});
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

        nlohmann::json loadedConfig;
        file >> loadedConfig;

        // 後方互換性のための移行ロジック
        if (loadedConfig.contains("switchbot") && !loadedConfig.contains("plugins"))
        {
            // switchbot セクションを plugins セクションに移行
            nlohmann::json plugins = nlohmann::json::object();
            plugins["SwitchBotLightSensor"] = {
                {"global_settings", {{"token", loadedConfig["switchbot"]["token"]}, {"secret", loadedConfig["switchbot"]["secret"]}}},
                {"devices", loadedConfig["switchbot"]["devices"]}};
            loadedConfig["plugins"] = plugins;
            loadedConfig.erase("switchbot");
        }

        m_config = loadedConfig;
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

std::string ConfigManager::GetPluginConfig(const std::string &pluginName, const std::string &key, const std::string &deviceName) const
{
    if (!m_isLoaded)
    {
        throw ConfigException("設定が読み込まれていません");
    }

    const auto &plugins = m_config["plugins"];
    if (!plugins.contains(pluginName))
    {
        throw ConfigException(ConfigValidationResult{
            false,
            "plugins." + pluginName,
            "object",
            "undefined",
            "",
            "プラグインが見つかりません: " + pluginName});
    }

    const auto &pluginConfig = plugins[pluginName];
    std::string basePath = "plugins." + pluginName;

    try
    {
        if (deviceName.empty())
        {
            // グローバル設定の取得
            if (!pluginConfig.contains("global_settings"))
            {
                throw ConfigException(ConfigValidationResult{
                    false,
                    basePath + ".global_settings",
                    "object",
                    "undefined",
                    "",
                    "global_settingsセクションが見つかりません"});
            }

            const auto &globalSettings = pluginConfig["global_settings"];
            if (!globalSettings.contains(key))
            {
                throw ConfigException(ConfigValidationResult{
                    false,
                    basePath + ".global_settings." + key,
                    "string",
                    "undefined",
                    "",
                    "グローバル設定が見つかりません: " + key});
            }

            const auto &value = globalSettings[key];
            if (value.is_null())
            {
                throw ConfigException(ConfigValidationResult{
                    false,
                    basePath + ".global_settings." + key,
                    "string",
                    "null",
                    "null",
                    "設定値がnullです: " + key});
            }

            auto result = ValidateString(value, basePath + ".global_settings." + key);
            if (!result.isValid)
            {
                throw ConfigException(result);
            }

            return value.get<std::string>();
        }
        else
        {
            // デバイス設定の取得
            if (!pluginConfig.contains("devices"))
            {
                throw ConfigException(ConfigValidationResult{
                    false,
                    basePath + ".devices",
                    "array",
                    "undefined",
                    "",
                    "devicesセクションが見つかりません"});
            }

            const auto &devices = pluginConfig["devices"];
            for (size_t i = 0; i < devices.size(); ++i)
            {
                const auto &device = devices[i];
                if (device["name"] == deviceName)
                {
                    std::string devicePath = basePath + ".devices[" + std::to_string(i) + "]";

                    if (!device.contains(key))
                    {
                        throw ConfigException(ConfigValidationResult{
                            false,
                            devicePath + "." + key,
                            "string",
                            "undefined",
                            "",
                            "デバイス設定が見つかりません: " + key});
                    }

                    const auto &value = device[key];
                    if (value.is_null())
                    {
                        throw ConfigException(ConfigValidationResult{
                            false,
                            devicePath + "." + key,
                            "string",
                            "null",
                            "null",
                            "設定値がnullです: " + key});
                    }

                    auto result = ValidateString(value, devicePath + "." + key);
                    if (!result.isValid)
                    {
                        throw ConfigException(result);
                    }

                    return value.get<std::string>();
                }
            }

            throw ConfigException(ConfigValidationResult{
                false,
                basePath + ".devices",
                "",
                "",
                deviceName,
                "デバイスが見つかりません: " + deviceName});
        }
    }
    catch (const nlohmann::json::type_error &e)
    {
        std::string path = deviceName.empty()
                               ? basePath + ".global_settings." + key
                               : basePath + ".devices[?]." + key;

        throw ConfigException(ConfigValidationResult{
            false,
            path,
            "string",
            "invalid",
            "",
            "設定値の型が不正です: " + std::string(e.what())});
    }
}

void ConfigManager::AddDevice(const std::string &pluginName, const std::string &id, const std::string &name, const std::string &type, const std::string &description)
{
    if (pluginName.empty())
    {
        throw ConfigException("プラグイン名を指定してください");
    }
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

    auto &plugins = m_config["plugins"];
    if (!plugins.contains(pluginName))
    {
        plugins[pluginName] = nlohmann::json::object();
    }
    auto &plugin = plugins[pluginName];

    if (!plugin.contains("devices"))
    {
        plugin["devices"] = nlohmann::json::array();
    }
    auto &devices = plugin["devices"];

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

std::vector<nlohmann::json> ConfigManager::GetDevicesByType(const std::string &type) const
{
    if (!m_isLoaded)
    {
        throw ConfigException("設定が読み込まれていません");
    }

    std::vector<nlohmann::json> result;
    const auto &plugins = m_config["plugins"];
    for (const auto &[pluginName, pluginConfig] : plugins.items())
    {
        if (pluginConfig.contains("devices"))
        {
            const auto &devices = pluginConfig["devices"];
            for (const auto &device : devices)
            {
                if (device["type"] == type)
                {
                    result.push_back(device);
                }
            }
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

    const auto &plugins = m_config["plugins"];
    for (const auto &[pluginName, pluginConfig] : plugins.items())
    {
        if (pluginConfig.contains("devices"))
        {
            const auto &devices = pluginConfig["devices"];
            for (const auto &device : devices)
            {
                if (device["type"] == type)
                {
                    return device;
                }
            }
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

    const auto &plugins = m_config["plugins"];
    for (const auto &[pluginName, pluginConfig] : plugins.items())
    {
        if (pluginConfig.contains("devices"))
        {
            const auto &devices = pluginConfig["devices"];
            for (const auto &device : devices)
            {
                if (device["name"] == name)
                {
                    return true;
                }
            }
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

    const auto &plugins = m_config["plugins"];
    for (const auto &[pluginName, pluginConfig] : plugins.items())
    {
        if (pluginConfig.contains("devices"))
        {
            const auto &devices = pluginConfig["devices"];
            for (const auto &device : devices)
            {
                if (device["type"] == type)
                {
                    return true;
                }
            }
        }
    }
    return false;
}

// 同期設定
bool ConfigManager::GetSyncOnStartup() const
{
 if (!m_isLoaded)
 {
  throw ConfigException("設定が読み込まれていません");
 }

 if (!m_config.contains("brightness_daemon"))
 {
  throw ConfigException("brightness_daemonセクションが見つかりません");
 }

 const auto &brightness = m_config["brightness_daemon"];
 if (!brightness.contains("sync_on_startup"))
 {
  return false; // デフォルト値はfalse
 }

 try
 {
  return brightness["sync_on_startup"].get<bool>();
 }
 catch (const nlohmann::json::exception &e)
 {
  StringUtils::OutputMessage("sync_on_startup の値が不正です。デフォルト値(false)を使用します: " + std::string(e.what()));
  return false; // パースエラー時はデフォルト値false
 }
}

// ここから不足していた実装を追加

int ConfigManager::GetUpdateInterval() const
{
    if (!m_isLoaded)
    {
        throw ConfigException("設定が読み込まれていません");
    }

    if (!m_config.contains("brightness_daemon"))
    {
        throw ConfigException("brightness_daemonセクションが見つかりません");
    }

    const auto &brightness = m_config["brightness_daemon"];
    if (!brightness.contains("update_interval_ms"))
    {
        throw ConfigException("update_interval_msが設定されていません");
    }

    try
    {
        return brightness["update_interval_ms"].get<int>();
    }
    catch (const nlohmann::json::exception &e)
    {
        throw ConfigException("update_interval_msの値が不正です: " + std::string(e.what()));
    }
}

void ConfigManager::SetUpdateInterval(int interval_ms)
{
    if (!m_isLoaded)
    {
        throw ConfigException("設定が読み込まれていません");
    }

    if (interval_ms < 1000)
    {
        throw ConfigException("update_interval_msは1000以上である必要があります");
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

    if (!m_config.contains("brightness_daemon"))
    {
        throw ConfigException("brightness_daemonセクションが見つかりません");
    }

    const auto &brightness = m_config["brightness_daemon"];
    if (!brightness.contains("min_brightness"))
    {
        throw ConfigException("min_brightnessが設定されていません");
    }

    try
    {
        int value = brightness["min_brightness"].get<int>();
        if (value < 0 || value > 100)
        {
            throw ConfigException("min_brightnessは0から100の範囲である必要があります");
        }
        return value;
    }
    catch (const nlohmann::json::exception &e)
    {
        throw ConfigException("min_brightnessの値が不正です: " + std::string(e.what()));
    }
}

void ConfigManager::SetMinBrightness(int value)
{
    if (!m_isLoaded)
    {
        throw ConfigException("設定が読み込まれていません");
    }

    if (value < 0 || value > 100)
    {
        throw ConfigException("min_brightnessは0から100の範囲である必要があります");
    }

    if (value > m_config["brightness_daemon"]["max_brightness"].get<int>())
    {
        throw ConfigException("min_brightnessはmax_brightness以下である必要があります");
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

    if (!m_config.contains("brightness_daemon"))
    {
        throw ConfigException("brightness_daemonセクションが見つかりません");
    }

    const auto &brightness = m_config["brightness_daemon"];
    if (!brightness.contains("max_brightness"))
    {
        throw ConfigException("max_brightnessが設定されていません");
    }

    try
    {
        int value = brightness["max_brightness"].get<int>();
        if (value < 0 || value > 100)
        {
            throw ConfigException("max_brightnessは0から100の範囲である必要があります");
        }
        return value;
    }
    catch (const nlohmann::json::exception &e)
    {
        throw ConfigException("max_brightnessの値が不正です: " + std::string(e.what()));
    }
}

void ConfigManager::SetMaxBrightness(int value)
{
    if (!m_isLoaded)
    {
        throw ConfigException("設定が読み込まれていません");
    }

    if (value < 0 || value > 100)
    {
        throw ConfigException("max_brightnessは0から100の範囲である必要があります");
    }

    if (value < m_config["brightness_daemon"]["min_brightness"].get<int>())
    {
        throw ConfigException("max_brightnessはmin_brightness以上である必要があります");
    }

    m_config["brightness_daemon"]["max_brightness"] = value;
    Save();
}

nlohmann::json ConfigManager::CreateDefaultMonitorConfig(const std::string& name) const
{
    return {
        {"name", name},
        {"brightness_range", {
            {"min", 0},
            {"max", 100}
        }}
    };
}

void ConfigManager::CreateBackup() const
{
    if (!m_isLoaded)
    {
        throw ConfigException("設定が読み込まれていません");
    }

    try
    {
        std::filesystem::path backupPath = GetBackupPath();
        std::filesystem::create_directories(backupPath.parent_path());

        std::ofstream backupFile(backupPath);
        if (!backupFile.is_open())
        {
            throw ConfigException("バックアップファイルを作成できませんでした: " + backupPath.string());
        }

        backupFile << m_config.dump(2);
        backupFile.close();
    }
    catch (const std::exception& e)
    {
        throw ConfigException("バックアップの作成に失敗しました: " + std::string(e.what()));
    }
}

std::string ConfigManager::GetBackupPath() const
{
    std::filesystem::path configPath = GetConfigPath();
    std::time_t now = std::time(nullptr);
    std::stringstream ss;
    ss << configPath.stem().string() << "_backup_"
       << std::put_time(std::localtime(&now), "%Y%m%d_%H%M%S")
       << configPath.extension().string();
    return (configPath.parent_path() / ss.str()).string();
}

void ConfigManager::RestoreFromBackup()
{
    std::filesystem::path configDir = std::filesystem::path(GetConfigPath()).parent_path();
    std::vector<std::filesystem::path> backupFiles;

    // バックアップファイルを検索
    for (const auto& entry : std::filesystem::directory_iterator(configDir))
    {
        if (entry.path().filename().string().find("_backup_") != std::string::npos)
        {
            backupFiles.push_back(entry.path());
        }
    }

    if (backupFiles.empty())
    {
        throw ConfigException("利用可能なバックアップファイルが見つかりません");
    }

    // 最新のバックアップファイルを使用
    std::sort(backupFiles.begin(), backupFiles.end(), std::greater<>());
    std::filesystem::path latestBackup = backupFiles[0];

    try
    {
        std::ifstream backupFile(latestBackup);
        if (!backupFile.is_open())
        {
            throw ConfigException("バックアップファイルを開けませんでした: " + latestBackup.string());
        }

        nlohmann::json backupConfig = nlohmann::json::parse(backupFile);
        m_config = backupConfig;
        ValidateConfig();
        Save();
    }
    catch (const nlohmann::json::exception& e)
    {
        throw ConfigException("バックアップファイルの解析に失敗しました: " + std::string(e.what()));
    }
    catch (const std::exception& e)
    {
        throw ConfigException("バックアップからの復元に失敗しました: " + std::string(e.what()));
    }
}
