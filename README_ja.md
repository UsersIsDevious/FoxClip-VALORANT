# FoxClip-VALORANT

VALORANT/Riot Client のローカル API にアクセスするために、lockfile を読み取り、必要に応じて WebSocket/REST に接続する C++ ユーティリティです。

## 特徴

- `config.json` による設定
- パスの環境変数展開（例: `%LOCALAPPDATA%`, `%APPDATA%`）
- デバッグモード切り替え
- Windows/Linux のロックファイルパスをサポート
- 既定値で自動的に設定ファイルを作成
- 任意の WebSocket 接続（Boost.Beast + OpenSSL）
- `OnJsonApiEvent` の購読と sessionLoopState の追跡
- バックオフ付き自動再接続とサイレンス監視
- nlohmann::json による堅牢な JSON 処理
- ローテーション風のファイル出力ログ（INFO/WARN/ERROR）

## 設定

初回起動時に `config.json` を自動作成します:

```json
{
  "debug": true,
  "enable_websocket": false,
  "lockfile_paths": [
    "%LOCALAPPDATA%\\Riot Games\\Riot Client\\Config\\lockfile",
    "%APPDATA%\\Riot Client\\Config\\lockfile",
    "C:\\Riot Games\\Riot Client\\Config\\lockfile",
    "/var/lib/riot/lockfile",
    "./lockfile"
  ]
}
```

メモ:
- パスは Windows 形式の環境変数を展開します。
- 未知/欠落のキーはデフォルトにフォールバックします。
- ビルドによっては `"websocket_auto_start"` を追加して使用できます。

## ビルド

必須:
- CMake 3.10+
- C++17 コンパイラ

任意（WebSocket 用）:
- Boost.Beast
- OpenSSL
- nlohmann::json（ヘッダーのみ）

WebSocket なし（既定）:

```bash
mkdir build && cd build
cmake ..
make
```

WebSocket あり:

```bash
mkdir build && cd build
cmake .. -DENABLE_WEBSOCKET=ON
make
```

## 使い方

- 通常: 実行すると既知の場所から lockfile を探索して読み取ります。
- WebSocket: `config.json` で `"enable_websocket": true` にすると接続します。

## ログ

作業ディレクトリ配下にタイムスタンプ付きのログファイルを出力します（INFO/WARN/ERROR）。

## ライセンス

MIT License（[LICENSE](LICENSE) を参照）。