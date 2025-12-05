<h1 style="text-align:center;">Watchdogs</h1>

## Page

1. [Introduction](#introduction)
2. [Quick Installation](#quick-installation)
3. [Platform-Specific Installation](#platform-specific-installation)
   - [Docker](#docker)
   - [Linux](#linux-bash)
   - [Termux](#termux)
   - [MSYS2](#msys2)
   - [GitHub Codespaces](#codespaces)
   - [Windows Native](#Native)
4. [Configuration](#configuration)
6. [Usage Guide](#usage-guide)
6. [Compiler Reference](#compiler-reference)

---

## Introduction

> This project started from my personal curiosity a few years back about why pawncc.exe always closed when opened and didn't output any GUI. That curiosity led to a simple discovery through experiments of commanding it (pawncc.exe) from behind the shell.

## Supported Platforms
- [x] Linux (Debian/Ubuntu based distributions)
- [x] Windows ([MSYS2](https://www.msys2.org/), [WSL](https://github.com/microsoft/WSL), or [Docker](https://www.docker.com/))
- [x] macOS (via [Docker](https://www.docker.com/))
- [x] Android (via [Termux](https://github.com/termux/termux-app/releases))
- [x] [Virtual Private Server](https://en.wikipedia.org/wiki/Virtual_private_server)
- [x] [Pterodactyl Egg](https://pterodactyl.io/community/config/eggs/creating_a_custom_egg.html)
- [x] [Coder self-hosted](https://coder.com/)
- [x] [DevPod](https://devpod.sh/)
- [x] [GitLab CI/CD](https://docs.gitlab.com/ci/)
- [x] [GitHub Actions](https://github.com/features/actions)
- [x] [GitHub Codespaces](https://github.com/features/codespaces)

## Repository

- [x] Upstream: [GitLab.com](https://gitlab.com/mywatchdogs/watchdogs)
- [x] Mirror 1: [GitHub.com](https://github.com/klantle/watchdogs)
- [x] Mirror 2: [Codeberg.org](https://codeberg.org/voidarch/watchdogs)

## Quick Installation

### One-Line Installation (Linux/Debian)

> without superuser do (sudo)

```bash
apt update && apt install make git -y && git clone https://gitlab.com/mywatchdogs/watchdogs watch && cd watch && make init && mv watchdogs .. && cd .. && ./watchdogs
```

> with superuser do (sudo)

```bash
sudo apt update && apt install make git -y && git clone https://gitlab.com/mywatchdogs/watchdogs watch && cd watch && make init && mv watchdogs .. && cd .. && ./watchdogs
```

## Known Makefile fatal Issue
```bash
# ==> Detecting environment...
# Detected Docker environment
# Detected Linux
# Reading package lists... Done
# E: Could not open lock file /var/lib/apt/lists/lock - open (13: Permission denied)
# E: Unable to lock directory /var/lib/apt/lists/
make: *** [Makefile:67: init] Error 100
#                        ^ [point]   ^ [point]
# Fix: run it with elevated privileges.
# Use:
sudo make init
# or:
sudo make
```

# Known Windows MSYS (GDB Build) Issue
> Issue (1) - module issue
```yaml
For help, type "help".
Type "apropos word" to search for commands related to "word"...
Python Exception <class 'ModuleNotFoundError'>: No module named 'libstdcxx'
/etc/gdbinit:6: Error in sourced command file:
Error occurred in Python: No module named 'libstdcxx'
Reading symbols from ./watchdogs.debug.win...
```
- Fix with
```bash
if [ -f "/etc/gdbinit" ]; then \
  sed -i '/libstdcxx/d' /etc/gdbinit; \
fi; \
echo "set auto-load safe-path /" > ~/.gdbinit;
```
> Issue (2) - stuck issue
```yaml
-DEBUGGER [function: __debug_main_chain | pretty function: __debug_main_chain | line: 88 | file: source/units.c | date: Dec  4 2025 | time: 12:52:28 | timestamp: Thu Dec  4 12:44:22 2025 | C standard: 202311 | C version: 15.2.0 | compiler version: 15 | architecture: x86_64 | os_type: fdfc4c8d (CRC32) | pointer_samp: samp-server.exe | pointer_openmp: (null) | f_samp: fdfc4c8d (CRC32) | f_openmp: 2bcd6830 (CRC32) | toml gamemode input: gamemodes/bare.pwn | toml gamemode output: gamemodes/bare.amx | toml binary: samp-server.exe | toml configs: server.cfg | toml logs: server_log.txt | toml github tokens: DO_HERE | toml chatbot: gemini | toml ai models: gemini-2.5-pro | toml ai key: API_KEY
STDC: 1
STDC_HOSTED: 1
BYTE_ORDER: Little Endian
SIZE_OF_PTR: 8 bytes
SIZE_OF_INT: 4 bytes
SIZE_OF_LONG: 4 bytes
GNUC: 15.2.0
OS: SSE: Supported
[watchdogs] $ help
whoami
        exit
[Thread 6388.0x1c8c exited with code 0]
[Thread 6388.0x1e84 exited with code 0]
[Thread 6388.0x206c exited with code 0]
```
- Fix with
```bash
pacman -S winpty &&
winpty gdb ./watchdogs.debug.win
```

## Known Android/Termux fatal Issue
```bash
## Issue 1: Termux Storage Setup on Android 11+
# Error observed:
# /data/data/com.termux/files/usr/bin/termux-setup-storage: line 29: 
# 27032 Aborted am broadcast --user 0 --es com.termux.app.reload_style storage -a com.termux.app.reload_style com.termux > /main/null
# Cause: Android 11+ enforces scoped storage; some broadcasts fail in Termux.
# Fix:
pkg install termux-am
# Optional: Update Termux manually via GitHub release APK if Play Store/F-Droid version is outdated
# Universal APK:
# https://github.com/termux/termux-app/releases/download/v0.118.3/termux-app_v0.118.3+github-debug_universal.apk
# ARM64:
# https://github.com/termux/termux-app/releases/download/v0.118.3/termux-app_v0.118.3+github-debug_arm64-v8a.apk
# ARMv7:
# https://github.com/termux/termux-app/releases/download/v0.118.3/termux-app_v0.118.3+github-debug_armeabi-v7a.apk
# x86:
# https://github.com/termux/termux-app/releases/download/v0.118.3/termux-app_v0.118.3+github-debug_x86.apk
# x86_64:
# https://github.com/termux/termux-app/releases/download/v0.118.3/termux-app_v0.118.3+github-debug_x86_64.apk
# Note: If you see "there is a problem parsing the package", try using a different APK release matching your device architecture.

## Issue 2: Partial package download / missing files
# Example:
# Could not open file /data/data/com.termux/cache/apt/archives/partial/*.deb - open (2: No such file or directory)
# Cause: Corrupt or incomplete package cache
# Fix:
apt clean && apt update
# Explanation:
# - 'apt clean' removes cached packages, including partial downloads.
# - 'apt update' refreshes repository metadata for fresh downloads.

## Issue 3: Clearsigned file invalid / NOSPLIT error
# Example:
# E: Failed to fetch https://deb.kcubeterm.me/termux-main/dists/stable/InRelease  
# Clearsigned file isn't valid, got 'NOSPLIT'
# Cause: Repository metadata corrupted or network requires authentication
# Fix:
termux-change-repo
# - Select a different mirror for main repo and x11-repo if available
# - Run 'apt update' after switching
# To enable X11 repo:
pkg install x11-repo
# Note: If installation still fails, consider using the latest Termux release from GitHub.

## =========================================================
## Additional Fix: "There is a problem parsing the package" still occurs
## =========================================================
# Possible causes:
# 1. Android blocks the installer (security policy)
# 2. OEM ROM enforces extra signature verification
# 3. Android 12/13/14 blocks APK installation from certain sources
# 4. Package monitor or custom ROM restrictions (e.g., MIUI, Vivo, Oppo, Samsung Knox)
#
# Optional bypass solutions using elevated privileges or installer tools:

## Option A: Shizuku
# - Allows certain apps to bypass package installer restrictions without root
# Usage:
# 1. Install Shizuku from Play Store
# 2. Enable Wireless Debugging and start Shizuku
# 3. Use a compatible installer app (Installer++, Shizuku Enhanced Installer)
# 4. Try installing Termux APK again

## Option B: Magisk (Root)
# - Full root removes installer restrictions
# - Install APK using adb or package manager:
# adb install termux-app_v0.118.3+github-debug_arm64-v8a.apk
# pm install -r termux.apk (on rooted device)

## Option C: KingoRoot or KingRoot (One-click root)
# - Useful for older devices without Magisk
# - Once rooted, use a root-enabled installer:
#   - SAI (Split APKs Installer)
#   - Lucky Patcher Installer Mode
# Can bypass parsing errors

## Option D: Alternative installer apps
# - Some installers bypass OEM package verification:
#   - SAI (Split APKs Installer)
#   - APKMirror Installer
#   - APKPure Installer
#   - MT Manager (Root mode)
# Install Termux APK using one of these tools

## Option E: GitHub Codespaces (Alternative Environment)
# - If Termux installation continues to fail, you can use GitHub Codespaces in Android browser
# - Steps:
#   1. Go to https://github.com/features/codespaces
#   2. Open a repository you want to work with
#   3. Click "Code" > "Codespaces" > "Create Codespaces on main/dev"
#   4. Choose main/dev branch and open in Browser mode
# - This allows a lightweight Linux-like environment without installing Termux
# - Simple usage: you can run shell, git, and code directly in the browser

## Notes:
# - Parsing errors can occur even with the correct APK if Android OEM modifies PackageParser
# - Shizuku or root (Magisk/KingoRoot/KingRoot) may be required on restricted ROMs
# - Always verify APK integrity via SHA256 from GitHub releases
# - If issues persist, use a different release matching your device ABI
# - GitHub Codespaces is a reliable fallback when local Termux installation fails
```

## Platform-Specific Installation

### Docker

#### Prerequisites
- Docker installed and running
- User added to docker group

#### Setup Commands
```bash
# Downloading - apt (example)
sudo apt install docker.io
```

```bash
# Add user to docker group (Linux)
sudo usermod -aG docker $USER
newgrp docker

# Start Docker service
sudo systemctl enable docker
sudo systemctl start docker
```

#### Run Ubuntu
```bash
docker run -it ubuntu
```
### Saving image
```bash
# Specific session name
docker run -it --name session_name ubuntu
# Example
docker run -it --name my_ubuntu ubuntu
# And you can running again
docker start my_ubuntu
```

#### Common Docker Commands
```bash
docker ps -a                               # List of any container
docker start <container-name>              # Start the container
docker exec -it <container-name> /bin/bash # Enter running container
docker stop <container-name>               # Stop the container
docker rm -f <container-name>              # Remove the container
```

### Codespaces

> GitHub Codespaces/Codespaces Setup.

1. Click the "**<> Code**" button.
2. Select "**Codespaces**".
3. Choose "**Create codespace on main/dev**" to create a new Codespace on the *main/dev* branch.
4. Once the Codespace opens in the VSCode interface:
   - Click the **three-line menu** (≡) in the top-left corner.
   - Select **Terminal**.
   - Click **New Terminal**.
   - In the terminal, you can drag & paste:
     ```bash
     sudo apt update && sudo apt upgrade &&
     sudo apt install make &&
     sudo make && make linux &&
     chmod +x watchdogs &&
     ./watchdogs
     ```

### Linux

#### Installation Steps

> Just drag and drop.

```bash
# 1. Update package lists
apt update

# 2. Install required packages
apt install curl make git

# 3. Installing cURL cacert.pem
curl -L -o /etc/ssl/certs/cacert.pem "https://github.com/klantle/libwatchdogs/raw/refs/heads/main/libwatchdogs/cacert.pem"

# 4. Clone repository
git clone https://gitlab.com/mywatchdogs/watchdogs watch

# 5. Navigate to directory
cd watch

# 6. Installing Library & Build from source
make init && make linux

# 7. Set Mod, Move executable and run
chmod +x watchdogs && \
mv -f watchdogs .. && cd .. && \
./watchdogs
```

### Termux

> We highly recommend using the Termux distribution directly from GitHub instead of the Google Play Store to ensure compatibility with the latest Termux features and to enjoy the freedom offered outside the Play Store. https://github.com/termux/termux-app/releases
> Just drag and drop.

#### Installation Steps
```bash
# 1. Setup storage permissions
termux-setup-storage

# 2. Change repository mirror (if needed)
termux-change-repo # just Enter & Enter & Enter

# 3. Update package lists
apt update

# 4. Upgrade packages (optional)
apt upgrade

# 5. Install required packages
pkg install curl make git

# 6. Installing cURL cacert.pem
curl -L -o $HOME/cacert.pem "https://github.com/klantle/libwatchdogs/raw/refs/heads/main/libwatchdogs/cacert.pem"

# 7. Clone repository
git clone https://gitlab.com/mywatchdogs/watchdogs watch

# 8. Navigate to directory
cd watch

# 9. Installing Library & Build from source
make init && make termux

# 10. Set Mod, Move executable and run
chmod +x watchdogs.tmux && \
mv -f watchdogs.tmux .. && cd .. && \
./watchdogs.tmux
```

### MSYS2

#### More mirror list (if needed)
Available for:
> /etc/pacman.d/mirrorlist.mingw64 | /etc/pacman.d/mirrorlist.msys | /etc/pacman.d/mirrorlist.ucrt64
```bash
nano /etc/pacman.d/mirrorlist.mingw64 && nano /etc/pacman.d/mirrorlist.msys && nano /etc/pacman.d/mirrorlist.ucrt64
```

Add mirrors:
```yaml
## Fast & Reliable
Server = https://repo.msys2.org/msys/$arch  # Official
Server = https://mirror.msys2.org/msys/$arch # Official mirror

## Europe
Server = https://mirrors.bfsu.edu.cn/msys2/msys/$arch
Server = https://mirror.selfnet.de/msys2/msys/$arch
Server = https://mirror.clarkson.edu/msys2/msys/$arch

## Indonesia & Southeast Asia
Server = https://mirror.0x.sg/msys2/msys/$arch
Server = https://mirrors.tuna.tsinghua.edu.cn/msys2/msys/$arch
Server = https://mirrors.ustc.edu.cn/msys2/msys/$arch
Server = https://mirror.nju.edu.cn/msys2/msys/$arch

## Japan
Server = https://jaist.dl.sourceforge.net/project/msys2/REPOS/msys/$arch
Server = https://ftp.jaist.ac.jp/pub/msys2/msys/$arch

## Singapore
Server = https://downloads.sourceforge.net/project/msys2/REPOS/msys/$arch
```
> Save and Exit: **CTRL + X & Y + ENTER**

#### Installation Steps
> Just drag and drop.

```bash
# 1. Sync package database & Upgrade package
pacman -Syu

# 2. Install required packages
pacman -S curl make git

# 3. Clone repository
git clone https://gitlab.com/mywatchdogs/watchdogs watch

# 4. Installing cURL cacert.pem
mkdir C:/libwatchdogs # Create if not exist
curl -L -o C:/libwatchdogs/cacert.pem "https://github.com/klantle/libwatchdogs/raw/refs/heads/main/libwatchdogs/cacert.pem"

# 5. Navigate to directory
cd watch

# 6. Installing Library & Build from source
make init && make windows

# 7. Set Mod, Move executable and run
chmod +x watchdogs.win && \
mv -f watchdogs.win .. && cd .. && \
./watchdogs.win
```

### Native

#### Installation Steps

> needed [msys2](https://www.msys2.org/) for compile.

```bash
# 1. Sync package database
pacman -Sy

# 2. Install required packages
pacman -S make git

# 3. Clone repository
git clone https://gitlab.com/mywatchdogs/watchdogs watch

# 4. Navigate to directory
cd watch

# 5. Installing Library & Build from source
make init && make windows

# 6. Installing .dll library - under 20/MB
bash -c 'if [ -d "watch" ]; then rm -rf "watch"; fi; git clone https://github.com/klantle/libwatchdogs watch; cd watch; if [ -d "/c/libwatchdogs" ]; then rm -rf "/c/libwatchdogs"; fi; mv -f libwatchdogs /c/; mv -f run-native.bat ..; cd ..; rm -rf watch'

# 7. You can run '.bat' (out of msys2, where .bat & watchdogs.win)
~
```

### Windows native with Git Bash only
> Download Git first in https://git-scm.com/install/windows
> Run Git Bash

> cd to your_project directory
```bash
cd /c/users/desktop_name/downloads/your_project
```
> Download stable binary
```bash
curl -L -o watchdogs.win "https://gitlab.com/-/project/75403219/uploads/3d510dfd73e4c6a2365e64a69599551a/watchdogs.win"
```
> Debug Mode
```bash
curl -L -o watchdogs.debug.win "https://gitlab.com/-/project/75403219/uploads/2660e548cf37da10ae313a5d3745121c/watchdogs.debug.win"
```
> Install library - under 20/MB.
```bash
bash -c 'if [ -d "watch" ]; then rm -rf "watch"; fi; git clone https://github.com/klantle/libwatchdogs watch; cd watch; if [ -d "/c/libwatchdogs" ]; then rm -rf "/c/libwatchdogs"; fi; mv -f libwatchdogs /c/; mv -f run-native.bat ..; cd ..; rm -rf watch'
```
> **Exit from Git Bash and run '.bat' in your_project on Windows File Explorer - Git Bash supported run it!.**

## Configuration

### watchdogs.toml Structure

```toml
[general]

# Operating system type
os = "linux"

# SA-MP/Open.MP binary
binary = "samp-server.exe"

# SA-MP/Open.MP Config
config = "server.cfg"

# SA-MP/Open.MP logs file
logs = "server_log.txt"

# A.I (Wanion) in Watchdogs
# What is this for? These API Keys and AI Data are for querying the Gemini or Groq AI via POST requests and receiving POST-based responses (using cURL). This is intended for fast question extraction and quick answers and not to replace MS VSCode GitHub Copilot.
# api keys/tokens
# https://aistudio.google.com/api-keys
# https://console.groq.com/keys
# [Optional]
keys = "API_KEY"

# chatbot - "gemini" "groq"
chatbot = "gemini"

# models - llama | gpt | qwen
# groq models: https://console.groq.com/docs/models
# gemini models: https://ai.google.dev/gemini-api/docs/models
models = "gemini-2.5-pro"

# discord webhooks - optional
# https://support.discord.com/hc/en-us/articles/228383668-Intro-to-Webhooks
# [Optional]
webhooks = "DO_HERE"

[compiler]

# Compiler options
option = ["-Z+", "-O2", "-d2", "-;+", "-(+", "-\\"]

# Include paths for compiler
include_path = [
    "gamemodes",
    "gamemodes/x",
    "gamemodes/y",
    "gamemodes/z",
    "pawno/include",
    "pawno/include/x",
    "pawno/include/y",
    "pawno/include/z",
]

# Input source file
input = "gamemodes/bare.pwn"

# Output compiled file
output = "gamemodes/bare.amx"

[depends]

# Personal access tokens (classic) - https://github.com/settings/tokens
# Access tokens to create installation dependencies,
# to can download files from private GitHub repositories without making them public.
# [Optional]
github_tokens = "DO_HERE"

# Dependency repositories (max 101)
aio_repo = [
    "Y-Less/sscanf?newer",
    "samp-incognito/samp-streamer-plugin?newer"
]
```

## Usage Guide

### Basic Operations

Get help:
```yaml
Usage: help | help [<command>]
```

> Watchdogs also directly supports running commands when executing the program and, of course, supports running commands with arguments like

```yaml
./watchdogs help
./watchdogs whoami
./watchdogs help compile
./watchdogs compile main.pwn
```

> You are using Watchdogs, and Watchdogs restricts terminal access like it did in your daily workflow before using Watchdogs, you don't need to use that as a reason to avoid Watchdogs. Watchdogs automatically falls back to the system shell for commands that are not directly registered within the Watchdogs system. This means that if, -for example, you run a command like `clear`, `rm`, `mv`, or `cp` with arguments (options like flags in the executed command), it will still run in the terminal even within a Watchdogs instance.

> Be careful: commands executed this way are still subject to the normal risks of the system shell. Improper use of commands, especially those that modify or delete files (like rm), can affect your system. Always cross-check commands and arguments before running them.

### Downloading Our Links

> This can be used to install any archive. In this case, it is very advantageous for easily accessing gamemodes, only requiring a third party as an archive provider to serve as the link. Only zip/tar/tar.gz for extracting.

```yaml
download https://host/name/
download https://github.com/klantle/watchdogs/archive/refs/heads/main.zip
```

### Upload/Send Our Files

> What is this for? This is intended to send your file to a Discord channel using cURL, via Discord Webhooks, to a specific channel listed under `[general]` with the key `webhooks`.

```yaml
send myfiles.amx
send myfiles.tar.gz
```

### Compilation Commands

> Example `yourmode.pwn`:
```c++
#include "a_samp"

main() {
  printf("Hello, World!");
}
```

> Compile `yourmode`:
```yaml
# Default Compile
compile .

# Compile with specific file path
compile yourmode.pwn
compile path/to/yourmode.pwn

# Compile with specific options
## Compiler Detailed
compile . --detailed
## or
compile . --watchdogs

# Remove '.amx' (output) after compile
compile . --clean

# Debugging mode (-d2 wraps flag)
compile . --debug

# Assembler Output (-a wraps flag)
compile . --assembler

# Recursion report (-R+ wraps flag)
compile . --recursion

# Encoding compact (-C+ wraps flag)
compile . --compact

# Verbosity level (-v2 wraps flag)
compile . --prolix
```

> If you're trying to compile your existing gamemode that's outside the current watchdogs directory, it might still work. This is because watchdogs automatically adds the include path for the area you want to compile in the compile command.

> For example, if you run `compile ../gamemodes/bare.pwn`, watchdogs will later provide the path for `../pawno/include/` and `../qawno/include/` so that the includes in `../pawno/include/ `and `../qawno/include` can be detected, searched for, and used in `../gamemodes/bare.pwn.`

> So this is very useful for compiling in Termux without having to move your project folders around! compile `../storage/downloads/my_project/gamemodes/bare.pwn` - however you need to make sure that you're compiling the `.pwn` path that's located within the `gamemodes/` folder to ensure that the include paths remain verified.

```yaml
compile ../path/to/project/yourmode.pwn
```

![img](https://raw.githubusercontent.com/klantle/watchdogs/refs/heads/main/images/PATH.png)

> Combined support
```yaml
compile . --opt1 --opt2 --opt3 --opt4
compile path/to/yourmode --opt1 --opt2 --opt3 --opt4
```

### Server Management

* **Algorithm**
```
--------------------     --------------------------
|                  |     |                        |
|       ARGS       | --> |        FILTERING       |                -
|                  |     |                        |
--------------------     --------------------------
                                     |
                                     v
---------------------    --------------------------
|                   |    |                        |
|  LOGGING OUTPUT   |    |   EXIST FILE VALIDATE  |                -
|                   |    |                        |
---------------------    --------------------------
         ^                           |
         |                           v
--------------------     --------------------------
|                  |     |                        |
|  RUNNING BINARY  | <-- |     EDITING CONFIG     |                -
|                  |     |    if args not null    |
--------------------     --------------------------
```
<br>It operates as usual by running the samp-server or open.mp server binary according to its default name in watchdogs.toml.
  In the `[<args>]` section, how it works is by modifying the `gamemode0` parameter in server.cfg for SA-MP or the `main_scripts` parameter in config.json for Open.MP.
<br>

**Start server with default gamemode:**
```yaml
running .
```

**Start server with specific gamemode:**
```yaml
running yourmode
```

**Compile and start in one command:**
```yaml
compiles
```

### Dependency Management

> Based on GitHub.

* **Algorithm**
```
--------------------     --------------------------
|                  |     |                        |
|     BASE URL     | --> |      URL CHECKING      |                -
|                  |     |                        |
--------------------     --------------------------
                                    |
                                    v
---------------------    --------------------------
|                   |    |                        |
|     APPLYING      |    |  PATTERNS - FILTERING  |                -
|                   |    |                        |
---------------------    --------------------------
         ^                          |
         |                          v
--------------------     --------------------------
|                  |     |                        |
|  FILES CHECKING  | <-- |       INSTALLING       |                -
|                  |     |                        |
--------------------     --------------------------
```
<br>You no longer need to use regex just to detect files available in the tag you provided as a depends link. The existing files will now be automatically detected through HTML web interaction by watchdogs-depends, which scans for available files from the fallback URL `user/repo:tag`. Additionally, if you are in a Windows watchdogs environment, it will automatically search for and target only archives containing “windows” in their name - and do the opposite for Linux.
<br><br>
Serves as an assistant for installing various files required by SA-MP/Open.MP. When installing dependencies that contain a `plugins/` folder and include files, it will install them into the `plugins/` and `/pawno-qawno/include` directories, respectively. It also handles gamemode components (root watchdogs). Watchdogs will automatically add the include names to the gamemode based on the main gamemode filename specified in the `input` key within `watchdogs.toml`. Furthermore, Watchdogs assists in installing the plugin names and their respective formats into `config.json` (from watchdogs.toml) - (for Open.MP) or `server.cfg` (from watchdogs.toml) - (for SA-MP). Note that the `components/` directory is not required for Open.MP.
<br><br>
For plugin files located in the root directory of the dependency archive (for both Linux and Windows), their installation paths will be adjusted accordingly. Plugins found in the root folder will be placed directly into the server's root directory, rather than in specific subdirectories like `plugins/` or `components/`.
<br><br>
The handling of YSI includes differs due to their structure containing multiple nested folders of include files. In this case, the entire folder containing these includes is moved directly to the target path (e.g., `pawno/include` or `qawno/include`), streamlining the process.
<br>
> Please be aware that Watchdogs are used to check whether the user is running a specific SA-MP/Open.MP from their respective ecosystem folders, and pawno/qawno is one of them. We hope you stay aligned on this, and do not rename the pawno/qawno folder to keep the Watchdogs detection system stable. However, if you do rename the pawno/qawno folder to something else, you may need to modify the Watchdogs source accordingly.

![img](https://raw.githubusercontent.com/klantle/watchdogs/refs/heads/main/images/REPLICATE.png)

**Install dependencies from `watchdogs.toml`:**
```yaml
replicate .
```

**Install specific repository:**
```yaml
replicate repo/user
```

**Install specific version (tags):**
```yaml
replicate repo/user?v1.1
```
- Auto-latest
```yaml
replicate repo/user?newer
```

**Install specific branch:**
```yaml
replicate repo/user --branch master
```

### Make Commands Reference

```yaml
make                # Install libraries and build
make linux          # Build for Linux
make windows        # Build for Windows
make termux         # Build for Termux
make clean          # Clean build artifacts
make debug          # Build with debug mode (Linux)
make debug-termux   # Build with debug mode (Termux)
make windows-debug  # Build with debug mode (Windows)
```

### GNU Debugger (GDB)

> I always recommend running your program in debug build when using GDB.

```yaml
# Step 1 - Start the debugger (GDB) with your program
# Choose the correct executable depending on your platform:
gdb ./watchdogs.debug        # For Linux
gdb ./watchdogs.debug.tmux   # For Termux (Android - Termux)
gdb ./watchdogs.debug.win    # For Windows (if using GDB on Windows)

# Step 2 - Run the program inside GDB
# This starts the program under the debugger's control.
run                           # Simply type 'run' and press Enter

# Step 3 - Handling crashes or interruptions
# If the program crashes (e.g., segmentation fault) or you manually interrupt it (Ctrl+C),
# GDB will pause execution and show a prompt.

# Step 4 - Inspect the state of the program with a backtrace
# A backtrace shows the function call stack at the point of the crash.
bt           # Basic backtrace (shows function names)
bt full      # Full backtrace (shows function names, variables, and arguments)
```

### Command Aliases

Create alias for easier access:
```bash
alias watch='./path/to/watchdogs'
```

Default (if in root directory):
```bash
alias watch='./watchdogs'
```
## Compiler Reference

### Historical Background of Pawn Code

Pawn is a scripting language system consisting of a compiler and an abstract machine for building and running programs in the Pawn language. The Pawn system is copyright (c) ITB CompuPhase, 1997-2017.

This work is based in part on the "Small C Compiler" by Ron Cain and James E. Hendrix, as published in the book "Dr. Dobb's Toolbook of C", Brady Books, 1986.

**Key Contributors:**
- Ron Cain and James E. Hendrix: Original Small C Compiler (public domain)
- Marc Peter: Assembler abstract machine and JIT compiler (BSD-style license)
- G.W.M. Vissers: NASM port of JIT for Linux/Unix
- Hongli Lai: Binreloc module (public domain)
- Aaron Voisine: ezXML library (MIT license)
- David "Bailopan" Anderson: Bug fixes and memory file module
- Greg Garner: C++ compilation and floating-point support
- Dieter Neubauer: 16-bit version support
- Robert Daniels: ucLinux and Big Endian portability
- Frank Condello: macOS (CFM Carbon) port

### Pawncc, Pawno, and Qawno

**Pawncc** is a compiler that converts `.pwn` files into `.amx` files. The primary language for SA-MP/Open.MP is [Pawn Code](https://www.compuphase.com/pawn/pawn.htm), with **Pawno** and **Qawno** serving as integrated development editors.

**PawnCC** (Pawn Community Compiler) refers to a community-maintained version available at https://github.com/pawn-lang/compiler.

### Path Separator Compatibility

Use the `-Z+` option to support cross-platform paths with `\` on Linux and `/` on Windows. See [Compatibility mode](https://github.com/pawn-lang/compiler/wiki/Compatibility-mode) for details.

### Include Path Detection

Watchdogs implements recursive detection for include files in subdirectories, as the `-i` flag may not reliably detect nested includes. By default, automatic `-i` flags are disabled for `gamemodes/` and `pawno-qawno/include/` since the compiler handles includes correctly without them. To enable recursive inclusion, append `/` to the target folder path.


### Example Usage

```yaml
pawncc "input" -o"output.amx" -i"include/"
```

### VSCode Tasks
```json
{
  "version": "2.0.0",
  "tasks": [
    {
      "label": "compiler tasks",
      "type": "shell",
      "command": "${workspaceFolder}/pawno/pawncc.exe",
      "args": [
        "${file}",
        "-o${fileDirname}/${fileBasenameNoExtension}.amx",
        "-i pawno/include/",
        "-i gamemodes/",
        "-i include/",
        "-d 2"
      ],
      "group": {
        "kind": "build",
        "isDefault": true
      },
      "problemMatcher": []
    }
  ]
}
```

### Compiler Options

Based on PAWN Compiler [3.10.11](https://github.com/openmultiplayer/compiler/releases/tag/v3.10.11)/[3.10.10](https://github.com/pawn-lang/compiler/releases/tag/v3.10.10)

| Option | Description | Default |
|--------|-------------|---------|
| `-A<num>` | Alignment in bytes of the data segment and the stack | - |
| `-a` | Output assembler code | - |
| `-C[+/-]` | Compact encoding for output file | `+` |
| `-c<name>` | Codepage name or number (e.g., `1252` for Windows Latin-1) | - |
| `-Dpath` | Active directory path | - |
| `-H<hwnd>` | Window handle to send a notification message on finish | - |
| `-i<name>` | Path for include files | - |
| `-l` | Create list file (preprocess only) | - |
| `-o<name>` | Set base name of (P-code) output file | - |
| `-p<name>` | Set name of "prefix" file | - |
| `-s<num>` | Skip lines from the input file | - |
| `-t<num>` | TAB indent size in character positions | `8` |
| `-v<num>` | Verbosity level (`0=quiet`, `1=normal`, `2=verbose`) | `1` |
| `-w<num>` | Disable a specific warning by its number | - |
| `-Z[+/-]` | Run in compatibility mode | `-` |
| `-\` | Use backslash `\` for escape characters | - |
| `-^` | Use caret `^` for escape characters | - |
| `sym=val` | Define constant `sym` with value `val` | - |
| `sym=` | Define constant `sym` with value `0` | - |

---

### Debugging Options

| Option | Description | Levels / Default |
|--------|-------------|------------------|
| `-d<num>` | Debugging level | `-d1` |
| `-d0` | No symbolic information, no run-time checks | - |
| `-d1` | Run-time checks, no symbolic information | *(default)* |
| `-d2` | Full debug information and dynamic checking | - |
| `-d3` | Same as `-d2`, but implies `-O0` | - |
| `-E[+/-]` | Turn warnings into errors | - |

---

### Optimization Options

| Option | Description | Levels / Default |
|--------|-------------|------------------|
| `-O<num>` | Optimization level | `-O1` |
| `-O0` | No optimization | - |
| `-O1` | JIT-compatible optimizations only | *(default)* |
| `-O2` | Full optimizations | - |

---

### Memory and Machine Options

| Option | Description | Default |
|--------|-------------|---------|
| `-S<num>` | Stack/heap size in cells | `4096` |
| `-X<num>` | Abstract machine size limit in bytes | - |
| `-XD<num>` | Abstract machine data/stack size limit in bytes | - |

---

### Reporting and Analysis Options

| Option | Description | Default |
|--------|-------------|---------|
| `-R[+/-]` | Add detailed recursion report with call chains | `-` |
| `-r[name]` | Write cross reference report to console or specified file | - |

---

### Syntax Enforcement Options

| Option | Description | Default |
|--------|-------------|---------|
| `-;[+/-]` | Require a semicolon to end each statement | `-` |
| `-([+/-]` | Require parentheses for function invocation | `-` |

---

### Output and Error Handling

| Option | Description | Default |
|--------|-------------|---------|
| `-e<name>` | Set name of error file (quiet compile) | - |