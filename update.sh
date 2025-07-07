#!/usr/bin/env bash
set -e

show_usage() {
  echo "Usage: $0 [--kernel|-k] [--mpy|-m] [--all|-a]"
  echo "Without arguments a menu is shown to choose what to update."
}

UPDATE_KERNEL=false
UPDATE_MPY=false

if [ $# -gt 0 ]; then
  for arg in "$@"; do
    case "$arg" in
      --kernel|-k)
        UPDATE_KERNEL=true
        ;;
      --mpy|-m)
        UPDATE_MPY=true
        ;;
      --all|-a)
        UPDATE_KERNEL=true
        UPDATE_MPY=true
        ;;
      -h|--help)
        show_usage
        exit 0
        ;;
      *)
        echo "Unknown option: $arg"
        show_usage
        exit 1
        ;;
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

if $UPDATE_KERNEL; then
  echo "Updating kernel repository..."
  if git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
    REMOTE=$(git remote | head -n1)
    if [ -n "$REMOTE" ]; then
      git pull -v "$REMOTE"
    fi
  fi
fi

if $UPDATE_MPY; then
  MP_DIR="micropython"
  if [ ! -d "$MP_DIR" ]; then
    echo "Cloning Micropython repository..."
    git clone --depth 1 --progress https://github.com/micropython/micropython.git "$MP_DIR"
  else
    echo "Updating Micropython repository..."
    (cd "$MP_DIR" && git pull -v)
  fi
fi

