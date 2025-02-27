#include "MonitorController.h"
#include <memory>

MonitorController::MonitorController()
{
}

std::vector<MonitorController::MonitorInfo> MonitorController::GetMonitors()
{
    std::vector<MonitorInfo> monitors;
    if (!EnumDisplayMonitors(nullptr, nullptr, MonitorEnumProc, reinterpret_cast<LPARAM>(&monitors)))
    {
        DWORD error = GetLastError();
        throw WindowsApiException("Failed to enumerate monitors: " + std::to_string(error));
    }
    return monitors;
}

BOOL CALLBACK MonitorController::MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
    auto monitors = reinterpret_cast<std::vector<MonitorInfo>*>(dwData);

    MONITORINFOEXW monitorInfo = { sizeof(MONITORINFOEXW) };
    if (!GetMonitorInfoW(hMonitor, &monitorInfo))
    {
        return TRUE; // Continue enumeration even if we fail to get info for one monitor
    }

    MonitorInfo info;
    info.deviceName = monitorInfo.szDevice;
    info.isPrimary = (monitorInfo.dwFlags & MONITORINFOF_PRIMARY) != 0;
    info.bounds = monitorInfo.rcMonitor;

    monitors->push_back(info);
    return TRUE;
}
