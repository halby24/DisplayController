#include "BrightnessManager.h"
#include <algorithm>
#include <stdexcept>

BrightnessManager::BrightnessManager(std::unique_ptr<ILightSensor> sensor)
    : m_sensor(std::move(sensor))
    , m_controller(std::make_unique<MonitorController>())
    , m_isRunning(false)
    , m_updateInterval(std::chrono::seconds(5))
    , m_minBrightness(20)
    , m_maxBrightness(100)
{
    if (!m_sensor) {
        throw std::invalid_argument("センサーがnullです");
    }
}

BrightnessManager::~BrightnessManager()
{
    StopSync();
}

void BrightnessManager::StartSync()
{
    if (!m_isRunning) {
        m_isRunning = true;
        m_syncThread = std::thread(&BrightnessManager::SyncLoop, this);
    }
}

void BrightnessManager::StopSync()
{
    if (m_isRunning) {
        m_isRunning = false;
        if (m_syncThread.joinable()) {
            m_syncThread.join();
        }
    }
}

void BrightnessManager::UpdateBrightness()
{
    try {
        // 照度レベルの取得と輝度値の計算
        int lightLevel = m_sensor->GetLightLevel();
        int brightness = CalculateBrightness(lightLevel);

        // すべてのモニターの輝度を統一的に設定
        m_controller->SetUnifiedBrightness(brightness);
    }
    catch (const std::exception& e) {
        // TODO: エラーログ機能の実装
        // とりあえず例外は握りつぶす（常駐プログラムは停止させない）
    }
}

void BrightnessManager::SetUpdateInterval(std::chrono::milliseconds interval)
{
    if (interval.count() < 1000) {
        throw std::invalid_argument("更新間隔は1秒以上である必要があります");
    }
    m_updateInterval = interval;
}

void BrightnessManager::SetBrightnessRange(int minBrightness, int maxBrightness)
{
    if (minBrightness < 0 || maxBrightness > 100 || minBrightness >= maxBrightness) {
        throw std::invalid_argument("不正な輝度範囲が指定されました");
    }
    m_minBrightness = minBrightness;
    m_maxBrightness = maxBrightness;
}

void BrightnessManager::SyncLoop()
{
    while (m_isRunning) {
        UpdateBrightness();
        std::this_thread::sleep_for(m_updateInterval);
    }
}

int BrightnessManager::CalculateBrightness(int lightLevel) const
{
    // 照度レベル（0-100）から輝度（m_minBrightness-m_maxBrightness）への線形変換
    return static_cast<int>(
        m_minBrightness +
        (static_cast<double>(lightLevel) / 100.0) *
        (m_maxBrightness - m_minBrightness)
    );
}
