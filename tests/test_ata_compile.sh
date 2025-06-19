#!/usr/bin/env bash
set -e
BG_BLACK="\033[40m"
FG_WHITE="\033[97m"
FG_GREEN="\033[32m"
FG_RED="\033[31m"
RESET="\033[0m"

DIR=$(dirname "$0")

# Compile ATA driver to ensure it builds on host
gcc -std=gnu99 -I"$DIR/../include" -c "$DIR/../linkdep/ata_pio.c" -o "$DIR/ata_pio.o"
gcc -std=gnu99 -I"$DIR/../include" -c "$DIR/../linkdep/io.c" -o "$DIR/io.o"

echo -e "${BG_BLACK}${FG_GREEN}ATA driver compiled successfully${RESET}"
