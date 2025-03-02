# 共通ライブラリのCMake設定
cmake_minimum_required(VERSION 3.15)

# 共通ライブラリのターゲットを作成
add_library(DisplayControllerCommon STATIC
    StringUtils.cpp
)

# コンパイル定義を設定
target_compile_definitions(DisplayControllerCommon
    PRIVATE
        COMMON_LIB_EXPORTS
        _UNICODE
        UNICODE
)

# インクルードディレクトリを設定
target_include_directories(DisplayControllerCommon
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# C++20を使用
target_compile_features(DisplayControllerCommon PRIVATE cxx_std_20)

# Windows環境でのUTF-8強制とコンパイラ警告レベル
if(MSVC)
    target_compile_options(DisplayControllerCommon PRIVATE /utf-8 /W4)
endif()
