#!/usr/bin/env bash
set -e

cd $HOME

if [ ! -d "storage" ]; then
  termux-setup-storage
fi

ls -a "storage/downloads"

echo

echo "Enter the path you want to switch to location in storage/downloads:"
echo "  ^ example: my_folder"
echo "  ^ a folder name for install; the folder doesn't exist?, don't worry.."
echo "  ^ just enter if you want install the watchdogs in home.."

read -r -p "> " TARGET_DIR

if [[ -n "$TARGET_DIR" ]]; then
  if [[ -d "storage/downloads/$TARGET_DIR" ]]; then
    cd "storage/downloads/$TARGET_DIR" || { echo "Failed to cd"; exit 1; }
  else
    mkdir "storage/downloads/$TARGET_DIR"
    cd "storage/downloads/$TARGET_DIR" || { echo "Failed to cd"; exit 1; }
  fi
  echo "Now in: $(pwd)"
  echo
fi

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
