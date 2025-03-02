# ConfigManager拡張設計

## 追加する機能

### デバイス管理機能の拡張

1. デバイスタイプによるフィルタリング
```cpp
// 指定したタイプのデバイスリストを取得
std::vector<nlohmann::json> GetDevicesByType(const std::string& type) const;
```

2. 最初の利用可能なデバイスを取得
```cpp
// 指定したタイプの最初のデバイスを取得
nlohmann::json GetFirstDeviceByType(const std::string& type) const;
```

3. デバイスの存在確認
```cpp
// 指定した名前のデバイスが存在するか確認
bool HasDevice(const std::string& name) const;
// 指定したタイプのデバイスが存在するか確認
bool HasDeviceType(const std::string& type) const;
```

### エラー処理の改善

1. エラーメッセージの具体化
- デバイスが見つからない場合のエラーメッセージをより詳細に
- デバイスタイプが不正な場合のエラーメッセージを追加
- 設定ファイルの状態に応じたエラーメッセージを提供

2. 設定ファイルの検証強化
- デバイスタイプの妥当性チェック
- 必須フィールドの存在確認の強化
- デバイス設定の整合性チェック

## 実装の注意点

1. パフォーマンス考慮事項
- デバイスリストのキャッシュ化を検討
- 頻繁なファイルI/Oを避ける
- 検索処理の効率化

2. スレッドセーフティ
- 設定読み込み時のスレッドセーフティ確保
- デバイスリスト取得時の整合性保証

3. バックワードコンパティビリティ
- 既存のAPIとの互換性維持
- 設定ファイルフォーマットの下位互換性確保

## 使用例

```cpp
// BrightnessDaemon.cppでの使用例
std::unique_ptr<ILightSensor> CreateLightSensor()
{
    try
    {
        auto& config = ConfigManager::Instance();
        config.Load();

        // Light Sensorタイプの最初のデバイスを取得
        if (config.HasDeviceType("Light Sensor"))
        {
            auto device = config.GetFirstDeviceByType("Light Sensor");
            return std::make_unique<SwitchBotLightSensor>(device["name"].get<std::string>());
        }
        else
        {
            ShowErrorMessage(
                "Light Sensorタイプのデバイスが設定されていません。\n"
                "ダミーセンサーを使用します。",
                "警告",
                MB_OK | MB_ICONWARNING
            );
            return std::make_unique<DummyLightSensor>();
        }
    }
    catch (const ConfigException& e)
    {
        ShowErrorMessage(
            std::string("設定の読み込みに失敗しました: ") + e.what() + "\n"
            "ダミーセンサーを使用します。",
            "エラー",
            MB_OK | MB_ICONWARNING
        );
    }
    catch (const std::exception& e)
    {
        ShowErrorMessage(
            std::string("SwitchBotの初期化に失敗しました: ") + e.what() + "\n"
            "ダミーセンサーを使用します。",
            "エラー",
            MB_OK | MB_ICONWARNING
        );
    }

    return std::make_unique<DummyLightSensor>();
}
```

## 次のステップ

1. Codeモードでの実装
   - ConfigManager.hの拡張
   - ConfigManager.cppの実装
   - BrightnessDaemon.cppの修正

2. テスト計画
   - 各新機能のユニットテスト作成
   - エラーケースのテスト
   - 統合テストの実施
