#!/bin/zsh
cd "$(dirname "$0")"
if command -v code >/dev/null 2>&1; then
  code .
else
  open .
fi
