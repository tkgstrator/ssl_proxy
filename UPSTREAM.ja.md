# アップストリームの出典情報

[English](UPSTREAM.md)

`ssl_proxy` は、コミット [`7dd6d036feb7bd4bdaca364dc589752bde9e36cf`](https://github.com/nookingtons/network_mitm/commit/7dd6d036feb7bd4bdaca364dc589752bde9e36cf) における [nookingtons/network_mitm](https://github.com/nookingtons/network_mitm) から派生した独立したソーススナップショットです。コピーされたソースには、元の作業環境で行われていたローカルかつ未コミットの NMTS v1 ストリーミングの実装も含まれており、ベースコミットのみでは本スナップショットの完全な記述とはなりません。ソースの複製記録は `docs/superpowers/plans/2026-10-31-source-snapshot.json` に記載されています。

オリジナルのアップストリーム Git 履歴は元リポジトリに保持されています。本プロジェクトの Git 履歴は独立したスナップショットから開始されており、アップストリームのコミットを祖先としては**含みません**。ビルド依存関係は特定コミットに固定された Git サブモジュールとして管理されており、それらのソース履歴も本リポジトリの履歴にはコピーされていません。

本スナップショットでは、証明書の置き換え、SSL 検証設定の変更、および SD カードへの PCAP キャプチャ機能を削除しています。元の SSL サービスの処理結果および挙動を完全に維持したまま、オプトイン方式の TCP ストリーミング機能を提供します。Mary Guillemard 氏の著作権表示を含む元のソース著作権表示、および GPLv2 [LICENSE](LICENSE) はすべて保持されています。
