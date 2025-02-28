#ifndef DISPLAYCONTROLLER_SYSTEM_BRIGHTNESS_MONITOR_H
#define DISPLAYCONTROLLER_SYSTEM_BRIGHTNESS_MONITOR_H

#include <windows.h>
#include <wbemidl.h>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <atomic>

class SystemBrightnessMonitor {
public:
    using BrightnessChangeCallback = std::function<void(int)>;

    // デフォルトコンストラクタ（内部でWMIを初期化）
    SystemBrightnessMonitor();
    // 既存のWMIサービスを使用するコンストラクタ
    explicit SystemBrightnessMonitor(IWbemServices* pWbemServices);
    ~SystemBrightnessMonitor();

    // 監視の開始と停止
    void StartMonitoring();
    void StopMonitoring();

    // コールバック設定
    void SetBrightnessChangeCallback(BrightnessChangeCallback callback);

    // 現在のシステム輝度を取得
    int GetCurrentBrightness();

    // システム輝度を設定
    void SetBrightness(int brightness);

private:
    // WMIイベント監視スレッド
    void MonitoringThread();

    // WMI初期化
    void InitializeWMI();
    void CleanupWMI();

    // WMIイベントハンドリング
    void HandleBrightnessChange(IWbemClassObject* pObject);

    // メンバー変数
    IWbemServices* m_pWbemServices;
    IWbemLocator* m_pWbemLocator;
    bool m_wmiInitialized;

    std::atomic<bool> m_isMonitoring;
    std::unique_ptr<std::thread> m_monitoringThread;
    BrightnessChangeCallback m_brightnessCallback;

    // エラーハンドリング用のヘルパー関数
    void ThrowIfFailed(HRESULT hr, const std::string& message);
};

#endif // DISPLAYCONTROLLER_SYSTEM_BRIGHTNESS_MONITOR_H
