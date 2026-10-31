#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
test_binary="$(mktemp "${TMPDIR:-/tmp}/network-mitm-sender-test.XXXXXX")"
trap 'rm -f "$test_binary"' EXIT HUP INT TERM
"${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror -pthread \
    tests/sender_test.cpp \
    network_mitm/source/impl/stream/stream_sender.cpp \
    network_mitm/source/impl/stream/stream_protocol.cpp \
    network_mitm/source/impl/stream/stream_queue.cpp \
    -o "$test_binary"
"$test_binary"
