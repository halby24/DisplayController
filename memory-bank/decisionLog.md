# 技術的決定ログ

## 2025/03/02 - StringUtilsの共通コンポーネント化

### 決定事項
1. StringUtilsを共通コンポーネントとして分離
   - 場所: src/common/
   - 形式: 静的ライブラリ（DisplayControllerCommon）

### 理由
- 複数のコンポーネントで共通して使用される文字列操作機能
- コードの重複を避け、保守性を向上
- 一貫した文字列処理の実装を提供

### 影響範囲
- DisplayControllerライブラリ
- SwitchBotLightSensorプラグイン
- その他の文字列処理を必要とするコンポーネント

### 実装詳細
1. CMake設定
   - 共通ライブラリのターゲット作成
   - インクルードパスの設定
   - DLLエクスポート設定の追加

2. 依存関係の管理
   - DisplayControllerライブラリがPUBLICで依存
   - プラグインがPRIVATEで依存

### 注意点
- UTF-8エンコーディングの一貫した使用
- Windows APIのUnicode版の使用
- DLLエクスポート設定の適切な管理
