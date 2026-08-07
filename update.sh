#!/usr/bin/env bash
set -e

usage() {
  cat <<'EOT'
Usage: $0 [--kernel|-k] [--mpy|-m] [--both|-b]
Without arguments a menu is shown to choose what to update.
EOT
}

update_kernel() {
  echo "Updating kernel repository..."
  if git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
    local remote
    remote=$(git remote | head -n1)
    if [ -n "$remote" ]; then
      git pull "$remote"
    else
      echo "No remote found; skipping."
    fi
  else
    echo "Not inside a git repository; skipping."
  fi
}

update_mpy() {
  local dir="micropython"
  if [ ! -d "$dir" ]; then
    echo "Cloning Micropython repository..."
    git clone --depth 1 https://github.com/micropython/micropython.git "$dir"
  else
    echo "Updating Micropython repository..."
    (cd "$dir" && git pull)
  fi
}

UPDATE_KERNEL=false
UPDATE_MPY=false

if [ $# -gt 0 ]; then
  for arg in "$@"; do
    case "$arg" in
      --kernel|-k) UPDATE_KERNEL=true ;;
      --mpy|-m) UPDATE_MPY=true ;;
      --both|-b|--all|-a) UPDATE_KERNEL=true; UPDATE_MPY=true ;;
      -h|--help) usage; exit 0 ;;
      *) echo "Unknown option: $arg"; usage; exit 1 ;;
    esac
  done
else
  echo "Select update option:"
  echo "1) Kernel repository"
  echo "2) Micropython source"
  echo "3) Both"
  echo "4) Exit"
  read -p "Choice: " choice
  case "$choice" in
    1) UPDATE_KERNEL=true ;;
    2) UPDATE_MPY=true ;;
    3) UPDATE_KERNEL=true; UPDATE_MPY=true ;;
    *) exit 0 ;;
  esac
fi

$UPDATE_KERNEL && update_kernel
$UPDATE_MPY && update_mpy

