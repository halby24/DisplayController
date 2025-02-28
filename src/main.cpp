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

int main()
{
    try
    {
        // Set console output to UTF-8
        SetConsoleOutputCP(CP_UTF8);

        MonitorController controller;
        auto monitors = controller.GetMonitors();

        std::cout << "Found " << monitors.size() << " monitor(s):" << std::endl;
        std::cout << "===================" << std::endl;

        for (const auto& monitor : monitors)
        {
            PrintMonitorInfo(monitor, controller);
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
