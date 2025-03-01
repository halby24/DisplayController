#include "ConfigManager.h"
#include <fstream>
#include <filesystem>
#include <shlobj.h>
#include <windows.h>

namespace {
    // APPDATAフォルダのパスを取得
    std::string GetAppDataPath() {
        PWSTR path;
        if (FAILED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &path))) {
            throw ConfigException("Failed to get AppData path");
        }
        std::wstring widePath(path);
        CoTaskMemFree(path);
        return std::string(widePath.begin(), widePath.end()) + "\\DisplayController";
    }
}

ConfigManager& ConfigManager::Instance() {
    static ConfigManager instance;
    return instance;
}

std::string ConfigManager::GetConfigPath() const {
    return GetAppDataPath() + "\\" + CONFIG_FILENAME;
}

void ConfigManager::EnsureConfigDirectoryExists() const {
    std::filesystem::path dirPath = GetAppDataPath();
    if (!std::filesystem::exists(dirPath)) {
        if (!std::filesystem::create_directories(dirPath)) {
            throw ConfigException("Failed to create config directory");
        }
    }
}

void ConfigManager::CreateDefaultConfig() {
    m_config = {
        {"switchbot", {
            {"token", ""},
            {"devices", nlohmann::json::array()}
        }}
    };
    Save();
}

void ConfigManager::ValidateConfig() const {
    if (!m_config.contains("switchbot")) {
        throw ConfigException("Invalid config: 'switchbot' section missing");
    }

    const auto& switchbot = m_config["switchbot"];
    if (!switchbot.contains("token")) {
        throw ConfigException("Invalid config: 'token' field missing");
    }
    if (!switchbot.contains("devices")) {
        throw ConfigException("Invalid config: 'devices' field missing");
    }
    if (!switchbot["devices"].is_array()) {
        throw ConfigException("Invalid config: 'devices' must be an array");
    }
}

void ConfigManager::Load() {
    try {
        std::string configPath = GetConfigPath();
        if (!std::filesystem::exists(configPath)) {
            EnsureConfigDirectoryExists();
            CreateDefaultConfig();
            return;
        }

        std::ifstream file(configPath);
        if (!file.is_open()) {
            throw ConfigException("Failed to open config file");
        }

        file >> m_config;
        ValidateConfig();
        m_isLoaded = true;
    }
    catch (const nlohmann::json::exception& e) {
        throw ConfigException(std::string("Failed to parse config file: ") + e.what());
    }
}

void ConfigManager::Save() {
    try {
        EnsureConfigDirectoryExists();
        std::ofstream file(GetConfigPath());
        if (!file.is_open()) {
            throw ConfigException("Failed to open config file for writing");
        }

        file << m_config.dump(2);
        m_isLoaded = true;
    }
    catch (const nlohmann::json::exception& e) {
        throw ConfigException(std::string("Failed to write config file: ") + e.what());
    }
}

std::string ConfigManager::GetSwitchBotToken() const {
    if (!m_isLoaded) {
        throw ConfigException("Config not loaded");
    }
    return m_config["switchbot"]["token"];
}

std::string ConfigManager::GetDeviceId(const std::string& name) const {
    if (!m_isLoaded) {
        throw ConfigException("Config not loaded");
    }

    const auto& devices = m_config["switchbot"]["devices"];
    for (const auto& device : devices) {
        if (device["name"] == name && device["type"] == "lightSensor") {
            return device["id"];
        }
    }
    throw ConfigException("Device not found: " + name);
}

void ConfigManager::SetSwitchBotToken(const std::string& token) {
    if (token.empty()) {
        throw ConfigException("Token cannot be empty");
    }
    m_config["switchbot"]["token"] = token;
    Save();
}

void ConfigManager::AddDevice(const std::string& id, const std::string& name, const std::string& type) {
    if (id.empty() || name.empty() || type.empty()) {
        throw ConfigException("Device ID, name and type cannot be empty");
    }

    auto& devices = m_config["switchbot"]["devices"];
    for (auto& device : devices) {
        if (device["name"] == name) {
            throw ConfigException("Device with this name already exists");
        }
        if (device["id"] == id) {
            throw ConfigException("Device with this ID already exists");
        }
    }

    devices.push_back({
        {"id", id},
        {"name", name},
        {"type", type}
    });
    Save();
}
