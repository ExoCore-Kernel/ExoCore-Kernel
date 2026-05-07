#!/usr/bin/env bash
set -e
if [ "$(uname -s)" != "Darwin" ]; then
  echo "buildmac.sh is intended for macOS only. Use ./build.sh on this OS." >&2
  exit 1
fi
export EXOCORE_FORCE_DARWIN=1
exec "$(cd "$(dirname "$0")" && pwd)/build.sh" "$@"
