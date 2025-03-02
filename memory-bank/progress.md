# ビルドエラー修正の進捗状況 (2025/03/02)

## 完了した作業

1. CMakeLists.txtの修正
   - C++20を有効化
   - UTF-8エンコーディングを強制するコンパイラオプションを追加
   - Unicode文字セットを有効化
   - コンパイラオプションの重複を解消（/utf-8と/source-charset:utf-8）

2. BrightnessDaemon.cppの修正
   - 文字列リテラルの改行を修正
   - Windows APIの呼び出しをUnicode版に統一
   - 日本語文字列の扱いを改善

3. SwitchBotLightSensor/CMakeLists.txtの修正
   - C++20を有効化
   - UTF-8エンコーディングを強制するコンパイラオプションを追加
   - インクルードパスの修正（${CMAKE_SOURCE_DIR}を追加）

4. ConfigManager.cppの修正
   - 文字列リテラルの改行問題を解決
   - パス操作をstd::filesystem::pathを使用するように改善
   - エンコーディング関連の問題を解決
   - u8プレフィックスを削除して文字列リテラルを修正

## 残作業

1. ConfigManager.cppの文字列リテラルエラーの修正
   - 文字列リテラルの構文エラーの解決
   - 文字列連結の問題解決

2. SwitchBotLightSensorのビルドエラー修正
   - インクルードパスの問題解決
   - 依存関係の解決

3. テストのリンクエラー修正
   - PluginLoader.libの参照問題の解決

## 次のステップ

1. ConfigManager.cppの文字列リテラルエラーの修正
2. SwitchBotLightSensorのビルドエラー修正
3. テストのリンクエラー修正

## 注意点

- すべてのソースファイルをUTF-8（BOMあり）で保存する必要がある
- 文字列リテラルの改行は避け、1行で記述するか、文字列連結演算子を使用する
- Windows APIの呼び出しはUnicode版（*W関数）に統一する
