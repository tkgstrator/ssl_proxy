# ssl_proxy

[日本語版はこちら](README.ja.md)

`ssl_proxy` is an Atmosphère sysmodule for an owned Nintendo Switch. When explicitly enabled, it proxies Horizon's `ssl` and `ssl:s` services, forwards SSL operations transparently to the original system service, and streams plaintext data from successful `Read` and `Write` calls to a designated receiver over an NMTS v1 TCP stream. The intended scope encompasses **all clients** of both services, including system titles. It does not capture general Switch network traffic, Pia (peer-to-peer) traffic, or raw Ethernet frames.

The sysmodule does not replace certificates or modify SSL verification settings. Capture operates on a best-effort basis: if the receiver disconnects or the export queue becomes full, events may be dropped without altering the SSL result returned to the application client. The TCP stream is transmitted as unencrypted plaintext without cryptographic peer authentication and may contain sensitive credentials or tokens. Use it only on a trusted, closed LAN, restrict receiver connections to the Switch's IP address, and safeguard captured output.

## Configuration

Add the following block to `sd:/atmosphere/config/system_settings.ini`, configuring the **current** IPv4 address and port of your receiver host (e.g. replacing `<RECEIVER_IP>` with `192.168.1.50`):

```ini
[ssl_proxy]
enabled = u8!0x1
export_host = str!<RECEIVER_IP>
export_port = u32!0x99E8
```

`0x99E8` corresponds to port 39400. Proxying and streaming are strictly opt-in: if `enabled` is missing, disabled (`0x0`), or points to an unreachable destination, the proxy remains inactive. Always verify both devices' active IPv4 addresses before testing, as DHCP leases may change across sessions. Legacy `[network_mitm]` configuration keys are not recognized.

### Stream receiver

The stream uses the NMTS v1 protocol over TCP. Any receiver implementing this protocol can collect and process captured payloads. InkScope is not required, though an implementation of the NMTS v1 receiver is available in [InkScope](https://github.com/tkgstrator/InkScope) via the `receive-mitm` command.

Example of launching an NMTS v1 receiver with InkScope:

```sh
cargo run -p inkscope --locked -- receive-mitm \
  --bind-ip <RECEIVER_IP> --port 39400 \
  --allowed-peer <SWITCH_IP> \
  --output-dir "./captures/mitm-$(date +%Y%m%d-%H%M%S)"
```

A compliant receiver typically generates:
- `ssl.pcap`: Captures SSL plaintext frames (e.g. using LINKTYPE_USER0 / DLT_USER0 records).
- `ssl-events.jsonl`: Structured event metadata detailing stream direction, context/connection identifiers, receipt timestamps, and loss diagnostics.

A `hello` frame confirms only that the TCP stream has been established. A `data` frame indicates that the hook intercepted an active SSL operation; neither confirms complete capture of an entire transaction or guarantees that a specific in-game action occurred. `loss` frames track cumulative buffer drops when the sysmodule queue overflows. System ticks are monotonic timer counts rather than absolute UTC timestamps.

### NMTS v1 protocol overview

NMTS v1 frames consist of:
- A 4-byte big-endian payload length.
- A 52-byte header with the `NMTS` magic, version indicator, monotonic tick, session IDs, and frame type.
- Up to 8,192 data bytes per frame.

Frame types include `hello`, `data`, and `loss`. Direction flags differentiate client-to-server (`Write`) from server-to-client (`Read`) transmissions. The sysmodule uses a bounded in-memory queue and handles reconnection asynchronously on a dedicated worker thread. This stream is independent of standard Ethernet capture tools and requires neither GameKey nor PiaKey.

The `ssl_proxy/ssl_proxy.json` NPDM manifest currently retains upstream service and filesystem permissions; these permissions have not yet been narrowed through controlled runtime testing.

## Build

The build references Git submodules pinned to explicit commits:

| Dependency | Commit |
| --- | --- |
| [Atmosphere-libs](https://github.com/Atmosphere-NX/Atmosphere-libs) | `cf48d23375cf36742876f4b014971b2ebabc6dc8` |
| [libnx](https://github.com/switchbrew/libnx) | `b156b4774fb2df7b93402169509266da7547fc4c` |

Clone the repository recursively with `git clone --recurse-submodules`, or run `git submodule update --init --recursive` in an existing working copy. The build environment uses the pinned Docker image `devkitpro/devkita64@sha256:1fc388c3a0d34bd2045a6dadcb1020e069d5f876a187fd705de14b4440c00282`.

### Docker (Recommended)

With Docker Compose available, run:

```sh
docker compose run --rm devkit
```

The pinned container image includes necessary devkitPro toolchains and `switch-libjpeg-turbo`. The build script initializes submodules, compiles and installs the pinned libnx inside the container, and produces the sysmodule package.

### Local devkitPro

Install devkitPro with `switch-dev` and `switch-libjpeg-turbo`, then ensure `DEVKITPRO` and `DEVKITA64` environment variables are properly exported:

```sh
git submodule update --init --recursive
make -C .deps/libnx PREFIX=aarch64-none-elf-
make -C .deps/libnx install
make
```

The release ZIP archive contains `atmosphere/contents/4200000000000666/exefs.nsp`, `flags/boot2.flag`, and a `mitm.lst` targeting both `ssl` and `ssl:s`. Here `exefs.nsp` is the sysmodule binary itself, not an application patch. Title ID `4200000000000666` matches the legacy `network_mitm` module and will replace it if installed at the same path.

## CI and deployment

[`integration.yaml`](.github/workflows/integration.yaml) checks commit messages and formatting of changed C/C++ files on pull requests and pushes to `master`. It does not build the Switch package.

[`deployment.yaml`](.github/workflows/deployment.yaml) starts after a successful Integration run for a push to `master`. It runs host tests, builds the exact successful commit with pinned submodules, verifies the ZIP archive and SHA-256 checksum, and publishes a GitHub Release. The version tag comes from `ssl_proxy/include/ssl_proxy_version.h`. A later release requires a version bump; the workflow never replaces an existing tag or published release.

## Origin and license

This repository is an independent snapshot based on [nookingtons/network_mitm](https://github.com/nookingtons/network_mitm) at commit [`7dd6d036feb7bd4bdaca364dc589752bde9e36cf`](https://github.com/nookingtons/network_mitm/commit/7dd6d036feb7bd4bdaca364dc589752bde9e36cf), incorporating subsequent local streaming additions detailed in [UPSTREAM.md](UPSTREAM.md). Upstream Git commit history is omitted. Original copyright notices remain preserved in source files. Licensed under GPL-2.0-only; see [LICENSE](LICENSE).
