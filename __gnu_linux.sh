#!/usr/bin/env bash
set -e

cd

if [ ! -d "download" ]; then
  mkdir "download"
fi


if command -v tree >/dev/null 2>&1; then
  tree "download"
else
  ls -a "download"
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
  if command -v apt >/dev/null 2>&1; then
  echo
  echo "  sudo apt install git"
  echo
  fi
  if command -v dnf >/dev/null 2>&1; then
  echo
  echo "  sudo dnf install git"
  echo
  fi
  if command -v pacman >/dev/null 2>&1; then
  echo
  echo "  sudo pacman -S git"
  echo
  fi
  if command -v zypper >/dev/null 2>&1; then
  echo
  echo "  sudo zypper install git"
  echo
  fi
  exit
fi

if [ -d "dog" ]; then
  git clone --single-branch --branch main https://github.com/gskeleton/watchdogs "dog_$rand"
  echo ".. Saving into: $pwd/dog_$rand"
  cd "dog_$rand"
else
  git clone --single-branch --branch main https://github.com/gskeleton/watchdogs "dog"
  echo ".. Saving into: $pwd/dog"
  cd "dog"
fi

if ! command -v make >/dev/null 2>&1; then
  echo "* make not found! install first.."
  if command -v apt >/dev/null 2>&1; then
  echo
  echo "  sudo apt install make"
  echo
  fi
  if command -v dnf >/dev/null 2>&1; then
  echo
  echo "  sudo dnf install make"
  echo
  fi
  if command -v pacman >/dev/null 2>&1; then
  echo
  echo "  sudo pacman -S make"
  echo
  fi
  if command -v zypper >/dev/null 2>&1; then
  echo
  echo "  sudo zypper install make"
  echo
  fi
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

if command -v sudo >/dev/null 2>&1; then
  sudo make && make linux && chmod +x watchdogs && clear && ./watchdogs
else
  make && make linux && chmod +x watchdogs && clear && ./watchdogs
fi