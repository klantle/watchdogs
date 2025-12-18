#!/usr/bin/env bash

if [ ! -d "$HOME/storage" ]; then
    termux-setup-storage
fi

termux-change-repo && apt update && pkg install curl make git

if [ ! -f "$HOME/cacert.pem" ]; then
  curl -L -o $HOME/cacert.pem "https://github.com/gskeleton/libwatchdogs/raw/refs/heads/main/libwatchdogs/cacert.pem"
fi

rand=$((100000 + RANDOM % 900000))

if [ -d "watchdogs" ]; then
  git clone https://github.com/gskeleton/watchdogs "watchdogs_$rand"
  cd watchdogs_$rand
  make && make termux && chmod +x watchdogs.tmux && ./watchdogs.tmux
else
  git clone https://github.com/gskeleton/watchdogs "watchdogs"
  cd watchdogs
  make && make termux && chmod +x watchdogs.tmux && ./watchdogs.tmux
fi
