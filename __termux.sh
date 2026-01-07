#!/usr/bin/env bash
set -e

export TERM=xterm-256color

cd $HOME

if [ ! -d "storage" ]; then
  termux-setup-storage
fi

ls -a "storage/downloads"

echo

echo "Enter the path you want to switch to location in storage/downloads:"
echo "  ^ example: my_folder my_project my_server"
echo "  ^ enter if you want install the watchdogs in home (recommended).."

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
    "https://github.com/gskeleton/libdog/raw/refs/heads/main/libdog/cacert.pem"
fi

rand=$(date +%s%N | tail -c 7)

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
