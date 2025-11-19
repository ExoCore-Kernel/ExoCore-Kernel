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

install_packages() {
  local pkgs=(build-essential nasm grub-pc-bin grub-common xorriso mtools qemu-system-x86)

  echo
  echo "Installing required keychains and software"
  progress_bar 5

  if ! $APT_UPDATED && command -v apt-get >/dev/null 2>&1; then
    export DEBIAN_FRONTEND=noninteractive
    apt-get update -y >/dev/null
    APT_UPDATED=true
  fi

  local installed_any=false

  for pkg in "${pkgs[@]}"; do
    if dpkg -s "$pkg" >/dev/null 2>&1; then
      continue
    fi
    installed_any=true
    echo "Currently installing: $pkg"
    echo "(press ctrl c to cancel)"
    apt-get install -y "$pkg" >/dev/null || true
  done

  if ! $installed_any; then
    echo "All required packages are already installed."
  fi

  progress_bar 100
  echo "Finished installing required software!"
}

compile_kernel() {
  echo
  echo "Compiling kernel via ./build.sh (logging to $LOG_FILE)"
  : > "$LOG_FILE"

  if [[ ! -d micropython/.git ]]; then
    echo "Micropython source missing; syncing via update.sh --mpy"
    ./update.sh --mpy
  fi

  set +e
  ./build.sh 2>&1 | tee -a "$LOG_FILE"
  build_status=${PIPESTATUS[0]}
  set -e

  echo
  if [[ $build_status -ne 0 ]]; then
    echo "Compilation failed. Check $LOG_FILE for details."
    exit "$build_status"
  fi

  echo "Compiling complete!"
  echo "Compiling logs:"
  if [[ -f "$LOG_FILE" ]]; then
    tail -n 5 "$LOG_FILE"
  else
    echo "(build log missing; build.sh output was not captured)"
  fi
}

run_qemu() {
  local mode="$1"
  local qemu_cmd="qemu-system-x86"

  if ! command -v "$qemu_cmd" >/dev/null 2>&1 && command -v qemu-system-x86_64 >/dev/null 2>&1; then
    qemu_cmd="qemu-system-x86_64"
  fi

  if [[ ! -f exocore.iso ]]; then
    echo "exocore.iso not found. Please compile first."
    return
  fi

  if ! command -v "$qemu_cmd" >/dev/null 2>&1; then
    echo "$qemu_cmd not available; attempting install..."
    apt-get update -y >/dev/null
    apt-get install -y qemu-system-x86 >/dev/null || true
  fi

  case "$mode" in
    gui)
      $qemu_cmd -cdrom exocore.iso -m 128M -boot order=d -serial stdio -monitor none
      ;;
    headless)
      $qemu_cmd -cdrom exocore.iso -m 128M -boot order=d -serial stdio -monitor none -display none -no-reboot
      ;;
  esac
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
