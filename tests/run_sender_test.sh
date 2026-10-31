#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
test_binary="$(mktemp "${TMPDIR:-/tmp}/ssl-proxy-sender-test.XXXXXX")"
trap 'rm -f "$test_binary"' EXIT HUP INT TERM
"${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror -pthread \
    tests/sender_test.cpp \
    source/stream_sender.cpp \
    source/stream_protocol.cpp \
    source/stream_queue.cpp \
    -o "$test_binary"
"$test_binary"
