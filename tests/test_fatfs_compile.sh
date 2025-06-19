#!/usr/bin/env bash
set -e
BG_BLACK="\033[40m"
FG_WHITE="\033[97m"
FG_GREEN="\033[32m"
RESET="\033[0m"

DIR=$(dirname "$0")

# Compile FAT filesystem driver to ensure it builds

gcc -std=gnu99 -I"$DIR/../include" -c "$DIR/../kernel/fatfs.c" -o "$DIR/fatfs.o"

echo -e "${BG_BLACK}${FG_GREEN}FAT filesystem driver compiled successfully${RESET}"
