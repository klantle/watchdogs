#!/usr/bin/env bash
set -e

if [ ! -d "$HOME/storage" ]; then
  termux-setup-storage
fi

ls -a "storage/downloads"

echo

echo "Enter the path you want to switch to:"
echo "  ^ example: my_folder"
echo "  ^ a folder name in Downloads/"
read -r TARGET_DIR

if [[ -n "$TARGET_DIR" ]]; then
  if [[ -d "storage/downloads/$TARGET_DIR" ]]; then
    cd "storage/downloads/$TARGET_DIR" || { echo "Failed to cd"; exit 1; }
  else
    echo "Directory not found: storage/downloads/$TARGET_DIR"
    exit 1
  fi
fi

echo "Now in: $(pwd)"
echo

termux-change-repo
pkg update -y
pkg install -y curl make git

if [ ! -f "$HOME/cacert.pem" ]; then
  curl -L -o "$HOME/cacert.pem" \
    "https://github.com/gskeleton/libwatchdogs/raw/refs/heads/main/libwatchdogs/cacert.pem"
fi

rand=$((100000 + RANDOM % 900000))

if [ -d "watchdogs" ]; then
  git clone --single-branch --branch main https://github.com/gskeleton/watchdogs "watchdogs_$rand"
  cd "watchdogs_$rand"
else
  git clone --single-branch --branch main https://github.com/gskeleton/watchdogs "watchdogs"
  cd "watchdogs"
fi

make
make termux
chmod +x watchdogs.tmux
./watchdogs.tmux
