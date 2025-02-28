#ifndef DISPLAYCONTROLLER_BRIGHTNESS_SYNC_H
#define DISPLAYCONTROLLER_BRIGHTNESS_SYNC_H

#include <windows.h>
#include <string>
#include <memory>
#include <filesystem>
#include "SystemBrightnessMonitor.h"
#include "MonitorController.h"

class BrightnessSync {
public:
    BrightnessSync(MonitorController& monitorController);
    ~BrightnessSync();

    // 同期の有効化/無効化
    void EnableSync(bool enable);
    bool IsSyncEnabled() const;

    // 同期処理
    void SyncToSystem(HMONITOR hMonitor, int brightness);
    void SyncToMonitor(int systemBrightness);

    // 設定の保存と読み込み
    void SaveSyncState();
    void LoadSyncState();

private:
    // 輝度値の変換
    int MonitorToSystemBrightness(int monitorBrightness) const;
    int SystemToMonitorBrightness(int systemBrightness) const;

    // システム輝度変更のコールバック
    void OnSystemBrightnessChanged(int newBrightness);

    // メンバー変数
    MonitorController& m_monitorController;
    std::unique_ptr<SystemBrightnessMonitor> m_systemMonitor;
    bool m_syncEnabled;
    bool m_isSyncing;  // 再帰的な同期を防ぐためのフラグ

    // 設定ファイルのパス
    std::filesystem::path GetSettingsPath() const;
};

#endif // DISPLAYCONTROLLER_BRIGHTNESS_SYNC_H
