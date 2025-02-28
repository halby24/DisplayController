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

        // Display mapping configuration
        auto mapping = controller.GetMappingConfig(info.id);
        std::cout << "Brightness Mapping:" << std::endl;
        std::cout << "  Min: " << mapping.minBrightness << "%" << std::endl;
        std::cout << "  Max: " << mapping.maxBrightness << "%" << std::endl;
        if (!mapping.mappingPoints.empty()) {
            std::cout << "  Custom Points:" << std::endl;
            for (const auto& point : mapping.mappingPoints) {
                std::cout << "    " << point.first << "% -> " << point.second << "%" << std::endl;
            }
        }
    }
    catch (const WindowsApiException& e) {
        std::cout << "Detailed Info: Unable to retrieve (Error: " << e.what() << ")" << std::endl;
    }

    // Get and display brightness
    try {
        int brightness = controller.GetBrightness(info.id);
        std::cout << "Current Brightness: " << brightness << "%" << std::endl;
    }
    catch (const WindowsApiException& e) {
        std::cout << "Brightness: Unable to retrieve (Error: " << e.what() << ")" << std::endl;
    }

    // Get and display monitor capabilities
    try {
        auto caps = controller.GetMonitorCapabilities(info.id);
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
    std::cout << "  setall <value>              : 全モニターの輝度を統一的に設定" << std::endl;
    std::cout << "  map <monitor_id> <options>  : モニターの輝度マッピング設定" << std::endl;
    std::cout << "    options:" << std::endl;
    std::cout << "      --min <value>           : 最小輝度値 (0-100)" << std::endl;
    std::cout << "      --max <value>           : 最大輝度値 (0-100)" << std::endl;
    std::cout << "      --point <in,out>        : マッピングポイントを追加 (複数指定可)" << std::endl;
    std::cout << "      --reset                 : マッピング設定をリセット" << std::endl;
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
        else if (command == "get")
        {
            if (argc < 3)
            {
                std::cerr << "Error: get command requires a monitor ID" << std::endl;
                return 1;
            }

            auto monitors = controller.GetMonitors();
            int monitorIndex = std::stoi(argv[2]);
            if (monitorIndex < 0 || monitorIndex >= static_cast<int>(monitors.size()))
            {
                std::cerr << "Error: Invalid monitor ID" << std::endl;
                return 1;
            }

            int brightness = controller.GetBrightness(monitors[monitorIndex].id);
            std::cout << "Monitor " << monitorIndex << " brightness: " << brightness << "%" << std::endl;
        }
        else if (command == "set")
        {
            if (argc < 4)
            {
                std::cerr << "Error: set command requires a monitor ID and brightness value" << std::endl;
                return 1;
            }

            auto monitors = controller.GetMonitors();
            int monitorIndex = std::stoi(argv[2]);
            if (monitorIndex < 0 || monitorIndex >= static_cast<int>(monitors.size()))
            {
                std::cerr << "Error: Invalid monitor ID" << std::endl;
                return 1;
            }

            int brightness = std::stoi(argv[3]);
            if (controller.SetBrightness(monitors[monitorIndex].id, brightness))
            {
                std::cout << "Monitor " << monitorIndex << " brightness set to " << brightness << "%" << std::endl;
            }
            else
            {
                std::cerr << "Error: Failed to set brightness" << std::endl;
                return 1;
            }
        }
        else if (command == "setall")
        {
            if (argc < 3)
            {
                std::cerr << "Error: setall command requires a brightness value" << std::endl;
                return 1;
            }

            int brightness = std::stoi(argv[2]);
            if (controller.SetUnifiedBrightness(brightness))
            {
                std::cout << "All monitors brightness set to " << brightness << "%" << std::endl;
            }
            else
            {
                std::cerr << "Error: Failed to set brightness for some monitors" << std::endl;
                return 1;
            }
        }
        else if (command == "map")
        {
            if (argc < 4)
            {
                std::cerr << "Error: map command requires a monitor ID and options" << std::endl;
                return 1;
            }

            auto monitors = controller.GetMonitors();
            int monitorIndex = std::stoi(argv[2]);
            if (monitorIndex < 0 || monitorIndex >= static_cast<int>(monitors.size()))
            {
                std::cerr << "Error: Invalid monitor ID" << std::endl;
                return 1;
            }

            MonitorId id = monitors[monitorIndex].id;
            MappingConfig config = controller.GetMappingConfig(id);

            for (int i = 3; i < argc; i++)
            {
                std::string option = argv[i];
                if (option == "--min" && i + 1 < argc)
                {
                    config.minBrightness = std::stoi(argv[++i]);
                }
                else if (option == "--max" && i + 1 < argc)
                {
                    config.maxBrightness = std::stoi(argv[++i]);
                }
                else if (option == "--point" && i + 1 < argc)
                {
                    std::string point = argv[++i];
                    size_t comma = point.find(',');
                    if (comma != std::string::npos)
                    {
                        int in = std::stoi(point.substr(0, comma));
                        int out = std::stoi(point.substr(comma + 1));
                        config.mappingPoints.push_back({in, out});
                    }
                }
                else if (option == "--reset")
                {
                    config = MappingConfig();
                }
            }

            controller.SetMappingConfig(id, config);
            std::cout << "Mapping configuration updated for monitor " << monitorIndex << std::endl;
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
