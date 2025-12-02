# Watchdogs

## Page

1. [Introduction](#introduction)
2. [Quick Installation](#quick-installation)
3. [Platform-Specific Installation](#platform-specific-installation)
   - [Docker](#docker)
   - [Linux](#linux-bash)
   - [Termux](#termux)
   - [MSYS2](#msys2)
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

## Platform-Specific Installation

### Docker

#### Prerequisites
- Docker installed and running
- User added to docker group

#### Setup Commands
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

#### Common Docker Commands
```bash
docker ps -a                               # List of any container
docker start <container-name>              # Start the container
docker exec -it <container-name> /bin/bash # Enter running container
docker stop <container-name>               # Stop the container
docker rm -f <container-name>              # Remove the container
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
curl -L -o /etc/ssl/certs/cacert.pem "https://github.com/klantle/libwatchdogs/raw/refs/heads/dev/libwatchdogs/cacert.pem"

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
curl -L -o $HOME/cacert.pem "https://github.com/klantle/libwatchdogs/raw/refs/heads/dev/libwatchdogs/cacert.pem"

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
curl -L -o C:/libwatchdogs/cacert.pem "https://github.com/klantle/libwatchdogs/raw/refs/heads/dev/libwatchdogs/cacert.pem"

# 5. Navigate to directory
cd watch

# 6. Installing Library & Build from source
make init && make windows

# 7. Set Mod, Move executable and run
chmod +x watchdogs.win && \
mv -f watchdogs.win .. && cd .. && \
./watchdogs.win
```

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
# A.I (Wanion) for Watchdogs
# api keys
# https://aistudio.google.com/api-keys
# https://console.groq.com/keys
keys = "API_KEY"
# chatbot - "gemini" "groq"
chatbot = "gemini"
# models - llama | gpt | qwen
# groq models: https://console.groq.com/docs/models
# gemini models: https://ai.google.dev/gemini-api/docs/models
models = "gemini-2.5-pro"

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
github_tokens = "DO_HERE"
# Dependency repositories (max 101)
aio_repo = [
    "Y-Less/sscanf:latest",
    "samp-incognito/samp-streamer-plugin:latest"
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

### Downloading Our Links

> This can be used to install any archive. In this case, it is very advantageous for easily accessing gamemodes, only requiring a third party as an archive provider to serve as the link. Only zip/tar/tar.gz for extracting.

```yaml
download https://host/name/
download https://github.com/klantle/watchdogs/archive/refs/heads/dev.zip
```

### Compilation Commands

**Compile default gamemode:**
```yaml
compile .
```

**Compile specific file:**
```yaml
compile yourmode.pwn
```

**Compile with specific path:**
```yaml
compile path/to/yourmode
```

**Compile specific options:**
> Extend compiler detail & cause
```yaml
compile . --detailed
compile path/to/yourmode --detailed
```
> Clean '.amx' after compile
```yaml
compile . --clean
compile path/to/yourmode --clean
```
> Options '-d2' Debugging Mode
```yaml
compile . --debug
compile path/to/yourmode --debug
```
> Options '-a' - assembler output
```yaml
compile . --assembler
compile path/to/yourmode --assembler
```
> Options '-R+' detailed recursion report with call chains
```yaml
compile . --recursion
compile path/to/yourmode --recursion
```
> Options '-C+' compact encoding for output file
```yaml
compile . --encoding
compile path/to/yourmode --encoding
```
> Option '-v2' verbosity level - verbose
```yaml
compile . --prolix
compile path/to/yourmode --prolix
```

- Can combined
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

![img](https://raw.githubusercontent.com/klantle/watchdogs/refs/heads/dev/__DEPS.png)

**Install dependencies from `watchdogs.toml`:**
```yaml
install
```

**Install specific repository:**
```yaml
install repo/user
```

**Install specific version (tags):**
```yaml
install repo/user:v1.1
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

> I always recommend running your program in debug mode when using GDB — it makes debugging way easier.

```yaml
# step one - start gdb
gdb ./watchdogs.tmux      # termux
gdb ./watchdogs.debug     # linux
gdb ./watchdogs.debug.win # windows
# step two - run program
run # just input run
# if you have a crash | SIGINT,ETC.
bt      # default
bt full # full
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