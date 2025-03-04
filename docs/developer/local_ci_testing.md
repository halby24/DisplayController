# GitHub Actionsのローカルテスト手順

このドキュメントでは、GitHub Actionsワークフローをローカル環境でテストする方法について説明します。

## 前提条件

- Docker Desktop for Windows がインストールされていること
- actツールがインストールされていること

## actのインストール

### Windows環境での方法

1. **Chocolateyを使用する場合**:
   ```powershell
   choco install act-cli
   ```

2. **Scoopを使用する場合**:
   ```powershell
   scoop install act
   ```

3. **手動インストール**:
   - [GitHub Releases](https://github.com/nektos/act/releases)から最新のWindows用バイナリをダウンロード
   - ダウンロードしたファイルを解凍し、PATHの通ったディレクトリに配置

## テスト手順

### 基本的なテスト

1. プロジェクトのルートディレクトリに移動します：
   ```powershell
   cd e:\Repos\node\DisplayController
   ```

2. すべてのワークフローを実行：
   ```powershell
   act
   ```

### 特定のイベントのテスト

1. pushイベントのテスト（ビルドジョブが実行されます）：
   ```powershell
   act push
   ```

2. タグプッシュイベントのテスト（リリースジョブが実行されます）：
   ```powershell
   # JSONファイルを作成
   echo '{"ref": "refs/tags/v1.0.0"}' > event.json

   # イベントファイルを使用してactを実行
   act --eventpath event.json
   ```

### 特定のジョブのテスト

1. ビルドジョブのみをテスト：
   ```powershell
   act -j build
   ```

2. リリースジョブのみをテスト：
   ```powershell
   # 環境変数を設定してタグプッシュをシミュレート
   $env:GITHUB_REF = "refs/tags/v1.0.0"
   act -j release
   ```

## 診断とトラブルシューティング

### ログの詳細レベルを上げる

詳細なログを表示するには、`-v`フラグを使用します：

```powershell
act -v
```

さらに詳細なログを表示するには、`-v`を複数回使用します：

```powershell
act -vvv
```

### 一般的な問題と解決策

1. **Docker関連のエラー**:
   - Docker Desktopが実行されていることを確認
   - WSL2が有効になっていることを確認
   - Dockerリソース（メモリ、CPU）が十分に割り当てられていることを確認

2. **Windows固有の問題**:
   - パスの区切り文字（`\` vs `/`）に関する問題が発生する場合は、WSL2を使用してactを実行
   - PowerShellでの環境変数の設定方法に注意（`$env:VARIABLE_NAME = "value"`）

3. **actの実行環境の問題**:
   - デフォルトのマイクロイメージではなく、フルイメージを使用：
     ```powershell
     act --container-architecture linux/amd64 -P ubuntu-latest=catthehacker/ubuntu:act-latest
     ```

4. **vcpkg関連の問題**:
   - vcpkgのセットアップに時間がかかる場合は、キャッシュを使用するか、ローカルのvcpkgインスタンスをマウント

### CI設定の検証

actはGitHub Actionsの完全な再現ではないため、以下の点に注意してください：

1. 一部のアクションはGitHub環境に依存しており、ローカルでは動作しない場合があります
2. リソース制約（メモリ、CPU）により、ビルドパフォーマンスが異なる場合があります
3. Secretsの扱いが異なります（`.secrets`ファイルまたは環境変数を使用）

## 特定の診断シナリオ

### ビルドエラーの診断

1. ビルドジョブを詳細ログで実行：
   ```powershell
   act -j build -vvv
   ```

2. 出力を確認し、エラーメッセージを特定

3. CMakeの設定に問題がある場合：
   ```powershell
   # CMake設定のみを実行
   act -j build --step "Configure CMake" -vvv
   ```

### リリースジョブの診断

1. アーティファクトのアップロードに問題がある場合：
   ```powershell
   # アップロードステップのみを実行
   act -j build --step "Upload artifacts" -vvv
   ```

2. リリース作成に問題がある場合：
   ```powershell
   # 環境変数を設定
   $env:GITHUB_REF = "refs/tags/v1.0.0"

   # リリースジョブを詳細ログで実行
   act -j release -vvv
   ```

## 参考リンク

- [act GitHub リポジトリ](https://github.com/nektos/act)
- [act ドキュメント](https://github.com/nektos/act#readme)
- [GitHub Actions ドキュメント](https://docs.github.com/en/actions)
