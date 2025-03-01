# vcpkg への nlohmann-json 移行計画

## 概要
vcpkg を導入したため、サブディレクトリで管理していた nlohmann-json を vcpkg の find_package システムを使用するように移行します。

## 必要な変更

### CMakeLists.txt の修正

1. find_package の追加
```cmake
# Find required packages
find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json REQUIRED)  # 追加
```

2. サブディレクトリの削除
以下の行を削除：
```cmake
# Disable JSON library tests
set(JSON_BuildTests OFF CACHE INTERNAL "")

# Add nlohmann_json
add_subdirectory(ThirdPartyLib/nlohmann_json)
```

3. インクルードディレクトリの参照修正
以下の行を削除：
```cmake
ThirdPartyLib/nlohmann_json/include
```

### ディレクトリの削除
- ThirdPartyLib/nlohmann_json ディレクトリを削除

## 確認事項
- vcpkg.json に nlohmann-json が依存関係として追加されていることを確認済み
- 既存のターゲットは nlohmann_json::nlohmann_json を正しくリンクしているため、変更不要

## 実装手順
1. Code モードに切り替えて CMakeLists.txt を修正
2. ThirdPartyLib/nlohmann_json ディレクトリを削除
3. ビルドして動作確認
