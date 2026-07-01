#!/usr/bin/env bash

set -euo pipefail

readonly workflow_files=(
  ".github/workflows/release.yml"
  ".github/workflows/release-bazel.yml"
  ".github/workflows/python.yml"
)

status=0

for file in "${workflow_files[@]}"; do
  while read -r line_no ref; do
    if [[ "$ref" == ./* ]] || [[ "$ref" == docker://* ]]; then
      continue
    fi

    if [[ ! "$ref" =~ @[0-9a-f]{40}$ ]]; then
      printf 'Mutable GitHub Actions ref in %s:%s -> %s\n' "$file" "$line_no" "$ref" >&2
      status=1
    fi
  done < <(awk '
    match($0, /^[[:space:]-]*uses:[[:space:]]*([^[:space:]#]+)/, m) {
      print NR, m[1]
    }
  ' "$file")
done

exit "$status"
