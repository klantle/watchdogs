#!/usr/bin/env bash
set -e

export TERM=xterm-256color

cd $HOME

if [ ! -d "storage" ]; then
  termux-setup-storage
fi

termux-change-repo

pkg update -y
pkg install -y make git

if [ ! -f "$HOME/cacert.pem" ]; then
  wget -O $HOME/cacert.pem \
    https://github.com/gskeleton/libdog/raw/refs/heads/main/libdog/cacert.pem
fi

rand=$(date +%s%N | tail -c 7)

if [ -d "dog" ]; then
  git clone --single-branch --branch main https://github.com/gskeleton/watchdogs "dog_$rand"
  cd "dog_$rand"
  if ! command -v dog >/dev/null 2>&1; then
    echo "alias dog='cd $HOME/dog_$rand && ./watchdogs.tmux'" >> ~/.bashrc
    source ~/.bashrc
  fi
else
  git clone --single-branch --branch main https://github.com/gskeleton/watchdogs "dog"
  cd "dog"
  if ! command -v dog >/dev/null 2>&1; then
    echo "alias dog='cd $HOME/watchdogs && ./watchdogs.tmux'" >> ~/.bashrc
    source ~/.bashrc
  fi
fi

make && make termux && chmod +x watchdogs.tmux && clear && echo "Later, please run 'dog' in the terminal like '$ dog.'" && ./watchdogs.tmux
