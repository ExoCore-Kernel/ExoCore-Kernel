#!/usr/bin/env bash
set -e
BG_BLACK="\033[40m"
FG_WHITE="\033[97m"
FG_GREEN="\033[32m"
FG_RED="\033[31m"
RESET="\033[0m"

DIR=$(dirname "$0")

echo -e "${BG_BLACK}${FG_WHITE}Building FAT fs test...${RESET}"

gcc -std=gnu99 -I"$DIR/../include" "$DIR/mock_ata_mem.c" "$DIR/../kernel/fatfs.c" "$DIR/../kernel/memutils.c" "$DIR/fatfs_test.c" -o "$DIR/fatfs_test"

if "$DIR/fatfs_test"; then
  echo -e "${BG_BLACK}${FG_GREEN}FAT fs test passed${RESET}"
else
  echo -e "${BG_BLACK}${FG_RED}FAT fs test failed${RESET}"
fi
