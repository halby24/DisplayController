#include "MonitorController.h"
#include <common/StringUtils.h>
#include <iostream>
#include <iomanip>
#include <windows.h>

void PrintMonitorInfo(const MonitorController::MonitorInfo& info, MonitorController& controller)
{
    // デバイス名をUTF-8に変換
    std::string deviceName = StringUtils::WideToUtf8(info.deviceName);

    StringUtils::OutputMessage("Device: " + deviceName);
    StringUtils::OutputMessage("Primary: " + std::string(info.isPrimary ? "Yes" : "No"));
    StringUtils::OutputMessage("Position: Left=" + std::to_string(info.bounds.left) +
                             ", Top=" + std::to_string(info.bounds.top) +
                             ", Right=" + std::to_string(info.bounds.right) +
                             ", Bottom=" + std::to_string(info.bounds.bottom));
    StringUtils::OutputMessage("Resolution: " +
                             std::to_string(info.bounds.right - info.bounds.left) + "x" +
                             std::to_string(info.bounds.bottom - info.bounds.top));

    // Get detailed monitor info
    try {
        controller.GetDetailedMonitorInfo(const_cast<MonitorController::MonitorInfo&>(info));
        StringUtils::OutputMessage("Manufacturer: " + StringUtils::WideToUtf8(info.manufacturerName));
        StringUtils::OutputMessage("Product Code: " + StringUtils::WideToUtf8(info.productCode));
        StringUtils::OutputMessage("Serial Number: " + StringUtils::WideToUtf8(info.serialNumber));
        StringUtils::OutputMessage("Friendly Name: " + StringUtils::WideToUtf8(info.friendlyName));

        // Load and display saved settings
        auto settings = controller.LoadMonitorSettings(info);
        StringUtils::OutputMessage("Saved Settings:");
        StringUtils::OutputMessage("  Brightness: " + std::to_string(settings.brightness) + "%");
        StringUtils::OutputMessage("  Contrast: " + std::to_string(settings.contrast) + "%");
        StringUtils::OutputMessage("  Color Temperature: " + std::to_string(settings.colorTemperature) + "K");

        // Display mapping configuration
        auto mapping = controller.GetMappingConfig(info.id);
        StringUtils::OutputMessage("Brightness Mapping:");
        StringUtils::OutputMessage("  Min: " + std::to_string(mapping.minBrightness) + "%");
        StringUtils::OutputMessage("  Max: " + std::to_string(mapping.maxBrightness) + "%");
        if (!mapping.mappingPoints.empty()) {
            StringUtils::OutputMessage("  Custom Points:");
            for (const auto& point : mapping.mappingPoints) {
                StringUtils::OutputMessage("    " + std::to_string(point.first) + "% -> " +
                                         std::to_string(point.second) + "%");
            }
        }
    }
    catch (const WindowsApiException& e) {
        StringUtils::OutputMessage("Detailed Info: Unable to retrieve (Error: " + std::string(e.what()) + ")");
    }

    // Get and display brightness
    try {
        int brightness = controller.GetBrightness(info.id);
        StringUtils::OutputMessage("Current Brightness: " + std::to_string(brightness) + "%");
    }
    catch (const WindowsApiException& e) {
        StringUtils::OutputMessage("Brightness: Unable to retrieve (Error: " + std::string(e.what()) + ")");
    }

    // Get and display monitor capabilities
    try {
        auto caps = controller.GetMonitorCapabilities(info.id);
        StringUtils::OutputMessage("Display Technology: " + caps.technologyType);
        StringUtils::OutputMessage("Capabilities:");
        StringUtils::OutputMessage("  - Brightness Control: " + std::string(caps.supportsBrightness ? "Yes" : "No"));
        StringUtils::OutputMessage("  - Contrast Control: " + std::string(caps.supportsContrast ? "Yes" : "No"));
        StringUtils::OutputMessage("  - Color Temperature: " + std::string(caps.supportsColorTemperature ? "Yes" : "No"));
        if (caps.displaySize.cx > 0 && caps.displaySize.cy > 0) {
            StringUtils::OutputMessage("Display Size: " + std::to_string(caps.displaySize.cx) + "mm x " +
                                     std::to_string(caps.displaySize.cy) + "mm");
        }
        if (caps.supportsColorTemperature && caps.colorTemperature > 0) {
            StringUtils::OutputMessage("Color Temperature: " + std::to_string(caps.colorTemperature) + "K");
        }
    }
    catch (const WindowsApiException& e) {
        StringUtils::OutputMessage("Monitor Capabilities: Unable to retrieve");
    }

    StringUtils::OutputMessage("-------------------");
}

void PrintUsage()
{
    StringUtils::OutputMessage("Usage: DisplayController.exe <command> [options]");
    StringUtils::OutputMessage("\nCommands:");
    StringUtils::OutputMessage("  list                        : モニター一覧の表示");
    StringUtils::OutputMessage("  get <monitor_id>            : 指定モニターの輝度取得");
    StringUtils::OutputMessage("  set <monitor_id> <value>    : 指定モニターの輝度設定");
    StringUtils::OutputMessage("  setall <value>              : 全モニターの輝度を統一的に設定");
    StringUtils::OutputMessage("  map <monitor_id> <options>  : モニターの輝度マッピング設定");
    StringUtils::OutputMessage("    options:");
    StringUtils::OutputMessage("      --min <value>           : 最小輝度値 (0-100)");
    StringUtils::OutputMessage("      --max <value>           : 最大輝度値 (0-100)");
    StringUtils::OutputMessage("      --point <in,out>        : マッピングポイントを追加 (複数指定可)");
    StringUtils::OutputMessage("      --reset                 : マッピング設定をリセット");
    StringUtils::OutputMessage("  help                        : このヘルプを表示");
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
            StringUtils::OutputMessage("Found " + std::to_string(monitors.size()) + " monitor(s):");
            StringUtils::OutputMessage("===================");
            for (const auto& monitor : monitors)
            {
                PrintMonitorInfo(monitor, controller);
            }
        }
        else if (command == "get")
        {
            if (argc < 3)
            {
                StringUtils::OutputErrorMessage("Error: get command requires a monitor ID");
                return 1;
            }

            auto monitors = controller.GetMonitors();
            int monitorIndex = std::stoi(argv[2]);
            if (monitorIndex < 0 || monitorIndex >= static_cast<int>(monitors.size()))
            {
                StringUtils::OutputErrorMessage("Error: Invalid monitor ID");
                return 1;
            }

            int brightness = controller.GetBrightness(monitors[monitorIndex].id);
            StringUtils::OutputMessage("Monitor " + std::to_string(monitorIndex) + " brightness: " +
                                     std::to_string(brightness) + "%");
        }
        else if (command == "set")
        {
            if (argc < 4)
            {
                StringUtils::OutputErrorMessage("Error: set command requires a monitor ID and brightness value");
                return 1;
            }

            auto monitors = controller.GetMonitors();
            int monitorIndex = std::stoi(argv[2]);
            if (monitorIndex < 0 || monitorIndex >= static_cast<int>(monitors.size()))
            {
                StringUtils::OutputErrorMessage("Error: Invalid monitor ID");
                return 1;
            }

            int brightness = std::stoi(argv[3]);
            if (controller.SetBrightness(monitors[monitorIndex].id, brightness))
            {
                StringUtils::OutputMessage("Monitor " + std::to_string(monitorIndex) + " brightness set to " +
                                         std::to_string(brightness) + "%");
            }
            else
            {
                StringUtils::OutputErrorMessage("Error: Failed to set brightness");
                return 1;
            }
        }
        else if (command == "setall")
        {
            if (argc < 3)
            {
                StringUtils::OutputErrorMessage("Error: setall command requires a brightness value");
                return 1;
            }

            int brightness = std::stoi(argv[2]);
            if (controller.SetUnifiedBrightness(brightness))
            {
                StringUtils::OutputMessage("All monitors brightness set to " + std::to_string(brightness) + "%");
            }
            else
            {
                StringUtils::OutputErrorMessage("Error: Failed to set brightness for some monitors");
                return 1;
            }
        }
        else if (command == "map")
        {
            if (argc < 4)
            {
                StringUtils::OutputErrorMessage("Error: map command requires a monitor ID and options");
                return 1;
            }

            auto monitors = controller.GetMonitors();
            int monitorIndex = std::stoi(argv[2]);
            if (monitorIndex < 0 || monitorIndex >= static_cast<int>(monitors.size()))
            {
                StringUtils::OutputErrorMessage("Error: Invalid monitor ID");
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
            StringUtils::OutputMessage("Mapping configuration updated for monitor " + std::to_string(monitorIndex));
        }
        else
        {
            StringUtils::OutputErrorMessage("Error: Unknown command '" + command + "'");
            PrintUsage();
            return 1;
        }

        return 0;
    }
    catch (const DisplayControllerException& e)
    {
        StringUtils::OutputExceptionMessage(e);
        return 1;
    }
    catch (const std::exception& e)
    {
        StringUtils::OutputErrorMessage("Unexpected error: " + std::string(e.what()));
        return 1;
    }
}
