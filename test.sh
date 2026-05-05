#!/usr/bin/env bash
set -Eeuo pipefail

HEADER="Welcome to EXOCORE-TEST-BUILD"
LOG_FILE="test_build.log"
APT_UPDATED=false

CHOICE_INSTALL=""
CHOICE_COMPILE=""
CHOICE_QEMU=""

PKGS=(
  build-essential
  nasm
  grub-pc-bin
  grub-common
  xorriso
  mtools
  qemu-system-x86
)

die() {
  echo "Error: $*" >&2
  exit 1
}

run_as_root() {
  if [[ $EUID -eq 0 ]]; then
    "$@"
  elif command -v sudo >/dev/null 2>&1; then
    sudo "$@"
  else
    die "This needs root. Please install sudo or run as root."
  fi
}

usage() {
  cat <<EOF
Usage: $0 [options]

Options:
  --auto            Install deps, compile, skip QEMU
  --install-deps    Install required packages
  --skip-install    Skip dependency install
  --yes-compile     Compile kernel
  --no-compile      Skip compile
  --yes-qemu        Run QEMU GUI
  --headless-qemu   Run QEMU headless
  --no-qemu         Skip QEMU
  -h, --help        Show help
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --auto)
      CHOICE_INSTALL="install"
      CHOICE_COMPILE="yes"
      CHOICE_QEMU="no"
      ;;
    --install-deps) CHOICE_INSTALL="install" ;;
    --skip-install) CHOICE_INSTALL="skip" ;;
    --yes-compile) CHOICE_COMPILE="yes" ;;
    --no-compile) CHOICE_COMPILE="no" ;;
    --yes-qemu) CHOICE_QEMU="gui" ;;
    --headless-qemu) CHOICE_QEMU="headless" ;;
    --no-qemu) CHOICE_QEMU="no" ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      die "Unknown option: $1"
      ;;
  esac
  shift
done

progress_bar() {
  local percent=$1
  local bar_width=20
  local filled=$((percent * bar_width / 100))
  local empty=$((bar_width - filled))

  printf '['
  for ((i = 0; i < filled; i++)); do printf '#'; done
  for ((i = 0; i < empty; i++)); do printf '-'; done
  printf '] %3d%%\n' "$percent"
}

install_packages() {
  command -v apt-get >/dev/null 2>&1 || die "apt-get not found. This installer only supports Debian/Ubuntu."

  echo
  echo "Installing required software..."
  progress_bar 5

  if [[ "$APT_UPDATED" == false ]]; then
    run_as_root env DEBIAN_FRONTEND=noninteractive apt-get update -y
    APT_UPDATED=true
  fi

  local missing=()

  for pkg in "${PKGS[@]}"; do
    if ! dpkg -s "$pkg" >/dev/null 2>&1; then
      missing+=("$pkg")
    fi
  done

  if [[ ${#missing[@]} -eq 0 ]]; then
    echo "All required packages are already installed."
  else
    echo "Installing: ${missing[*]}"
    run_as_root env DEBIAN_FRONTEND=noninteractive apt-get install -y "${missing[@]}"
  fi

  progress_bar 100
  echo "Finished installing required software!"
}

compile_kernel() {
  [[ -f ./build.sh ]] || die "build.sh not found."
  [[ -x ./build.sh ]] || die "build.sh exists but is not executable. Run: chmod +x build.sh"

  echo
  echo "Compiling kernel via ./build.sh"
  echo "Logging to $LOG_FILE"
  : > "$LOG_FILE"

  set +e
  ./build.sh 2>&1 | tee -a "$LOG_FILE"
  local build_status=${PIPESTATUS[0]}
  set -e

  echo

  if [[ $build_status -ne 0 ]]; then
    echo "Compilation failed. Last log lines:"
    tail -n 20 "$LOG_FILE"
    exit "$build_status"
  fi

  echo "Compile complete!"
  echo "Last log lines:"
  tail -n 5 "$LOG_FILE"
}

find_qemu() {
  for candidate in qemu-system-x86_64 qemu-system-i386 qemu-system-x86; do
    if command -v "$candidate" >/dev/null 2>&1; then
      echo "$candidate"
      return 0
    fi
  done

  return 1
}

run_qemu() {
  local mode="$1"

  [[ -f exocore.iso ]] || die "exocore.iso not found. Please compile first."

  local qemu_cmd
  if ! qemu_cmd=$(find_qemu); then
    echo "QEMU not found. Trying to install it..."
    install_packages
    qemu_cmd=$(find_qemu) || die "QEMU could not be installed."
  fi

  case "$mode" in
    gui)
      "$qemu_cmd" \
        -cdrom exocore.iso \
        -m 128M \
        -boot order=d \
        -serial stdio \
        -monitor none
      ;;
    headless)
      "$qemu_cmd" \
        -cdrom exocore.iso \
        -m 128M \
        -boot order=d \
        -serial stdio \
        -monitor none \
        -display none \
        -no-reboot
      ;;
    *)
      die "Unknown QEMU mode: $mode"
      ;;
  esac
}

ask_install_choice() {
  if [[ -n "$CHOICE_INSTALL" ]]; then
    echo "$CHOICE_INSTALL"
    return
  fi

  echo "$HEADER"
  echo "Have you installed all required software to compile the kernel?"
  echo
  echo "1. Yes"
  echo "2. No, try anyway"
  echo "3. No, install for me"
  read -rp "> " c

  case "$c" in
    1) echo "ready" ;;
    2) echo "skip" ;;
    3) echo "install" ;;
    *) echo "skip" ;;
  esac
}

ask_compile_choice() {
  if [[ -n "$CHOICE_COMPILE" ]]; then
    echo "$CHOICE_COMPILE"
    return
  fi

  echo
  echo "Would you like to compile the kernel?"
  echo "1. Yes"
  echo "3. No"
  read -rp "> " c

  case "$c" in
    1) echo "yes" ;;
    *) echo "no" ;;
  esac
}

ask_qemu_choice() {
  if [[ -n "$CHOICE_QEMU" ]]; then
    echo "$CHOICE_QEMU"
    return
  fi

  echo
  echo "Would you like to run in QEMU?"
  echo "1. Yes"
  echo "2. Yes, headless"
  echo "3. No"
  read -rp "> " c

  case "$c" in
    1) echo "gui" ;;
    2) echo "headless" ;;
    *) echo "no" ;;
  esac
}

main() {
  local install_choice compile_choice qemu_choice

  install_choice=$(ask_install_choice)

  case "$install_choice" in
    ready) echo "Proceeding with existing environment." ;;
    skip) echo "Proceeding without installing dependencies." ;;
    install) install_packages ;;
  esac

  compile_choice=$(ask_compile_choice)

  if [[ "$compile_choice" == "yes" ]]; then
    compile_kernel
  else
    echo "Skipping compile."
  fi

  qemu_choice=$(ask_qemu_choice)

  case "$qemu_choice" in
    gui) run_qemu gui ;;
    headless) run_qemu headless ;;
    no) echo "Skipping QEMU." ;;
  esac
}

main
