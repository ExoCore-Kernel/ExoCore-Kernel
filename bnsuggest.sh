#!/usr/bin/env bash
# Suggest a build number via interactive prompts.
set -e

WRITE_INFO=false
if [[ "$1" == "-w" || "$1" == "--write" ]]; then
  WRITE_INFO=true
  shift
fi

VERSION_FILE="version.txt"
COUNT_FILE="build.txt"
MAJOR=$(cut -d'.' -f1 "$VERSION_FILE" 2>/dev/null || echo 0)
if [ -f "$COUNT_FILE" ]; then
  COUNT=$(cut -d':' -f2 "$COUNT_FILE" 2>/dev/null || echo 0)
else
  COUNT=0
fi

read -rp "Describe the fix or feature: " DESC
echo "Select update type:"
echo "  M) Minor"
echo "  S) Substantial"
echo "  B) Bugfix"
echo "  F) Feature"
read -rp "Choice [M/S/B/F]: " UPDATE

read -rp "Build type (I=Internal, R=Release, T=Test, D=Debug): " TYPE

NEXT=$((COUNT + 1))
printf -v BUILD "%04d" "$NEXT"

MODEL="${MAJOR}${TYPE}${BUILD}${UPDATE}"

echo "Suggested build model: $MODEL"
printf "%s:%d" "$MAJOR" "$NEXT" > "$COUNT_FILE"

if $WRITE_INFO; then
  cat > include/buildinfo.h <<EOF
#ifndef BUILDINFO_H
#define BUILDINFO_H

#define BUILD_MODEL "$MODEL"

#endif /* BUILDINFO_H */
EOF
  echo "Updated include/buildinfo.h"
fi

