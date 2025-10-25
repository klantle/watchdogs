set -euo pipefail
TARGET="${1:-}"

detect_platform() {
  if [[ -n "$TARGET" ]]; then
    echo "$TARGET"
    return
  fi
  uname_s="$(uname -s 2>/dev/null || echo Unknown)"
  if [[ -n "${PREFIX:-}" ]] && [[ "$PREFIX" == /data/data/* || -f /data/data/com.termux/files/usr/bin/bash ]]; then
    echo "termux" && return
  fi
  if [[ -f /data/data/com.termux/files/usr/bin/termux-info ]]; then
    echo "termux" && return
  fi
  case "$uname_s" in
    MINGW*|MSYS*|Windows_NT) echo "windows" && return ;;
  esac
  echo "linux"
}

target="$(detect_platform)"

if [[ -z "${1:-}" ]]; then
  echo "Detected target: $target"
  read -r -p "Press ENTER to continue or type (install|linux|termux|windows|debug|windows-debug|clean|compress|strip) to override: " reply
  if [[ -n "$reply" ]]; then
    target="$reply"
  fi
fi

if [[ "$target" == "clean" ]]; then
  echo "=> Cleaning build files..."
  make clean
  echo "Clean completed."
  exit 0
fi

echo "=> Running: make $target"
if ! command -v make >/dev/null 2>&1; then
  echo "ERROR: make not found." >&2
  exit 2
fi

make "$target"

if [[ "$target" == "debug" && -x "./watchdogs.debug" ]]; then
  echo "Build succeeded. Running ./watchdogs.debug"
  ./watchdogs.debug
elif [[ -x "./watchdogs" ]]; then
  echo "Build succeeded. Running ./watchdogs"
  ./watchdogs
else
  echo "Build finished but no executable found."
  ls -l ./watchdogs* || true
  exit 3
fi

