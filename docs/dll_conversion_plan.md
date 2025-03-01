# Dynamic Library変換計画

## 概要
MonitorControllerLibをStatic LibraryからDynamic Library（DLL）に変換する作業の計画です。

## 必要な変更

### 1. MonitorController.hの変更
```cpp
// DLLエクスポート/インポートマクロの追加
#ifdef DISPLAYCONTROLLERLIB_EXPORTS
    #define DISPLAYCONTROLLER_API __declspec(dllexport)
#else
    #define DISPLAYCONTROLLER_API __declspec(dllimport)
#endif

// 各publicクラスにマクロを適用
class DISPLAYCONTROLLER_API DisplayControllerException : public std::runtime_error {
    // ...
};

class DISPLAYCONTROLLER_API WindowsApiException : public DisplayControllerException {
    // ...
};

class DISPLAYCONTROLLER_API MonitorController : public IMonitorManager, public IBrightnessMapper, public IMonitorController {
    // ...
};
```

### 2. CMakeLists.txtの変更
```cmake
# DLLビルドの設定
add_library(DisplayControllerLib SHARED ${LIB_SOURCES})

# DLLエクスポートマクロの定義
target_compile_definitions(DisplayControllerLib
    PRIVATE
        DISPLAYCONTROLLERLIB_EXPORTS
)
```

## 実装手順
1. MonitorController.hにDLLエクスポート/インポートマクロを追加
2. CMakeLists.txtをDLL用に修正
3. ビルドしてDLLが正しく生成されることを確認
4. メインアプリケーションが正しくDLLを使用できることを確認

## 注意点
- DLLのエクスポート/インポートマクロは、Windowsプラットフォームでのみ必要
- クラスのメンバー関数は自動的にエクスポートされる
- テンプレートクラスやインライン関数はDLLでエクスポートできない
