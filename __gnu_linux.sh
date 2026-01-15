#!/usr/bin/env bash
set -e

cd

if [ ! -d "download" ]; then
  mkdir "download"
fi


if command -v tree >/dev/null 2>&1; then
  tree "download"
else
  la -a "download"
fi

echo

echo "Enter the path you want to switch to location in download:"
echo "  ^ example: my_folder my_project my_server"

read -r -p "> " TARGET_DIR

if [[ -n "$TARGET_DIR" ]]; then
  if [[ -d "download/$TARGET_DIR" ]]; then
    cd "download/$TARGET_DIR" || { echo "Failed to cd"; exit 1; }
  else
    mkdir "download/$TARGET_DIR"
    cd "download/$TARGET_DIR" || { echo "Failed to cd"; exit 1; }
  fi
fi

echo "Now in: $(pwd)"
echo

rand=$(date +%s%N | tail -c 7)

if ! command -v git >/dev/null 2>&1; then
  echo "* git not found! install first.."
  echo "** apt? apt install git"
  echo "** dnf? dng install git"
  echo "** pacman? pacman -S git"
  echo "** zypper? zypper install git"
  exit
fi

if [ -d "watchdogs" ]; then
  git clone --single-branch --branch main https://github.com/gskeleton/watchdogs "watchdogs_$rand"
  cd "watchdogs_$rand"
else
  git clone --single-branch --branch main https://github.com/gskeleton/watchdogs "watchdogs"
  cd "watchdogs"
fi

if ! command -v make >/dev/null 2>&1; then
  echo "* make not found! install first.."
  echo "** apt? apt install make"
  echo "** dnf? dng install make"
  echo "** pacman? pacman -S make"
  echo "** zypper? zypper install make"
  exit
fi

if [ ! -f "/etc/ssl/certs/cacert.pem" ]; then
  if command -v sudo >/dev/null 2>&1; then
    sudo curl -L -o /etc/ssl/certs/cacert.pem \
      "https://github.com/gskeleton/libdog/raw/refs/heads/main/libdog/cacert.pem"
  else
    curl -L -o /etc/ssl/certs/cacert.pem \
      "https://github.com/gskeleton/libdog/raw/refs/heads/main/libdog/cacert.pem"
  fi
fi

make && make linux && chmod +x watchdogs && && clear && ./watchdogs
