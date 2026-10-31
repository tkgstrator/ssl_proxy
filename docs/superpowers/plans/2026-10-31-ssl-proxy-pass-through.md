# ssl_proxy Pass-Through Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Publish a separate, trust-preserving Atmosphère SSL stream proxy that forwards `ssl` and `ssl:s` unchanged and asynchronously exports only successful Read/Write plaintext to Mac.

**Architecture:** Copy the dirty source snapshot into a new independent repository while leaving the upstream clone and its history intact. Keep the NMTS v1 queue/sender and existing InkScope receiver; remove CA/verification mutation and synchronous SD capture. Use a new opt-in `[ssl_proxy]` configuration and a guarded Git snapshot commit.

**Tech Stack:** Atmosphère libstratosphere GNU C++23, libnx BSD, Make, Docker/devkitA64, host C++ tests, GitHub guarded Git.

**Spec:** `docs/superpowers/specs/2026-10-31-ssl-proxy-pass-through-design.md`

## Global Constraints

- Do not edit or reset the original `network_mitm` checkout, its uncommitted work, or upstream history.
- Do not operate the Switch, replace a sysmodule, restart a game, or stop any user-owned capture.
- Do not create a game ExeFS/IPS patch; the generated `exefs.nsp` is only the sysmodule artifact.
- Keep NMTS v1 wire compatibility with existing InkScope and NXCTF receivers.
- Do not use certificate replacement, verification overrides, or synchronous SD capture.
- New public Git history must use only `gh-commit-identity` guarded init/stage/commit/push; stop on any guard refusal.

## Review Focus

- Successful partial Write: copy only the original service's accepted byte count, preserving its returned count and Result.
- Failed or zero-byte I/O: no data event; no output buffer read after a failed call.
- Certificate and verification requests: forward exact values/buffers and original Result without patching, force flags, or faked getters.
- `ssl:s`: default route actually creates proxied contexts/connections rather than only registering the service name.
- Failed BSD init, disconnected Mac, and full queue: SSL requests still return original results promptly; losses are visible where possible.

---

### Task 1: Preserve and copy the source snapshot

**Files:** source checkout read-only; new `ssl_proxy` tree.

**Interfaces:** Existing base `7dd6d036feb7bd4bdaca364dc589752bde9e36cf`; copy tracked dirty and untracked stream/tests, GPLv2 LICENSE, and build files. Exclude `.git`, `.gitmodules`, `Atmosphere-libs` gitlink, and generated `out` trees.

- [ ] Record source HEAD, status, and SHA-256 manifest for copied files; compare destination copy against it and recheck source hashes after copying so concurrent source changes cannot enter unnoticed.
- [ ] Copy source without changing original, keeping source copyright and license.
- [ ] Add upstream provenance document with exact URL/base commit and explain that new snapshot history does not contain upstream ancestry.

### Task 2: Remove trust and verification changes

**Files:** `network_mitm/source/networkmitm_main.cpp`, `networkmitm_ssl_connection_impl.{cpp,hpp}`, `networkmitm_ssl_service_impl.cpp`, `networkmitm_ssl_for_system_service_impl.cpp`, `networkmitm_utils.{cpp,hpp}`, `network_mitm/Makefile`, README.

**Interfaces:** Certificate and verify APIs return the original shim's Result and output values. `Read` and `Write` call the shim once, then conditionally enqueue post-success data; no other method enqueues.

- [ ] Remove `custom_ca_public_cert`, CA storage/conversion/patch functions and calls. Let certificate commands use Atmosphère's raw `ResultShouldForwardToSession` path, including returned buffer/size.
- [ ] Remove `should_disable_ssl_verification`, constructor override, cached/faked verify settings. Let verification and other non-capture commands raw-forward to the corresponding original domain object instead of manually reserializing them; keep only necessary CreateContext/CreateConnection and Read/Write handlers mapped.
- [ ] Remove unused mbedTLS linkage and utility source if no longer needed; audit source for residual trust-changing symbols.
- [ ] Add a host-testable post-success capture boundary for returned count validation, including zero, over-buffer count, and partial Write prefix. Audit source for one original shim call, exact result/count return, and no queue access before success; inspect actual source diff.

### Task 3: Make capture Mac-only and opt-in

**Files:** `network_mitm/source/impl/stream/stream_config.{cpp,hpp}`, `networkmitm_main.cpp`, SSL context/service connection files and headers, SD PCAP files, `tests/stream_test.cpp`, `tests/sender_test.cpp`.

**Interfaces:** Parse only `[ssl_proxy] enabled/export_host/export_port`; valid opt-in starts sender and registers both `ssl` and `ssl:s` MITM. All proxied non-Read/Write calls forward unchanged. NMTS v1 data is captured only after successful Read/Write using the original count within buffer bounds.

- [ ] Write failing host tests for missing/invalid/valid destination and queue loss/reconnect; verify red.
- [ ] Remove `sd|both|off` legacy fallback, SD writer construction and inline writes, SD mount/logger path. Keep bounded queue copy on SSL IPC threads and socket operations on worker only.
- [ ] Register `ssl:s` with `SslServiceForSystemImpl` (the current `RegisterMitmServer<SslServiceImpl>` selects the wrong `ShouldMitm` callback); make both services select intended clients when enabled and test the registration template and all-client scope.
- [ ] Run host tests and static source audit; verify no old setting can activate certificate or verification behavior.

### Task 4: Make the new snapshot buildable without a Git submodule

**Files:** `scripts/fetch-dependencies.sh`, `.github/workflows/build.yml`, `README.md`, `Makefile`, `.gitignore`, `UPSTREAM.md`.

**Interfaces:** Dependency script fetches Atmosphere-libs at `cf48d23375cf36742876f4b014971b2ebabc6dc8` and libnx at `b156b4774fb2df7b93402169509266da7547fc4c` for the known toolchain; no dependency checkout is committed. Build still packages the sysmodule at `atmosphere/contents/4200000000000666/exefs.nsp` with `mitm.lst` for `ssl` and `ssl:s`.

- [ ] Update CI and local build instructions to fetch pinned dependencies without `.gitmodules` or gitlinks.
- [ ] Pin the CI/Docker devkitA64 image digest to the verified image. State that external package availability still limits full reproducibility.
- [ ] Explain NMTS receiver command, loss/readiness limits, plaintext sensitivity, and sysmodule versus game patch distinction.
- [ ] Document that title ID `4200000000000666` matches the old `network_mitm`; these SSL MITM sysmodules cannot coexist, and no device replacement happens in this task.
- [ ] Run host tests, target Docker build, package inspection, SHA-256 check, and synthetic Python-to-InkScope compatibility check. Keep device runtime unverified.

### Task 5: Independent review and guarded publication

**Files:** all new-repo source/config/docs/build files.

- [ ] Request independent actual-diff source review with severity/file references; resolve material findings and re-review affected fixes.
- [ ] Check no secrets, game patch artifacts, `.gitmodules`, gitlinks, nested repositories, build outputs, or altered upstream clone files enter the staged tree.
- [ ] Use guarded Git `init master https://github.com/tkgstrator/ssl_proxy.git`, fresh identity, explicit stage paths, short English Conventional Commit, raw commit verification, and normal guarded push. Stop and report if any check refuses.
- [ ] Verify remote OID/tree against the reviewed local commit. Do not stage, reboot, or activate the module on Switch.
