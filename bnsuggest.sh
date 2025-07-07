#!/usr/bin/env bash
# Suggest a build number using optional OpenAI API or manual prompts.
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

if [ -n "$OPENAI_API_KEY" ]; then
  PROMPT="Using the legend Major=${MAJOR}, Type=${TYPE}, Build=${BUILD}, Update=${UPDATE}. Given description: $DESC. Suggest a short build label. Only output the label."
  RESPONSE=$(curl -sS https://api.openai.com/v1/chat/completions \
    -H "Content-Type: application/json" \
    -H "Authorization: Bearer $OPENAI_API_KEY" \
    -d '{"model":"gpt-3.5-turbo","messages":[{"role":"user","content":"'$PROMPT'"}],"max_tokens":10}' || true)
  SUGGEST=$(echo "$RESPONSE" | jq -r '.choices[0].message.content' 2>/dev/null || echo "")
  if [ -n "$SUGGEST" ]; then
    MODEL="$SUGGEST"
  fi
fi

echo "Suggested build model: $MODEL"
echo "$NEXT" > "$COUNT_FILE"
