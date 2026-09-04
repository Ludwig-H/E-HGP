#!/usr/bin/env bash
# Explicit v7 snapshot controller; preparation itself makes no cloud calls.
set -euo pipefail
exec python3 "$(dirname "$0")/v7_g4_session.py" "$@"
