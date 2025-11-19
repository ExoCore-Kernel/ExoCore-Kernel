#!/usr/bin/env bash
set -euo pipefail

CHOICE_INSTALL=""
CHOICE_COMPILE=""
CHOICE_QEMU=""

# -----------------------
# Parse Arguments
# -----------------------
while [[ $# -gt 0 ]]; do
  case "$1" in
    --auto)
      CHOICE_INSTALL="3"
      CHOICE_COMPILE="1"
      CHOICE_QEMU="3"
      ;;
    --install-deps)
      CHOICE_INSTALL="3"
      ;;
    --skip-install)
      CHOICE_INSTALL="2"
      ;;
    --yes-compile)
      CHOICE_COMPILE="1"
      ;;
    --no-compile)
      CHOICE_COMPILE="3"
      ;;
    --yes-qemu)
      CHOICE_QEMU="1"
      ;;
    --no-qemu)
      CHOICE_QEMU="3"
      ;;
  esac
  shift
done

HEADER="Welcome to EXOCORE-TEST-BUILD"
LOG_FILE="test_build.log"
APT_UPDATED=false

print_header() {
  echo "$HEADER"
  echo "Have you installed all required software to compile the kernel?"
  echo
  echo "1. Yes"
  echo "2. No, Try anyway"
  echo "3. No, install for me"
}

get_install_choice() {
  if [[ -n "${CHOICE_INSTALL}" ]]; then
    echo "$CHOICE_INSTALL"
    return
  fi
  read -r c
  echo "$c"
}

get_compile_choice() {
  if [[ -n "${CHOICE_COMPILE}" ]]; then
    echo "$CHOICE_COMPILE"
    return
  fi
  read -r c
  echo "$c"
}

get_qemu_choice() {
  if [[ -n "${CHOICE_QEMU}" ]]; then
    echo "$CHOICE_QEMU"
    return
  fi
  read -r c
  echo "$c"
}

# (your original functions here, unchanged except for the qemu-system-x86 fix)

main() {
  print_header
  choice=$(get_install_choice)

  case "$choice" in
    1) echo "Proceeding with existing environment."; ;;
    2) echo "Proceeding without installing dependencies."; ;;
    3) install_packages; ;;
    *) echo "Invalid option selected; defaulting to try without installs."; ;;
  esac

  echo "Would you like to Compile the kernel?"
  echo "1. Yes"
  echo "3. No"
  compile_choice=$(get_compile_choice)

  if [[ "$compile_choice" == "1" ]]; then
    compile_kernel
  else
    echo "Skipping compile."
  fi

  echo "Would you like to run in QEMU?"
  echo "1. Yes"
  echo "2. Yes (headless)"
  echo "3. No"

  qemu_choice=$(get_qemu_choice)

  case "$qemu_choice" in
    1) run_qemu gui ;;
    2) run_qemu headless ;;
    *) echo "Skipping QEMU."; ;;
  esac
}

main "$@"
