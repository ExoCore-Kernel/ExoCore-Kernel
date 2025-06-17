#!/usr/bin/env bash
set -e

BG_BLACK="\033[40m"
FG_WHITE="\033[97m"
FG_GREEN="\033[32m"
FG_RED="\033[31m"
RESET="\033[0m"

DIR=$(dirname "$0")
echo -e "${BG_BLACK}${FG_WHITE}Building memory test...${RESET}"
gcc -std=gnu99 -I"$DIR/../include" "$DIR/../kernel/mem.c" "$DIR/memory_test.c" -o "$DIR/memory_test"
if "$DIR/memory_test"; then
  echo -e "${BG_BLACK}${FG_GREEN}Memory test passed${RESET}"
else
  echo -e "${BG_BLACK}${FG_RED}Memory test failed${RESET}"
fi
