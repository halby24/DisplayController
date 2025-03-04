# DisplayController

DisplayControllerは、環境光センサーを使用してモニターの明るさを自動調整するデーモンプログラムです。プラグイン方式を採用しており、様々な種類の光センサーに対応可能です。

## 主な機能

- 環境光に基づく自動的な画面の明るさ調整
- プラグインによる拡張可能な光センサー対応
- 複数モニターのサポート
- 滑らかな明るさ調整
- 設定可能な更新間隔とスムージング係数

## 対応センサー

- SwitchBotライトセンサー（公式対応）
- その他のセンサー（プラグインで拡張可能）

## インストール方法

1. リリースページから最新のバイナリをダウンロード
2. 必要なプラグインを`plugins`ディレクトリに配置
3. `config.json`を設定

詳細なインストール手順は[docs/user/getting_started.md](docs/user/getting_started.md)を参照してください。

## 基本的な使用方法

1. 設定ファイル（config.json）を作成
```json
{
  "monitors": [
    {
      "name": "DELL P2419H",
      "brightness_range": {
        "min": 0,
        "max": 100
      }
    }
  ],
  "plugins": {
    "SwitchBotLightSensor": {
      "device_id": "YOUR_DEVICE_ID",
      "token": "YOUR_TOKEN"
    }
  },
  "brightness_control": {
    "update_interval_ms": 5000,
    "smoothing_factor": 0.3
  }
}
```

2. プログラムを起動
```bash
./DisplayController
```

詳細な設定オプションについては[docs/user/configuration.md](docs/user/configuration.md)を参照してください。

## プラグイン開発

DisplayControllerは、プラグインシステムを通じて新しい光センサーの追加をサポートしています。

### プラグインの基本構造

```
plugins/YourSensorName/
├── CMakeLists.txt
├── include/
│   ├── YourSensorPlugin.h
│   └── YourSensor.h
└── src/
    ├── YourSensorPlugin.cpp
    └── YourSensor.cpp
```

プラグイン開発の詳細については[docs/developer/plugin_development.md](docs/developer/plugin_development.md)を参照してください。

## トラブルシューティング

問題が発生した場合は、以下を確認してください：

- 設定ファイルの構文が正しいか
- プラグインが正しく配置されているか
- センサーの接続状態
- ログファイルの内容

## ドキュメント

- [ユーザーガイド](docs/user/getting_started.md)
- [設定ガイド](docs/user/configuration.md)
- [APIリファレンス](docs/developer/api_reference.md)
- [アーキテクチャ概要](docs/developer/architecture.md)
- [プラグイン開発ガイド](docs/developer/plugin_development.md)

## ライセンス

このプロジェクトはMITライセンスの下で公開されています。詳細は[LICENSE](LICENSE)ファイルを参照してください。

## あとがき

このアプリケーションは99.9%Roo CodeとClaude, Geminiが作ってくれました。ありがとう✨
