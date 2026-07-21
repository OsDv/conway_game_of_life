#!/usr/bin/env bash

set -euo pipefail

SHADER_DIR="${1:-.}"
find "$SHADER_DIR" -type f \( -name "*.fs" -o -name "*.vs" \) | while read -r shader; do
  header="${shader}.h"

  echo "Generating $header"
  xxd -i "$shader" >"$header"
done
