#include "MonitorController.h"
#include <iostream>
#include <iomanip>
#include <windows.h>

void PrintMonitorInfo(const MonitorController::MonitorInfo& info, MonitorController& controller)
{
    // Convert wide string to UTF-8 for console output
    int size = WideCharToMultiByte(CP_UTF8, 0, info.deviceName.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string deviceName(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, info.deviceName.c_str(), -1, &deviceName[0], size, nullptr, nullptr);

    std::cout << "Device: " << deviceName << std::endl;
    std::cout << "Primary: " << (info.isPrimary ? "Yes" : "No") << std::endl;
    std::cout << "Position: "
              << "Left=" << info.bounds.left << ", "
              << "Top=" << info.bounds.top << ", "
              << "Right=" << info.bounds.right << ", "
              << "Bottom=" << info.bounds.bottom << std::endl;
    std::cout << "Resolution: "
              << (info.bounds.right - info.bounds.left) << "x"
              << (info.bounds.bottom - info.bounds.top) << std::endl;

    // Convert wide strings to UTF-8 for console output
    auto toUtf8 = [](const std::wstring& ws) {
        int size = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, nullptr, 0, nullptr, nullptr);
        std::string s(size, 0);
        WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, &s[0], size, nullptr, nullptr);
        return s;
    };

    // Get detailed monitor info
    try {
        controller.GetDetailedMonitorInfo(const_cast<MonitorController::MonitorInfo&>(info));
        std::cout << "Manufacturer: " << toUtf8(info.manufacturerName) << std::endl;
        std::cout << "Product Code: " << toUtf8(info.productCode) << std::endl;
        std::cout << "Serial Number: " << toUtf8(info.serialNumber) << std::endl;
        std::cout << "Friendly Name: " << toUtf8(info.friendlyName) << std::endl;

        // Load and display saved settings
        auto settings = controller.LoadMonitorSettings(info);
        std::cout << "Saved Settings:" << std::endl;
        std::cout << "  Brightness: " << settings.brightness << "%" << std::endl;
        std::cout << "  Contrast: " << settings.contrast << "%" << std::endl;
        std::cout << "  Color Temperature: " << settings.colorTemperature << "K" << std::endl;
    }
    catch (const WindowsApiException& e) {
        std::cout << "Detailed Info: Unable to retrieve (Error: " << e.what() << ")" << std::endl;
    }

    // Get and display brightness
    try {
        int brightness = controller.GetBrightness(info.hMonitor);
        std::cout << "Brightness: " << brightness << "%" << std::endl;
    }
    catch (const WindowsApiException& e) {
        std::cout << "Brightness: Unable to retrieve (Error: " << e.what() << ")" << std::endl;
    }

    // Get and display monitor capabilities
    try {
        auto caps = controller.GetMonitorCapabilities(info.hMonitor);
        std::cout << "Display Technology: " << caps.technologyType << std::endl;
        std::cout << "Capabilities:" << std::endl;
        std::cout << "  - Brightness Control: " << (caps.supportsBrightness ? "Yes" : "No") << std::endl;
        std::cout << "  - Contrast Control: " << (caps.supportsContrast ? "Yes" : "No") << std::endl;
        std::cout << "  - Color Temperature: " << (caps.supportsColorTemperature ? "Yes" : "No") << std::endl;
        if (caps.displaySize.cx > 0 && caps.displaySize.cy > 0) {
            std::cout << "Display Size: " << caps.displaySize.cx << "mm x " << caps.displaySize.cy << "mm" << std::endl;
        }
        if (caps.supportsColorTemperature && caps.colorTemperature > 0) {
            std::cout << "Color Temperature: " << caps.colorTemperature << "K" << std::endl;
        }
    }
    catch (const WindowsApiException& e) {
        std::cout << "Monitor Capabilities: Unable to retrieve" << std::endl;
    }

    std::cout << "-------------------" << std::endl;
}

void PrintUsage()
{
    std::cout << "Usage: DisplayController.exe <command> [options]" << std::endl;
    std::cout << "\nCommands:" << std::endl;
    std::cout << "  list                        : モニター一覧の表示" << std::endl;
    std::cout << "  get <monitor_id>            : 指定モニターの輝度取得" << std::endl;
    std::cout << "  set <monitor_id> <value>    : 指定モニターの輝度設定" << std::endl;
    std::cout << "  sync enable                 : 自動同期を有効化" << std::endl;
    std::cout << "  sync disable                : 自動同期を無効化" << std::endl;
    std::cout << "  sync status                 : 同期状態の表示" << std::endl;
    std::cout << "  system-brightness get       : システムの輝度設定を取得" << std::endl;
    std::cout << "  system-brightness set <value>: システムの輝度設定を変更" << std::endl;
    std::cout << "  help                        : このヘルプを表示" << std::endl;
}

int main(int argc, char* argv[])
{
    try
    {
        // Set console output to UTF-8
        SetConsoleOutputCP(CP_UTF8);

        MonitorController controller;

        if (argc < 2)
        {
            PrintUsage();
            return 1;
        }

        std::string command = argv[1];

        if (command == "help")
        {
            PrintUsage();
            return 0;
        }
        else if (command == "list")
        {
            auto monitors = controller.GetMonitors();
            std::cout << "Found " << monitors.size() << " monitor(s):" << std::endl;
            std::cout << "===================" << std::endl;
            for (const auto& monitor : monitors)
            {
                PrintMonitorInfo(monitor, controller);
            }
        }
        else if (command == "sync")
        {
            if (argc < 3)
            {
                std::cerr << "Error: sync command requires an action (enable/disable/status)" << std::endl;
                return 1;
            }

            std::string action = argv[2];
            if (action == "enable")
            {
                controller.EnableBrightnessSync(true);
                std::cout << "Brightness sync enabled" << std::endl;
            }
            else if (action == "disable")
            {
                controller.EnableBrightnessSync(false);
                std::cout << "Brightness sync disabled" << std::endl;
            }
            else if (action == "status")
            {
                bool enabled = controller.IsBrightnessSyncEnabled();
                std::cout << "Brightness sync is " << (enabled ? "enabled" : "disabled") << std::endl;
            }
            else
            {
                std::cerr << "Error: Invalid sync action. Use enable, disable, or status" << std::endl;
                return 1;
            }
        }
        else if (command == "system-brightness")
        {
            if (argc < 3)
            {
                std::cerr << "Error: system-brightness command requires an action (get/set)" << std::endl;
                return 1;
            }

            std::string action = argv[2];
            if (action == "get")
            {
                auto monitors = controller.GetMonitors();
                if (!monitors.empty())
                {
                    int brightness = controller.GetBrightness(monitors[0].hMonitor);
                    std::cout << "System brightness: " << brightness << "%" << std::endl;
                }
            }
            else if (action == "set")
            {
                if (argc < 4)
                {
                    std::cerr << "Error: system-brightness set requires a value (0-100)" << std::endl;
                    return 1;
                }

                int brightness = std::stoi(argv[3]);
                auto monitors = controller.GetMonitors();
                if (!monitors.empty())
                {
                    if (controller.SetBrightness(monitors[0].hMonitor, brightness))
                    {
                        std::cout << "System brightness set to " << brightness << "%" << std::endl;
                    }
                    else
                    {
                        std::cerr << "Error: Failed to set system brightness" << std::endl;
                        return 1;
                    }
                }
            }
            else
            {
                std::cerr << "Error: Invalid system-brightness action. Use get or set" << std::endl;
                return 1;
            }
        }
        else
        {
            std::cerr << "Error: Unknown command '" << command << "'" << std::endl;
            PrintUsage();
            return 1;
        }

        return 0;
    }
    catch (const DisplayControllerException& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Unexpected error: " << e.what() << std::endl;
        return 1;
    }
}
