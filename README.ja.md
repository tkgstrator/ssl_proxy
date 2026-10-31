# ssl_proxy

[English](README.md)

`ssl_proxy` は、Nintendo Switch（所有機）向けの Atmosphère sysmodule です。設定によって明示的に有効化されている場合、Horizon OS の `ssl` および `ssl:s` サービスをプロキシ（中間介入）し、元のシステムサービスへ SSL 操作を透過的に転送しつつ、成功した `Read` および `Write` 呼び出しの平文データを NMTS v1 TCP ストリーム経由で受信側ホストへ送信します。対象スコープは両サービスの**すべてのクライアント**（システムタイトルを含む）です。Switch の全ネットワーク通信、Pia（P2P）通信、または生の Ethernet フレームをキャプチャするものではありません。

本 sysmodule は、証明書の置き換えや SSL 検証設定の変更を行いません。データ取得はベストエフォート方式で動作します。受信側が未接続であったり送信キューが満杯になったりした場合は、クライアントに返される SSL の処理結果に影響を与えることなくイベントが破棄（ドロップ）されることがあります。TCP ストリームは暗号化も認証もない平文（plaintext）で送信されるため、ゲームやシステムクライアントの認証情報やトークンが含まれる可能性があります。信頼できる閉じたローカルネットワーク（LAN）内でのみ使用し、受信側では Switch の IP アドレスからの接続のみを許可するように制限し、出力データの取り扱いには十分注意してください。

## 設定

`sd:/atmosphere/config/system_settings.ini` に以下の設定を追加し、受信側ホストの**現在の** IPv4 アドレスと待受ポート番号を指定します:

```ini
[ssl_proxy]
enabled = u8!0x1
export_host = str!192.168.1.119
export_port = u32!0x99E8
```

`0x99E8` はポート番号 39400 に対応します。プロキシおよびデータ送信はオプトイン方式です。`enabled` の値が存在しない場合、`0x0`（無効）に設定されている場合、または送信先が無効な場合はプロキシは動作しません。DHCP 等によりセッションごとにアドレスが変わることがあるため、使用前に両デバイスの最新の IPv4 アドレスを確認してください。旧来の `[network_mitm]` の設定項目は本モジュールでは使用されません。

### ストリーム受信側（Receiver）

データ送信には TCP 上の NMTS v1 プロトコルが使用されます。このプロトコルを実装した任意の受信機（レシーバー）でキャプチャデータを受信・保存できます。InkScope の使用は必須ではありませんが、NMTS v1 受信機の一例として [InkScope](https://github.com/tkgstrator/InkScope) に `receive-mitm` コマンドが用意されています。

InkScope を使用した受信機の起動例:

```sh
cargo run -p inkscope --locked -- receive-mitm \
  --bind-ip 192.168.1.119 --port 39400 \
  --allowed-peer 192.168.1.159 \
  --output-dir "$HOME/captures/mitm-$(date +%Y%m%d-%H%M%S)"
```

準拠する受信機は通常、以下のファイルを生成します:
- `ssl.pcap`: SSL 平文フレームを記録した PCAP ファイル（例: LINKTYPE_USER0 / DLT_USER0 レコード形式）。
- `ssl-events.jsonl`: 通信方向、コンテキスト/接続識別子、受信時刻、損失診断情報などの構造化イベントログ。

`hello` フレームは TCP ストリームが接続されたことのみを示します。`data` フレームはフック処理がアクティブな SSL 呼び出しを検知したことを示します。これらは完全なトランザクションの取得や特定のゲーム内操作の発生を保証するものではありません。`loss` フレームは、sysmodule の送信キューが溢れた際の累積ドロップ数を記録します。Switch の Tick 値は絶対時刻（UTC）ではなく単調増加タイマーの値です。

### NMTS v1 プロトコル概要

NMTS v1 フレームは以下の構造を持ちます:
- 4 バイトのビッグエンディアン長。
- `NMTS` マジック、バージョン、単調増加 Tick、セッション識別子、フレーム種別を含む 52 バイトのヘッダー。
- 1 フレームあたり最大 8,192 バイトのデータペイロード。

フレーム種別には `hello`、`data`、`loss` があります。方向フラグによりクライアントからサーバーへの送信（`Write`）とサーバーからクライアントへの受信（`Read`）を区別します。sysmodule は容量制限付きのインメモリキューを使用し、専用のワーカースレッド上で非同期に再接続を処理します。このストリームは標準の Ethernet パケットキャプチャとは独立しており、GameKey や PiaKey は不要です。

`ssl_proxy/ssl_proxy.json` の NPDM マニフェストは、現時点ではアップストリームの広範なサービス権限およびファイルシステム権限をそのまま保持しています。実機での制御されたランタイムテストによる権限の最小化は今後の課題です。

## ビルド

ビルドでは、特定コミットに固定された Git サブモジュールを使用します:

| 依存関係 | コミット |
| --- | --- |
| [Atmosphere-libs](https://github.com/Atmosphere-NX/Atmosphere-libs) | `cf48d23375cf36742876f4b014971b2ebabc6dc8` |
| [libnx](https://github.com/switchbrew/libnx) | `b156b4774fb2df7b93402169509266da7547fc4c` |

リポジトリを再帰的にクローンする場合は `git clone --recurse-submodules` を実行するか、既存のクローン内で `git submodule update --init --recursive` を実行してください。ビルド環境には固定された Docker イメージ `devkitpro/devkita64@sha256:1fc388c3a0d34bd2045a6dadcb1020e069d5f876a187fd705de14b4440c00282` を使用します。

### Docker（推奨）

Docker Compose が利用可能な場合、以下を実行します:

```sh
docker compose run --rm devkit
```

固定されたコンテナイメージには必要な devkitPro ツール一式および `switch-libjpeg-turbo` が含まれています。コンテナ内でサブモジュールの初期化、固定 libnx のビルドおよびインストール、sysmodule パッケージの作成が自動的に実行されます。

### ローカル devkitPro

`switch-dev` および `switch-libjpeg-turbo` を含む devkitPro をインストールし、環境変数 `DEVKITPRO` および `DEVKITA64` を設定した上で以下を実行します:

```sh
git submodule update --init --recursive
make -C .deps/libnx PREFIX=aarch64-none-elf-
make -C .deps/libnx install
make
```

リリース ZIP アーカイブには `atmosphere/contents/4200000000000666/exefs.nsp`、`flags/boot2.flag`、および `ssl` と `ssl:s` を指定した `mitm.lst` が含まれます。ここで `exefs.nsp` はゲームパッチではなく sysmodule 本体の実行可能バイナリです。Title ID `4200000000000666` は旧来の `network_mitm` モジュールと同一であり、同一パスに配置した場合は上書きされます。

## CIとデプロイ

[`integration.yaml`](.github/workflows/integration.yaml) は、プルリクエストおよび `master` へのプッシュ時にホスト単体テストを実行し、Switch パッケージをビルドして生成された ZIP アーカイブの内容を検証します。

[`deployment.yaml`](.github/workflows/deployment.yaml) は、既存のバージョンダグを対象に手動（workflow_dispatch）で実行されます。対象タグをクリーンビルドし、ソースコミットとパッケージのチェックサムを検証した上で、公開済みリリースを上書きすることなく**下書き（Draft）** の GitHub Release を作成または更新します。

## 出典とライセンス

本リポジトリは、コミット [`7dd6d036feb7bd4bdaca364dc589752bde9e36cf`](https://github.com/nookingtons/network_mitm/commit/7dd6d036feb7bd4bdaca364dc589752bde9e36cf) の [nookingtons/network_mitm](https://github.com/nookingtons/network_mitm) をベースとした独立したソーススナップショットであり、[UPSTREAM.md](UPSTREAM.md) に記載されているローカルのストリーミング拡張が追加されています。アップストリームの Git 履歴は含まれていません。ソースファイル内の原著作権表示はすべて維持されています。GPL-2.0-only の下でライセンスされています（[LICENSE](LICENSE) を参照）。
