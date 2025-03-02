#ifndef DISPLAYCONTROLLER_STRING_UTILS_H
#define DISPLAYCONTROLLER_STRING_UTILS_H

#include <string>
#include <windows.h>

// 共通ライブラリのエクスポート設定
#ifdef COMMON_LIB_EXPORTS
    #define COMMON_API __declspec(dllexport)
#else
    #define COMMON_API __declspec(dllimport)
#endif

class COMMON_API StringUtils {
public:
    // ワイド文字列からUTF-8文字列への変換
    static std::string WideToUtf8(const std::wstring& wide);

    // UTF-8文字列からワイド文字列への変換
    static std::wstring Utf8ToWide(const std::string& utf8);

    // システムデフォルトエンコーディングからUTF-8への変換
    static std::string SystemToUtf8(const std::string& system);

    /**
     * @brief エラーメッセージをコンソールに出力
     * @param message UTF-8エンコードされたエラーメッセージ
     * @note メッセージは自動的に適切なエンコーディングに変換されて出力されます
     */
    static void OutputErrorMessage(const std::string& message);

    /**
     * @brief 例外からエラーメッセージを抽出してコンソールに出力
     * @param e 出力する例外オブジェクト
     * @note 例外のwhat()メッセージは自動的に適切なエンコーディングに変換されて出力されます
     */
    static void OutputExceptionMessage(const std::exception& e);

    /**
     * @brief メッセージをコンソールに出力
     * @param message UTF-8エンコードされたメッセージ
     * @note メッセージは自動的に適切なエンコーディングに変換されて出力されます
     */
    static void OutputMessage(const std::string& message);

private:
    // Windows APIのエラーメッセージを取得
    static std::string GetLastErrorMessage();
};

#endif // DISPLAYCONTROLLER_STRING_UTILS_H
