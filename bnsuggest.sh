#!/usr/bin/env bash
# Suggest a build number via interactive prompts.
set -e

VERSION_FILE="version.txt"
COUNT_FILE="build_count.txt"
MAJOR=$(cut -d'.' -f1 "$VERSION_FILE" 2>/dev/null || echo 0)
COUNT=$(cat "$COUNT_FILE" 2>/dev/null || echo 0)

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
echo "$NEXT" > "$COUNT_FILE"
