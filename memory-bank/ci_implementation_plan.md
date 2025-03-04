# CI/CD実装計画

## 概要
このドキュメントでは、DisplayControllerプロジェクトのCI/CD（継続的インテグレーション/継続的デリバリー）パイプラインの実装計画について詳細に説明します。GitHub Actionsを使用して、コードのプッシュ時に自動ビルド、テスト、およびリリース時のアーティファクト生成を行います。

## CI/CDの目的
- コードの品質保証
- ビルドプロセスの自動化
- テスト実行の自動化
- リリースプロセスの簡素化

## ワークフロー設計

### トリガー
- **プッシュ時**: すべてのブランチへのプッシュでビルドとテストを実行
- **プルリクエスト時**: mainブランチへのプルリクエストでビルドとテストを実行
- **タグプッシュ時**: vX.Y.Z形式のタグがプッシュされた場合、リリースを自動作成

### 環境
- **OS**: Windows Server 2022
- **ビルドツール**: Visual Studio 2022
- **パッケージマネージャ**: vcpkg

### ジョブ構成

#### 1. ビルドとテスト
```yaml
jobs:
  build:
    runs-on: windows-latest
    steps:
      - name: Checkout code
        uses: actions/checkout@v3

      - name: Setup vcpkg
        uses: lukka/run-vcpkg@v11
        with:
          vcpkgGitCommitId: 'efb1e7436979a30c4d3e5ab2375fd8e2e461d541'  # vcpkg.jsonのbaseline値と一致させる

      - name: Configure CMake
        run: |
          cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=${{ github.workspace }}/vcpkg/scripts/buildsystems/vcpkg.cmake

      - name: Build
        run: cmake --build build --config Release

      - name: Run tests
        run: ctest -C Release --test-dir build

      - name: Upload artifacts
        uses: actions/upload-artifact@v3
        with:
          name: display-controller
          path: |
            build/bin/Release/*.exe
            build/bin/Release/*.dll
            build/bin/Release/plugins/*.dll
```

#### 2. リリース作成（タグプッシュ時のみ）
```yaml
  release:
    needs: build
    if: startsWith(github.ref, 'refs/tags/v')
    runs-on: windows-latest
    steps:
      - name: Download artifacts
        uses: actions/download-artifact@v3
        with:
          name: display-controller
          path: display-controller

      - name: Create ZIP archive
        run: |
          Compress-Archive -Path display-controller/* -DestinationPath DisplayController-${{ github.ref_name }}.zip

      - name: Create Release
        uses: softprops/action-gh-release@v1
        with:
          files: DisplayController-${{ github.ref_name }}.zip
          draft: false
          prerelease: false
          generate_release_notes: true
```

## 完全なワークフロー設定

以下は、`.github/workflows/ci.yml`に実装する完全なワークフロー設定です：

```yaml
name: CI/CD Pipeline

on:
  push:
    branches: [ main, develop ]
    tags: [ 'v*' ]
  pull_request:
    branches: [ main ]

jobs:
  build:
    runs-on: windows-latest
    steps:
      - name: Checkout code
        uses: actions/checkout@v3

      - name: Setup vcpkg
        uses: lukka/run-vcpkg@v11
        with:
          vcpkgGitCommitId: 'efb1e7436979a30c4d3e5ab2375fd8e2e461d541'

      - name: Configure CMake
        run: |
          cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=${{ github.workspace }}/vcpkg/scripts/buildsystems/vcpkg.cmake

      - name: Build
        run: cmake --build build --config Release

      - name: Run tests
        run: ctest -C Release --test-dir build

      - name: Upload artifacts
        uses: actions/upload-artifact@v3
        with:
          name: display-controller
          path: |
            build/bin/Release/*.exe
            build/bin/Release/*.dll
            build/bin/Release/plugins/*.dll

  release:
    needs: build
    if: startsWith(github.ref, 'refs/tags/v')
    runs-on: windows-latest
    steps:
      - name: Download artifacts
        uses: actions/download-artifact@v3
        with:
          name: display-controller
          path: display-controller

      - name: Create ZIP archive
        run: |
          Compress-Archive -Path display-controller/* -DestinationPath DisplayController-${{ github.ref_name }}.zip

      - name: Create Release
        uses: softprops/action-gh-release@v1
        with:
          files: DisplayController-${{ github.ref_name }}.zip
          draft: false
          prerelease: false
          generate_release_notes: true
```

## 実装手順

1. `.github/workflows/`ディレクトリを作成
2. `ci.yml`ファイルを作成し、上記の設定を実装
3. リポジトリにプッシュして動作確認
4. 必要に応じて設定を調整

## リリース手順

1. コードの変更をmainブランチにマージ
2. バージョン番号を更新（必要に応じて）
3. `git tag v1.0.0`のようにバージョンタグを作成
4. `git push origin v1.0.0`でタグをプッシュ
5. GitHub Actionsが自動的にビルドとリリース作成を実行

## 注意点

- vcpkgのコミットIDは`vcpkg.json`のbaseline値と一致させる必要があります
- リリース作成には適切なGitHubトークンが必要です（通常はGITHUB_TOKENが自動的に提供されます）
- ビルドパスやアーティファクトパスはプロジェクト構造に合わせて調整が必要な場合があります
