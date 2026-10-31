# Upstream provenance

[日本語版はこちら](UPSTREAM.ja.md)

`ssl_proxy` is an independent source snapshot derived from [nookingtons/network_mitm](https://github.com/nookingtons/network_mitm) at commit [`7dd6d036feb7bd4bdaca364dc589752bde9e36cf`](https://github.com/nookingtons/network_mitm/commit/7dd6d036feb7bd4bdaca364dc589752bde9e36cf). The copied source also included local, uncommitted NMTS v1 stream work from the original checkout; the base commit alone is not a complete description of this snapshot. The source copy was recorded in `docs/superpowers/plans/2026-10-31-source-snapshot.json`.

The original upstream Git history remains in its repository. This project's Git history starts from a separate snapshot and does **not** include the upstream commits as ancestors. Build dependencies are recorded as pinned Git submodules; their source histories are not copied into this repository's history.

The snapshot removes certificate replacement, SSL verification changes, and SD PCAP capture. It uses opt-in TCP streaming while retaining the original SSL service's results and behavior. The original source copyright notices, including Mary Guillemard's notices, and the GPLv2 [LICENSE](LICENSE) are retained.
