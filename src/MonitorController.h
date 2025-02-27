#ifndef DISPLAYCONTROLLER_MONITOR_CONTROLLER_H
#define DISPLAYCONTROLLER_MONITOR_CONTROLLER_H

#include <windows.h>
#include <vector>
#include <string>
#include <stdexcept>

class DisplayControllerException : public std::runtime_error
{
public:
    explicit DisplayControllerException(const std::string& message)
        : std::runtime_error(message) {}
};

class WindowsApiException : public DisplayControllerException
{
public:
    explicit WindowsApiException(const std::string& message)
        : DisplayControllerException(message) {}
};

class MonitorController
{
public:
    struct MonitorInfo
    {
        std::wstring deviceName;
        bool isPrimary;
        RECT bounds;
    };

    MonitorController();
    ~MonitorController() = default;

    std::vector<MonitorInfo> GetMonitors();

private:
    static BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData);
};

#endif // DISPLAYCONTROLLER_MONITOR_CONTROLLER_H
