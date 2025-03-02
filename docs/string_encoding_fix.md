# 標準出力の文字化け修正

## 概要
現在、printfなどを使用している標準出力で文字化けが発生している問題を解決するため、StringUtilsクラスに標準出力用のヘルパーメソッドを追加する。

## 実装計画

### StringUtils.hの変更
```cpp
class StringUtils {
public:
    // 既存のメソッド定義...

    /**
     * @brief メッセージをコンソールに出力
     * @param message UTF-8エンコードされたメッセージ
     * @note メッセージは自動的に適切なエンコーディングに変換されて出力されます
     */
    static void OutputMessage(const std::string& message);
};
```

### StringUtils.cppの変更
```cpp
void StringUtils::OutputMessage(const std::string& message) {
    if (message.empty()) {
        return;
    }

    // UTF-8からワイド文字列に変換
    std::wstring wide = Utf8ToWide(message);
    wide += L"\n";  // 改行を追加

    // 標準出力のハンドルを取得
    HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hStdout == INVALID_HANDLE_VALUE) {
        return;
    }

    // メッセージを出力
    DWORD written;
    WriteConsoleW(hStdout, wide.c_str(), static_cast<DWORD>(wide.length()), &written, nullptr);
}
```

## 使用例
```cpp
// Before:
printf("モニターの輝度を%dに設定しました\n", brightness);

// After:
StringUtils::OutputMessage("モニターの輝度を" + std::to_string(brightness) + "に設定しました");
```

## 実装手順
1. StringUtils.hにOutputMessageメソッドを追加
2. StringUtils.cppに実装を追加
3. 既存のprintf使用箇所をOutputMessageに置き換え

## 注意点
- OutputErrorMessageと同様のパターンで実装し、一貫性を保つ
- エラーハンドリングも同様のパターンで実装
- 自動的に改行を追加する仕様はエラー出力と統一

## 次のステップ
1. Codeモードに切り替えて実装
2. 既存コードの修正
3. 動作確認
