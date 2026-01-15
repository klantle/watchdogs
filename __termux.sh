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

if [ -d "watchdogs" ]; then
  git clone --single-branch --branch main https://github.com/gskeleton/watchdogs "watchdogs_$rand"
  cd "watchdogs_$rand"
else
  git clone --single-branch --branch main https://github.com/gskeleton/watchdogs "watchdogs"
  cd "watchdogs"
fi

make && make termux && chmod +x watchdogs.tmux && clear && ./watchdogs.tmux
