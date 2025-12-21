#!/usr/bin/env bash
set -e

ls -a "~/Downloads"

echo

echo "Enter the path you want to switch to:"
echo "  ^ example: my_folder"
echo "  ^ a folder name in Downloads/"
read -r TARGET_DIR

if [[ -n "$TARGET_DIR" ]]; then
  if [[ -d "~/Downloads/$TARGET_DIR" ]]; then
    cd "~/Downloads/$TARGET_DIR" || { echo "Failed to cd"; exit 1; }
  else
    echo "Directory not found: ~/Downloads/$TARGET_DIR"
    exit 1
  fi
fi

echo "Now in: $(pwd)"
echo

sudo apt update && sudo apt install -y curl make git

if [ ! -f "/etc/ssl/certs/cacert.pem" ]; then
  sudo curl -L -o /etc/ssl/certs/cacert.pem \
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
make linux
chmod +x watchdogs
./watchdogs
