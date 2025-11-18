#!/usr/bin/env bash

set -euo pipefail

print_header() {
  echo "Welcome to EXOCORE-TEST-BUILD"
  echo "Have you installed all required software to compile the kernel?"
  echo
  echo "1. Yes"
  echo "2. No, Try anyway"
  echo "3. No, install for me"
}

progress_bar() {
  local percent=$1
  local bar_width=10
  local filled=$((percent * bar_width / 100))
  local empty=$((bar_width - filled))
  printf '['
  printf '#%.0s' $(seq 1 "$filled")
  printf '—%.0s' $(seq 1 "$empty")
  printf ']\n'
  printf "%14s%3d%%\n" "" "$percent"
}

simulate_install() {
  echo
  echo "Installing required keychains and software"
  progress_bar 29
  echo "Currently installing: Example Keychain"
  echo "(press ctrl c to cancel)"
  sleep 1
  progress_bar 100
  echo "Finished installing required software!"
}

simulate_compile() {
  echo
  echo "Compiling kernel"
  progress_bar 75
  echo "Compiling logs:"
  if [[ -f build.txt ]]; then
    tail -n 1 build.txt
  else
    echo "(no build logs available)"
  fi
  sleep 1
  progress_bar 100
  echo
  echo "Compiling complete!"
}

prompt_compile() {
  echo "Would you like to Compile the kernel?"
  echo
  echo "1. Yes"
  echo "3. No"
  read -r compile_choice
  if [[ "$compile_choice" == "1" ]]; then
    simulate_compile
  else
    echo "Compile step skipped."
  fi
}

prompt_qemu() {
  echo "Would you like to run in QEMU?"
  echo
  echo "1. Yes"
  echo "2. Yes (headless)"
  echo "3. No"
  read -r qemu_choice
  case "$qemu_choice" in
    1)
      echo "Booting in QEMU";;
    2)
      echo "Booting in QEMU (headless)";;
    *)
      echo "QEMU launch skipped.";;
  esac
}

main() {
  print_header
  read -r choice
  case "$choice" in
    1)
      echo "Proceeding with existing environment.";;
    2)
      echo "Proceeding without installing dependencies.";;
    3)
      simulate_install;;
    *)
      echo "Invalid option selected; defaulting to try without installs.";;
  esac
  prompt_compile
  echo
  prompt_qemu
}

main
