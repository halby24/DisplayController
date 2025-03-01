#ifndef DISPLAYCONTROLLER_CONFIG_MANAGER_H
#define DISPLAYCONTROLLER_CONFIG_MANAGER_H

#include <string>
#include <memory>
#include <nlohmann/json.hpp>

class ConfigManager {
public:
    static ConfigManager& Instance();

    // 設定の読み込み
    void Load();

    // 設定の保存
    void Save();

    // 設定値の取得
    std::string GetSwitchBotToken() const;
    std::string GetSwitchBotSecret() const;
    std::string GetDeviceId(const std::string& name) const;

    // 設定値の設定
    void SetSwitchBotToken(const std::string& token);
    void SetSwitchBotSecret(const std::string& secret);
    void AddDevice(const std::string& id, const std::string& name, const std::string& type);

private:
    ConfigManager() = default;
    ~ConfigManager() = default;
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    std::string GetConfigPath() const;
    void ValidateConfig() const;
    void CreateDefaultConfig();
    void EnsureConfigDirectoryExists() const;

    nlohmann::json m_config;
    bool m_isLoaded = false;
    static constexpr const char* CONFIG_FILENAME = "config.json";
};

class ConfigException : public std::runtime_error {
public:
    explicit ConfigException(const std::string& message)
        : std::runtime_error(message) {}
};

#endif // DISPLAYCONTROLLER_CONFIG_MANAGER_H
