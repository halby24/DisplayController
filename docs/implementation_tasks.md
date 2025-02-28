# 実装タスク

## 1. システム輝度連携機能の削除
- SystemBrightnessMonitor クラスの削除
- BrightnessSync クラスの削除
- MonitorController からの関連コードの削除

## 2. 新しいインターフェースの実装

### 2.1 IMonitorManager の実装
```cpp
class IMonitorManager {
    virtual std::vector<MonitorInfo> EnumerateMonitors() = 0;
    virtual MonitorInfo GetMonitorInfo(MonitorId id) = 0;
};
```

### 2.2 IBrightnessMapper の実装
```cpp
class IBrightnessMapper {
    virtual int MapBrightness(MonitorId id, int normalizedBrightness) = 0;
    virtual void SetMappingConfig(MonitorId id, const MappingConfig& config) = 0;
    virtual MappingConfig GetMappingConfig(MonitorId id) = 0;
};
```

### 2.3 IMonitorController の実装
```cpp
class IMonitorController {
    virtual bool SetBrightness(MonitorId id, int brightness) = 0;
    virtual int GetBrightness(MonitorId id) = 0;
    virtual bool SetUnifiedBrightness(int normalizedBrightness) = 0;
};
```

## 3. 輝度マッピング機能の実装

### 3.1 MappingConfig クラスの実装
- 最小/最大輝度値の管理
- マッピングポイントの管理
- 設定の永続化

### 3.2 輝度マッピングロジック
- 線形マッピング
- カスタムポイントによるマッピング
- マッピング値の検証

## 4. コマンドライン機能の更新
- 新しいコマンドの追加
- ヘルプメッセージの更新
- エラーメッセージの改善

## 5. テスト実装
- 各インターフェースのユニットテスト
- 輝度マッピングのテスト
- 統合テスト
