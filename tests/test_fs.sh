#!/usr/bin/env bash
set -e
BG_BLACK="\033[40m"
FG_WHITE="\033[97m"
FG_GREEN="\033[32m"
FG_RED="\033[31m"
RESET="\033[0m"

DIR=$(dirname "$0")

echo -e "${BG_BLACK}${FG_WHITE}Building fs test...${RESET}"

gcc -std=gnu99 -I"$DIR/../include" "$DIR/../kernel/fs.c" "$DIR/../kernel/memutils.c" "$DIR/fs_test.c" -o "$DIR/fs_test"

if "$DIR/fs_test"; then
  echo -e "${BG_BLACK}${FG_GREEN}FS test passed${RESET}"
else
  echo -e "${BG_BLACK}${FG_RED}FS test failed${RESET}"
fi
