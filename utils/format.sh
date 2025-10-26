#!/usr/bin/env bash
#
# format.sh — run clang-format on all .c and .h files recursively, excluding build/
# Usage:
#   ./format.sh
#
# Optionally set CLANG_FORMAT to override the binary:
#   CLANG_FORMAT=clang-format-18 ./format.sh

set -euo pipefail

CLANG_FORMAT=${CLANG_FORMAT:-clang-format}

if ! command -v "$CLANG_FORMAT" &> /dev/null; then
    echo "❌ Error: $CLANG_FORMAT not found in PATH."
    exit 1
fi

echo "🔍 Finding and formatting .c and .h files (excluding build/)..."

find . \
    -path ./build -prune -o \
    -regex '.*\.\(c\|h\)$' \
    -exec "$CLANG_FORMAT" -i {} \;

echo "✅ Formatting complete."
