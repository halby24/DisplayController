#include "PluginLoader.h"
#include <windows.h>
#include <filesystem>
#include <stdexcept>
#include <sstream>

namespace fs = std::filesystem;

PluginLoader::PluginLoader() = default;

PluginLoader::~PluginLoader() {
    // 全てのプラグインをアンロード
    for (auto& [name, info] : m_plugins) {
        UnloadPlugin(info);
    }
    m_plugins.clear();
}

size_t PluginLoader::LoadPlugins(const std::string& pluginDir) {
    size_t loadedCount = 0;

    try {
        for (const auto& entry : fs::directory_iterator(pluginDir)) {
            if (entry.path().extension() == ".dll") {
                try {
                    LoadPlugin(entry.path().string());
                    loadedCount++;
                }
                catch (const std::exception& e) {
                    // 個別のプラグインのロード失敗は記録するが、続行する
                    std::ostringstream oss;
                    oss << "Failed to load plugin " << entry.path().string()
                        << ": " << e.what() << "\n";
                    OutputDebugStringA(oss.str().c_str());
                }
            }
        }
    }
    catch (const fs::filesystem_error& e) {
        std::ostringstream oss;
        oss << "Failed to access plugin directory: " << e.what();
        throw std::runtime_error(oss.str());
    }

    return loadedCount;
}

void PluginLoader::LoadPlugin(const std::string& pluginPath) {
    // DLLをロード
    HMODULE handle = LoadLibraryA(pluginPath.c_str());
    if (!handle) {
        std::ostringstream oss;
        oss << "Failed to load plugin DLL: " << pluginPath;
        throw std::runtime_error(oss.str());
    }

    // 関数ポインタを取得
    auto createPlugin = reinterpret_cast<ILightSensorPlugin*(*)()>(
        GetProcAddress(handle, "CreatePlugin")
    );

    if (!createPlugin) {
        FreeLibrary(handle);
        std::ostringstream oss;
        oss << "Invalid plugin DLL (CreatePlugin not found): " << pluginPath;
        throw std::runtime_error(oss.str());
    }

    // プラグインインスタンスを作成
    ILightSensorPlugin* plugin = createPlugin();
    if (!plugin) {
        FreeLibrary(handle);
        std::ostringstream oss;
        oss << "Failed to create plugin instance: " << pluginPath;
        throw std::runtime_error(oss.str());
    }

    // プラグイン情報を保存
    PluginInfo info{
        .handle = handle,
        .plugin = plugin,
        .path = pluginPath
    };

    const char* pluginName = plugin->GetPluginName();
    auto it = m_plugins.find(pluginName);
    if (it != m_plugins.end()) {
        // 同名のプラグインが既に存在する場合は古いものをアンロード
        UnloadPlugin(it->second);
    }
    m_plugins[pluginName] = std::move(info);
}

void PluginLoader::UnloadPlugin(PluginInfo& info) {
    if (info.plugin) {
        // プラグインの破棄関数を取得
        auto destroyPlugin = reinterpret_cast<void(*)(ILightSensorPlugin*)>(
            GetProcAddress((HMODULE)info.handle, "DestroyPlugin")
        );

        if (destroyPlugin) {
            destroyPlugin(info.plugin);
        }
        info.plugin = nullptr;
    }

    if (info.handle) {
        FreeLibrary((HMODULE)info.handle);
        info.handle = nullptr;
    }
}

std::unique_ptr<ILightSensor> PluginLoader::CreateSensor(
    const std::string& pluginName,
    const json& config
) {
    auto it = m_plugins.find(pluginName);
    if (it == m_plugins.end()) {
        std::ostringstream oss;
        oss << "Plugin not found: " << pluginName;
        throw std::runtime_error(oss.str());
    }

    try {
        return it->second.plugin->CreateSensor(config);
    }
    catch (const std::exception& e) {
        std::ostringstream oss;
        oss << "Failed to create sensor from plugin " << pluginName
            << ": " << e.what();
        throw std::runtime_error(oss.str());
    }
}

std::vector<std::string> PluginLoader::GetLoadedPluginNames() const {
    std::vector<std::string> names;
    names.reserve(m_plugins.size());

    for (const auto& [name, _] : m_plugins) {
        names.push_back(name);
    }

    return names;
}
