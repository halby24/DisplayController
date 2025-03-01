#ifndef DISPLAYCONTROLLER_BRIGHTNESSMANAGER_H
#define DISPLAYCONTROLLER_BRIGHTNESSMANAGER_H

#include "ILightSensor.h"
#include "MonitorController.h"
#include <memory>
#include <thread>
#include <atomic>
#include <chrono>

class BrightnessManager {
public:
    explicit BrightnessManager(std::unique_ptr<ILightSensor> sensor);
    ~BrightnessManager();

    // コピー禁止
    BrightnessManager(const BrightnessManager&) = delete;
    BrightnessManager& operator=(const BrightnessManager&) = delete;

    // 同期処理の制御
    void StartSync();
    void StopSync();
    void UpdateBrightness();

    // 設定
    void SetUpdateInterval(std::chrono::milliseconds interval);
    void SetBrightnessRange(int minBrightness, int maxBrightness);

private:
    void SyncLoop();
    int CalculateBrightness(int lightLevel) const;

    std::unique_ptr<ILightSensor> m_sensor;
    std::unique_ptr<MonitorController> m_controller;
    std::atomic<bool> m_isRunning;
    std::thread m_syncThread;
    std::chrono::milliseconds m_updateInterval;

    // 輝度の範囲設定
    int m_minBrightness;
    int m_maxBrightness;
};

#endif // DISPLAYCONTROLLER_BRIGHTNESSMANAGER_H
