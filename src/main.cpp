#include "MonitorController.h"
#include <iostream>
#include <iomanip>
#include <windows.h>

void PrintMonitorInfo(const MonitorController::MonitorInfo& info)
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
            PrintMonitorInfo(monitor);
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
