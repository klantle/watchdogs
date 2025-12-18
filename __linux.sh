#!/usr/bin/env bash

apt update && apt install curl make git

if [ ! -f "/etc/ssl/certs/cacert.pem" ]; then
  curl -L -o /etc/ssl/certs/cacert.pem "https://github.com/gskeleton/libwatchdogs/raw/refs/heads/main/libwatchdogs/cacert.pem"
fi

rand=$((100000 + RANDOM % 900000))

if [ -d "watchdogs" ]; then
  git clone https://github.com/gskeleton/watchdogs "watchdogs_$rand"
  cd watchdogs_$rand
  make && make linux && chmod +x watchdogs && ./watchdogs
else
  git clone https://github.com/gskeleton/watchdogs "watchdogs"
  cd watchdogs
  make && make linux && chmod +x watchdogs && ./watchdogs
fi
