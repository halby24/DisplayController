#include "StringUtils.h"
#include <stdexcept>
#include <vector>
#include <iostream>

std::string StringUtils::WideToUtf8(const std::wstring& wide) {
    if (wide.empty()) {
        return std::string();
    }

    // 必要なバッファサイズを計算
    int size = WideCharToMultiByte(
        CP_UTF8,                // UTF-8を使用
        0,                      // デフォルトフラグ
        wide.c_str(),          // 入力文字列
        static_cast<int>(wide.length()), // 入力長
        nullptr,               // 出力バッファ（サイズ計算用にnull）
        0,                     // 出力バッファサイズ（サイズ計算用に0）
        nullptr,              // デフォルト文字（未使用）
        nullptr               // デフォルト文字使用フラグ（未使用）
    );

    if (size <= 0) {
        throw std::runtime_error("WideCharToMultiByte failed: " + GetLastErrorMessage());
    }

    // 変換実行
    std::vector<char> buffer(size);
    if (WideCharToMultiByte(
        CP_UTF8, 0,
        wide.c_str(), static_cast<int>(wide.length()),
        buffer.data(), size,
        nullptr, nullptr) <= 0) {
        throw std::runtime_error("WideCharToMultiByte failed: " + GetLastErrorMessage());
    }

    return std::string(buffer.data(), size);
}

std::wstring StringUtils::Utf8ToWide(const std::string& utf8) {
    if (utf8.empty()) {
        return std::wstring();
    }

    // 必要なバッファサイズを計算
    int size = MultiByteToWideChar(
        CP_UTF8,                // UTF-8を使用
        0,                      // デフォルトフラグ
        utf8.c_str(),          // 入力文字列
        static_cast<int>(utf8.length()), // 入力長
        nullptr,               // 出力バッファ（サイズ計算用にnull）
        0                      // 出力バッファサイズ（サイズ計算用に0）
    );

    if (size <= 0) {
        throw std::runtime_error("MultiByteToWideChar failed: " + GetLastErrorMessage());
    }

    // 変換実行
    std::vector<wchar_t> buffer(size);
    if (MultiByteToWideChar(
        CP_UTF8, 0,
        utf8.c_str(), static_cast<int>(utf8.length()),
        buffer.data(), size) <= 0) {
        throw std::runtime_error("MultiByteToWideChar failed: " + GetLastErrorMessage());
    }

    return std::wstring(buffer.data(), size);
}

std::string StringUtils::SystemToUtf8(const std::string& system) {
    if (system.empty()) {
        return std::string();
    }

    // システムデフォルトエンコーディング → ワイド文字列 → UTF-8 の2段階変換
    std::wstring wide = Utf8ToWide(system);
    return WideToUtf8(wide);
}

std::string StringUtils::GetLastErrorMessage() {
    DWORD error = GetLastError();
    if (error == 0) {
        return "No error";
    }

    LPWSTR buffer = nullptr;
    size_t size = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        error,
        MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT),
        reinterpret_cast<LPWSTR>(&buffer),
        0,
        nullptr
    );

    if (size == 0) {
        return "Unknown error";
    }

    std::wstring wide(buffer, size);
    LocalFree(buffer);

    // 末尾の改行を削除
    while (!wide.empty() && (wide.back() == L'\n' || wide.back() == L'\r')) {
        wide.pop_back();
    }

    return WideToUtf8(wide);
}

void StringUtils::OutputErrorMessage(const std::string& message) {
    if (message.empty()) {
        return;
    }

    // UTF-8からワイド文字列に変換
    std::wstring wide = Utf8ToWide(message);
    wide += L"\n";  // 改行を追加

    // 標準エラー出力のハンドルを取得
    HANDLE hStderr = GetStdHandle(STD_ERROR_HANDLE);
    if (hStderr == INVALID_HANDLE_VALUE) {
        return;
    }

    // エラーメッセージを出力
    DWORD written;
    WriteConsoleW(hStderr, wide.c_str(), static_cast<DWORD>(wide.length()), &written, nullptr);
}

void StringUtils::OutputExceptionMessage(const std::exception& e) {
    // 例外のメッセージを取得して出力
    OutputErrorMessage(e.what());
}

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
