#!/usr/bin/env bash
set -e

DIR=$(dirname "$0")
gcc -std=gnu99 -I"$DIR/../include" "$DIR/../kernel/mem.c" "$DIR/memory_test.c" -o "$DIR/memory_test"
"$DIR/memory_test"
