#!/usr/bin/env bash
# Suggest a build number via interactive prompts.
set -e

VERSION_FILE="version.txt"
COUNT_FILE="build.txt"
# extract the major component of the version
MAJOR=$(cut -d'.' -f1 "$VERSION_FILE" 2>/dev/null || echo 0)
# read build count from file (format: MAJOR:COUNT)
if [ -f "$COUNT_FILE" ]; then
    COUNT=$(cut -d':' -f2 "$COUNT_FILE")
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
printf "%s:%d\n" "$MAJOR" "$NEXT" > "$COUNT_FILE"
