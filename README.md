<!-- markdownlint-disable MD033 MD041 -->
<div align="center">
  <h3>
    <a href="https://gitlab.com/mywatchdogs/watchdogs" >
         工具链 | 辅助工具 | 快速工具 | 自由工具
    </a>
  </h3>
</div>
<div align="center">
  <img src="https://readme-typing-svg.demolab.com?lines=Welcome!;Discord+Server:;to+get+help+and+support&font=Fira+Code&center=true&width=380&height=50&duration=4000&pause=1000" alt="Typing SVG">
</div>
<div align="center">
  <a href="https://discord.gg/HrWUySYS8Z" alt="Discord" title="Discord Server">
    <img src="https://img.shields.io/discord/819650821314052106?color=7289DA&logo=discord&logoColor=white&style=for-the-badge"/>
  </a>
</div>
<!-- markdownlint-enable MD033 -->

![img](https://raw.githubusercontent.com/klantle/watchdogs/refs/heads/main/__WATCH.png)

---

## Page

1. [Introduction](#introduction)
2. [Quick Installation](#quick-installation)
3. [Platform-Specific Installation](#platform-specific-installation)
   - [Docker](#docker)
   - [Termux](#termux)
   - [MSYS2](#msys2)
   - [Windows native](#Native)
   - [Linux (Bash)](#linux-bash)
4. [Configuration](#configuration)
6. [Usage Guide](#usage-guide)
6. [Compiler Reference](#compiler-reference)

---

## Introduction

## Wanion - A.I on Watchdogs
![img](https://raw.githubusercontent.com/klantle/watchdogs/refs/heads/main/__WANION.png)

## "Inspired from XXX"
> This project started from my personal curiosity a few years back about why pawncc.exe always closed when opened and didn't output any GUI. That curiosity led to a simple discovery through experiments of commanding it (pawncc.exe) from behind the shell.
> This project is built purely from our own thinking and effort, not merely by imitation. We have never claimed to be inspired by similar projects that themselves draw inspiration from others.

![img](https://raw.githubusercontent.com/klantle/watchdogs/refs/heads/main/__ACTORIDE.png)

## Supported Platforms
- [x] Linux (Debian/Ubuntu based distributions)
- [x] Windows (via Native, MSYS2, WSL, or Docker)
- [x] macOS (via Docker)
- [x] Android (via Termux)
- [x] Github Actions/VPS

## Supported Architectures
- [x] Qualcomm Snapdragon
- [x] MediaTek  
- [x] Intel
- [x] AMD

## Prerequisites
- GNU Make
- Git
- Bash shell environment

## Roadmap
| File              | Description                                                                                           |
| ----------------- | ----------------------------------------------------------------------------------------------------- |
| **wd_unit**     | This is the main Watchdogs file.                                                                      |
| **wd_util**     | This file contains utility functions for Watchdogs.                                                   |
| **wd_hardware** | This file contains hardware-related information for Watchdogs.                                        |
| **wd_crypto**   | This file handles cryptographic operations such as hashing, encryption, and decryption for Watchdogs. |
| **wd_curl**     | This file manages cURL connections, file installations, and Pawncc integration for Watchdogs.         |
| **wd_archive**  | This file handles archive management, including extraction for Watchdogs.                             |
| **wd_compiler** | This file handles the Pawncc compiler integration for Watchdogs.                                      |
| **wd_extra**    | This file contains additional functions for Watchdogs.                                        |
| **wd_depends**  | This file manages dependencies for Watchdogs.                                                         |
| **wd_package**  | This file handles the installation process for SA-MP/Open.MP and Pawncc within Watchdogs.             |
| **wd_server**   | This file manages the SA-MP/Open.MP server operations for Watchdogs.                                  |

## Quick Installation

### One-Line Installation (Linux/Debian)
> Non Sudo
```bash
apt update && apt install make git -y && git clone https://gitlab.com/mywatchdogs/watchdogs watch && cd watch && make init && mv watchdogs .. && cd .. && ./watchdogs
```
> Sudo
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
docker ps -a                               # List all containers
docker start <container-name>              # Start a container
docker exec -it <container-name> /bin/bash # Enter a running container
docker stop <container-name>               # Stop a container
docker rm -f <container-name>              # Remove a container
```

### Linux (Bash)

#### Installation Steps
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

#### Mirror Configuration (if needed)
Edit mirror lists:
```bash
nano /etc/pacman.d/mirrorlist.mingw64
nano /etc/pacman.d/mirrorlist.msys
nano /etc/pacman.d/mirrorlist.ucrt64
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
curl -L -o watchdogs.win "https://gitlab.com/-/project/75403219/uploads/49b8a96058cdb82c5f36fdf9fb7c8481/watchdogs.win"
```
> Debug Mode
```bash
curl -L -o watchdogs.win "https://gitlab.com/-/project/75403219/uploads/b5eea2bb70e45ef236fcd81a9ba4d7b0/watchdogs.debug.win"
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

### VS Code Integration
1. Create folder ".vscode"
2. Create file "tasks.json" in .vscode

```json
{
  "version": "2.0.0",
  "tasks": [
    {
      "label": "watchdogs",
      "type": "shell",
      "command": "${workspaceRoot}/watchdogs",
      "group": {
        "kind": "build",
        "isDefault": true
      }
    }
  ]
}
```
For Windows native:
```json
{
  "version": "2.0.0",
  "tasks": [
    {
      "label": "watchdogs",
      "type": "shell",
      "command": "${workspaceRoot}/run-native.bat",
      "group": {
        "kind": "build",
        "isDefault": true
      }
    }
  ]
}
```

### Sublime Text
> ANSI (Optional) https://github.com/aziz/SublimeANSI
1. Go to Tools > Build System > New Build System
2. Replace the code with:
> (FYI): You cannot run watchdogs without args (inline command) because in Sublime Text, it is not possible to run shell I/O processes simultaneously with terminal linkup like in cmd.exe or powershell.exe in VSCode.

> Windows native
```json
{
  "cmd": ["path/to/run-native.bat", "arg1", "arg2"]
}
```
```json
{
  "cmd": ["C:///", "compile", "bare.pwn"]
}
```
> Linux
```json
{
  "cmd": ["path/to/watchdogs", "arg1", "arg2"]
}
```
```json
{
  "cmd": ["/home///", "compile", "bare.pwn"]
}
```

3. Save - **CTRL + S**
4. Run: **CTRL + SHIFT & B**

## Usage Guide

### Basic Operations

Get help:
```yaml
Usage: help | help [<command>]
```
Watchdogs also directly supports running commands when executing the program and, of course, supports running commands with arguments like
```yaml
./watchdogs help
./watchdogs help compile
./watcdhgos compile main.pwn
```

### Compilation Commands

**Compile default gamemode:**
```bash
compile .
```

**Compile specific file:**
```bash
compile yourmode.pwn
```

**Compile with specific path:**
```bash
compile path/to/yourmode
```

**Compile specific options:**
> Extend compiler detail & cause
```bash
compile . --watchdogs
compile path/to/yourmode --watchdogs
```
> Clean '.amx' after compile
```bash
compile . --clean
compile path/to/yourmode --clean
```
> Options '-d2' Debugging Mode
```bash
compile . --debug
compile path/to/yourmode --debug
```
> Options '-a' - assembler output
```bash
compile . --assembler
compile path/to/yourmode --assembler
```
> Options '-R+' detailed recursion report with call chains
```bash
compile . --recursion
compile path/to/yourmode --recursion
```
> Options '-C+' compact encoding for output file
```bash
compile . --encoding
compile path/to/yourmode --encoding
```
> Option '-v2' verbosity level - verbose
```bash
compile . --verbose
compile path/to/yourmode --verbose
```

- Can combined
```bash
compile . --opt1 --opt2 --opt3 --opt4
compile path/to/yourmode --opt1 --opt2 --opt3 --opt4
```

### Server Management

* **Algorithm**
<br>It operates as usual by running the samp-server or open.mp server binary according to its default name in watchdogs.toml.
  In the `[<args>]` section, how it works is by modifying the `gamemode0` parameter in server.cfg for SA-MP or the `main_scripts` parameter in config.json for Open.MP.
<br>

**Start server with default gamemode:**
```bash
running .
```

**Start server with specific gamemode:**
```bash
running yourmode
```

**Compile and start in one command:**
```bash
compiles
```

### Dependency Management

* **Algorithm**
<br>You no longer need to use regex just to detect files available in the tag you provided as a depends link. The existing files will now be automatically detected through HTML web interaction by watchdogs-depends, which scans for available files from the fallback URL `user/repo:tag`. Additionally, if you are in a Windows watchdogs environment, it will automatically search for and target only archives containing “windows” in their name - and do the opposite for Linux.
<br><br>
Serves as an assistant for installing various files required by SA-MP/Open.MP. When installing dependencies that contain a `plugins/` folder and include files, it will install them into the `plugins/` and `/pawno-qawno/include` directories, respectively. It also handles gamemode components (root watchdogs). Watchdogs will automatically add the include names to the gamemode based on the main gamemode filename specified in the `input` key within `watchdogs.toml`. Furthermore, Watchdogs assists in installing the plugin names and their respective formats into `config.json` (from watchdogs.toml) - (for Open.MP) or `server.cfg` (from watchdogs.toml) - (for SA-MP). Note that the `components/` directory is not required for Open.MP.
<br><br>
For plugin files located in the root directory of the dependency archive (for both Linux and Windows), their installation paths will be adjusted accordingly. Plugins found in the root folder will be placed directly into the server's root directory, rather than in specific subdirectories like `plugins/` or `components/`.
<br><br>
The handling of YSI includes differs due to their structure containing multiple nested folders of include files. In this case, the entire folder containing these includes is moved directly to the target path (e.g., `pawno/include` or `qawno/include`), streamlining the process.
<br>

![img](https://raw.githubusercontent.com/klantle/watchdogs/refs/heads/main/__DEPS.png)

**Install dependencies from `watchdogs.toml`:**
```bash
install
```

**Install specific repository:**
```bash
install repo/user
```

**Install specific version (tags):**
```bash
install repo/user:v1.1
```

**Install specific branch:**
```bash
install --branch main repo/user:v1.1
```

### Make Commands Reference

```bash
make                # Install libraries and build
make linux          # Build for Linux
make windows        # Build for Windows
make termux         # Build for Termux
make clean          # Clean build artifacts
make debug          # Build with debug mode (Linux)
make debug-termux   # Build with debug mode (Termux)
make windows-debug  # Build with debug mode (Windows)
```

### XTerm Usage

```bash
xterm -hold -e ./watchdogs          # Linux
xterm -hold -e ./watchdogs.win      # Windows  
xterm -hold -e ./watchdogs.tmux     # Termux
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

* **Historical Background of Pawn Code**  
Pawn originated in the early 1990s as a small, fast, and embeddable scripting language developed by **ITB CompuPhase**, primarily by **Frank Peelen** and **Johan Bosman**. Its design was inspired by C but with simpler syntax and a lightweight virtual machine that executes *Abstract Machine eXecutable (.amx)* bytecode.  
Initially called **Small**, the language evolved into **Pawn** around 1998, when it became part of CompuPhase’s toolset for embedded systems and game engines. Its purpose was to allow rapid scripting within constrained environments, where resources like memory and processing power were limited.  
Later, the **SA-MP (San Andreas Multiplayer)** community adopted Pawn as its scripting backbone due to its lightweight structure and flexibility. Over time, community forks like **PawnCC (PCC)** emerged to modernize the compiler, add better platform support (Windows/Linux/macOS), UTF-8 encoding, path handling, and maintain active development after CompuPhase’s version became static.
<br>

* **Pawncc/PawnCC/Pawn Code/Pawno/Qawno**
<br>Pawncc is essentially an extension for converting .pwn files into .amx files (a converter). The primary language for SA-MP/Open.MP is [Pawn Code](https://www.compuphase.com/pawn/pawn.htm), and pawno/qawno are Pawn Editors designed to facilitate the integration of Pawncc itself. PawnCC refers to a modified version of Pawncc from pawn-lang - https://github.com/pawn-lang/compiler, which means Pawn Community Compiler (PawnCC or PCC).
<br>

* **Path Separator**
<br>You need the -Z+ option if it exists to support specific paths with `\` on Linux and `/` on Windows for cross-platform compatibility. https://github.com/pawn-lang/compiler/wiki/Compatibility-mode
<br>

* **Include Path**
<br>There may be instances where the -i"path/" option does not reliably detect include files located in subdirectories within the specified path. To address this, Watchdogs implements its own detection mechanism to recursively scan and add all folders within pawno-qawno/include and gamemodes/.
<br>By default, Watchdogs disables the automatic `-i` flag for folders under `gamemodes/` or `pawno-qawno/include/` because the compiler can still handle includes correctly even if the flag is not explicitly set. Users can manually enable it by adding a trailing / to the target folder, allowing the compiler to automatically include all subdirectories under that path.
<br><br>

### Example Usage

```bash
pawncc "input" -o"output.amx" -i"include/"
```

### VSCode Tasks

```json
{
  "version": "2.0.0",
  "tasks": [
    {
      "label": "Compile Pawn Script",
      "type": "shell",
      "command": "${workspaceFolder}/pawno/pawncc.exe",
      "args": [
        "${file}",
        "-o${fileDirname}/${fileBasenameNoExtension}.amx",
        "-ipawno/include/",
        "-igamemodes/",
        "-iinclude/",
        "-d2"
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
