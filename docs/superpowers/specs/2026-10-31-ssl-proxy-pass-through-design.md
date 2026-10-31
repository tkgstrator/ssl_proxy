# ssl_proxy の透過 SSL 受信設計

## 目的と境界

所有する Switch の Atmosphère SSL サービス MITM で `ssl` と `ssl:s` の呼び出しを元サービスへそのまま転送する。元サービスが成功した `Read` の返却バイトと、成功した `Write` が受理したバイト数に対応する暗号化前データだけを、固定容量キューへコピーして別スレッドから Mac の指定 IPv4/TCP ポートへ送る。ゲーム側に返す Result、バイト数、SSL 設定、証明書、バッファを送信成否によって変えない。

## 採用する構成

- 既存 `network_mitm` の NMTS v1 フレーム、送信キュー、再接続、累積 loss、定期 hello を利用する。InkScope の `receive-mitm` と NXCTF の Python 受信機は同じフレームを受信できる。キュー満杯・接続失敗は欠落として計上し、SSL 呼び出しを待たせない。これはベストエフォートの平文取得であり、全 TCP/UDP や Pia ではない。
- 設定は新しい `[ssl_proxy]` 名前空間の `enabled = u8!0x1`、`export_host = str!<Mac IPv4>`、`export_port = u32!<1..65535>` だけにする。明示的な有効化と有効な宛先がなければ MITM サーバーを登録せず、送信しない。従来の `[network_mitm]` 設定や `export_mode`、SD 保存、`custom_ca_public_cert`、`should_disable_ssl_verification` は読み込まない。既存の古い設定が残っていても新しいモジュールには効かない。
- `ssl` と `ssl:s` は有効時に対象クライアントをともにフックする。`ssl:s` は `SslServiceForSystemImpl` を登録して、その実際の `ShouldMitm` を使う。コマンドは元サービスへ転送し、証明書返却、検証オプション、ハンドシェイクを変更しない。SSL 接続の `Read`/`Write` が成功した後にだけ、元サービスが返した長さを上限確認してキューにコピーする。`Peek`、失敗した I/O、ゼロバイトは data イベントにしない。
- 初版は両サービスの全クライアントを対象にするため、ゲーム以外のシステム通信や認証情報が送られる可能性がある。README でこの範囲を明示し、実機の通信経路・タイミング影響は未検証とする。
- SD PCAP と SD ロガーを実行経路から除き、SSL IPC スレッドでファイル I/O、ソケット I/O、待機を行わない。送信側は別スレッドで接続、hello、loss、data を処理する。ゲーム ExeFS/IPS パッチは作成も配置もしない。ビルド生成物の `exefs.nsp` は専用 Atmosphère **sysmodule** の実行ファイルであり、ゲームの ExeFS パッチではないことを明記する。
- sysmodule title ID `4200000000000666` は元 `network_mitm` と同じなので、両者を併用しない。新しい成果物は旧モジュールを置き換える対象として文書化するが、今回の作業では Switch 上の配置・削除・再起動を行わない。

## リポジトリと出典

- 元 `network_mitm` クローンの上流 `7dd6d036feb7bd4bdaca364dc589752bde9e36cf`、作業ツリー、サブモジュールを変更しない。変更済みファイルと追加ファイルを検証付きで別の `ssl_proxy` ディレクトリへコピーし、そこだけ修正する。GPLv2 の LICENSE と元ソースの著作権を保持し、README に上流 URL・基点コミット・変更点を記す。
- 新しい公開 `tkgstrator/ssl_proxy` は空である。guarded Git は元リポジトリの gitlink/`.gitmodules` と上流 43 コミットの著者・メッセージを拒否するため、上流の Git 履歴を新リポジトリの祖先としてプッシュできない。上流履歴は元 clone と上流リポジトリに残し、新リポジトリは出典を明記した独立スナップショット履歴にする。`.gitmodules` と gitlink を含めず、Atmosphere-libs は固定コミット `cf48d23375cf36742876f4b014971b2ebabc6dc8` をビルド時に別途取得する。CI/README をその方法に合わせる。
- 新リポジトリの stage/commit/push は `gh-commit-identity` の guarded Git だけで行い、通常の Git コミットや push で制約を回避しない。コミット前に差分、機密情報、ライセンス、テスト、ビルド生成物の除外を確認する。公開先はユーザー指定の `https://github.com/tkgstrator/ssl_proxy.git` のみ。
- Docker ビルド環境は今回検証済みの devkitA64 image digest `sha256:1fc388c3a0d34bd2045a6dadcb1020e069d5f876a187fd705de14b4440c00282` に固定する。外部パッケージや将来の OS の完全な再現性までは主張しない。

## 検証と運用

- ホストテストで設定無効・宛先不正、NMTS フレーム、順序、loss、再接続、キュー満杯を確認する。元サービスへの透過転送については SSL/証明書/検証設定の呼び出し経路を差分監査し、Read/Write の成功・部分 Write・失敗・ゼロバイトを対象に検証する。Switch 向けビルド、パッケージ内容、ハッシュ、InkScope との合成フレーム互換を確認する。
- 実機への配置、再起動、実際の SSL データ確認は、ユーザーのゲームとキャプチャを止めずに行える時点まで保留する。配置ファイルの読み戻しは実行中の有効化や元サービスとの同一動作を証明しない。
