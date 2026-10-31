# ssl_proxy

`ssl_proxy` is an Atmosphère sysmodule for an owned Nintendo Switch. When explicitly enabled, it proxies the Horizon `ssl` and `ssl:s` services, forwards SSL operations to the original service, and sends plaintext from successful `Read` and `Write` calls to a Mac receiver over an NMTS v1 TCP stream. The intended scope is **all clients** of both services, including system titles. It does not capture all Switch network traffic, Pia traffic, or Ethernet frames.

The sysmodule does not replace certificates or change SSL verification settings. Capture is best effort: a disconnected receiver or full queue can lose events without changing the SSL result returned to the client. The stream is plaintext and has no encryption or cryptographic peer authentication. It may contain credentials and tokens from games and system clients. Use it only on a trusted, closed LAN, restrict the receiver to the current Switch IPv4 address, and protect receiver output.

## Configuration

Add this to `sd:/atmosphere/config/system_settings.ini`, using the **current** IPv4 address of the Mac receiver and its listening port:

```ini
[ssl_proxy]
enabled = u8!0x1
export_host = str!192.168.1.119
export_port = u32!0x99E8
```

`0x99E8` is port 39400. The MITM and stream are opt in: a missing or disabled `enabled` value or an invalid destination leaves the proxy inactive. Check both devices' current IPv4 addresses before use; a prior session's addresses can be stale. The old `[network_mitm]` settings are not used by this module.

### Mac receiver

The NMTS v1 receiver currently exists only in the prepared InkScope receiver worktree; it is not available from the public InkScope `master` branch yet. From that worktree, start the receiver on the Mac before enabling the sysmodule, substituting verified local addresses and a new output path:

```sh
cd /path/to/InkScope-receiver-worktree
cargo run -p inkscope --locked -- receive-mitm \
  --bind-ip 192.168.1.119 --port 39400 \
  --allowed-peer 192.168.1.159 \
  --output-dir "$HOME/captures/mitm-$(date +%Y%m%d-%H%M%S)"
```

The receiver creates private `ssl.pcap` and `ssl-events.jsonl` files. The PCAP uses USER0 records for SSL plaintext; the JSONL retains direction, identifiers, receipt time, and loss diagnostics. A `hello` frame proves only that the stream connected. A `data` frame proves that the hook observed an SSL call; neither proves completeness or that a particular game operation occurred. `loss` frames report cumulative queue drops where the sender can report them. A connection failure, sender startup failure, or process interruption may prevent complete loss reporting. Switch ticks are monotonic values, not UTC timestamps.

NMTS v1 frames use a four-byte big-endian length, a 52-byte header with `NMTS` magic and version, and at most 8192 data bytes per frame. Frame types are `hello`, `data`, and `loss`; directions distinguish client-to-server `Write` from server-to-client `Read`. The sender uses a bounded queue and reconnects in its own thread. This stream is separate from InkScope's ordinary Ethernet packet capture and does not require a GameKey or PiaKey.

The `network_mitm/network_mitm.json` NPDM manifest still has the upstream broad filesystem and service permissions. Its permissions have not yet been narrowed through controlled runtime testing, although this code has no SD capture or certificate path.

## Build

The build fetches source archives at fixed commits, without Git submodules or nested Git repositories:

| Dependency | Commit |
| --- | --- |
| [Atmosphere-libs](https://github.com/Atmosphere-NX/Atmosphere-libs) | `cf48d23375cf36742876f4b014971b2ebabc6dc8` |
| [libnx](https://github.com/switchbrew/libnx) | `b156b4774fb2df7b93402169509266da7547fc4c` |

`./scripts/fetch-dependencies.sh` places Atmosphere-libs at `./Atmosphere-libs` and libnx at `./.deps/libnx`. Both paths are ignored by Git. Existing directories with an unknown revision cause the script to stop. The build image is pinned to `devkitpro/devkita64@sha256:1fc388c3a0d34bd2045a6dadcb1020e069d5f876a187fd705de14b4440c00282`. Source revisions and the image are pinned; external package availability can still change, so this is not a fully reproducible build.

### Docker

With Docker Compose installed, run:

```sh
docker compose run --rm devkit
```

The pinned image already contains the required devkitPro tools and `switch-libjpeg-turbo`. The container fetches the pinned sources, builds and installs pinned libnx inside the container, then runs `make`. The first run requires network access.

### Local devkitPro

Install devkitPro with `switch-dev` and `switch-libjpeg-turbo`, and provide the `DEVKITPRO` and `DEVKITA64` environment variables. Then run:

```sh
./scripts/fetch-dependencies.sh
make -C .deps/libnx PREFIX=aarch64-none-elf-
make -C .deps/libnx install
make
```

The libnx install command updates the local devkitPro installation. `make` fetches missing source archives but expects the pinned libnx to have been installed in that toolchain.

The release ZIP contains `atmosphere/contents/4200000000000666/exefs.nsp`, `flags/boot2.flag`, and a `mitm.lst` naming both `ssl` and `ssl:s`. Here `exefs.nsp` is the **sysmodule** executable, not a game ExeFS/IPS patch. Title ID `4200000000000666` is the same as the old `network_mitm` sysmodule, so they cannot coexist; this artifact replaces the old module at that path. Building it does not place it on a Switch or prove runtime operation.

## Origin and license

This is an independent source snapshot based on [nookingtons/network_mitm](https://github.com/nookingtons/network_mitm) at commit [`7dd6d036feb7bd4bdaca364dc589752bde9e36cf`](https://github.com/nookingtons/network_mitm/commit/7dd6d036feb7bd4bdaca364dc589752bde9e36cf), with later local stream work and the changes described in [UPSTREAM.md](UPSTREAM.md). Its Git history does not contain upstream ancestry. Original copyright notices remain in the source. The project remains licensed under GPLv2; see [LICENSE](LICENSE).
