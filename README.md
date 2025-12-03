<h1 style="text-align:center;">Watchdogs</h1>

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

# A.I (Wanion) in Watchdogs
# What is this for? These API Keys and AI Data are for querying the Gemini or Groq AI via POST requests and receiving POST-based responses (using cURL). This is intended for fast question extraction and quick answers and not to replace MS VSCode GitHub Copilot.
# api keys/tokens
# https://aistudio.google.com/api-keys
# https://console.groq.com/keys
keys = "API_KEY"

# chatbot - "gemini" "groq"
chatbot = "gemini"

# models - llama | gpt | qwen
# groq models: https://console.groq.com/docs/models
# gemini models: https://ai.google.dev/gemini-api/docs/models
models = "gemini-2.5-pro"

# discord webhooks - optional
# https://support.discord.com/hc/en-us/articles/228383668-Intro-to-Webhooks
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

### Upload/Send Our Files

> What is this for? This is intended to send your file to a Discord channel using cURL, via Discord Webhooks, to a specific channel listed under `[general]` with the key `webhooks`.

```yaml
send myfiles.amx
send myfiles.tar.gz
```

### Compilation Commands

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
replicate .
```

**Install specific repository:**
```yaml
replicate repo/user
```

**Install specific version (tags):**
```yaml
replicate repo/user:v1.1
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
# This starts the program under the debugger’s control.
run                           # Simply type 'run' and press Enter

# Step 3 - Handling crashes or interruptions
# If the program crashes (e.g., segmentation fault) or you manually interrupt it (Ctrl+C),
# GDB will pause execution and show a prompt.

# Step 4 - Inspect the state of the program with a backtrace
# A backtrace shows the function call stack at the point of the crash.
bt           # Basic backtrace (shows function names)
bt full      # Full backtrace (shows function names, variables, and arguments)
```

> Comparing
- without debug build
```js
Thread 1 "watchdogs" received signal SIGSEGV, Segmentation fault.
Download failed: Invalid argument.  Continuing without source file ./string/../sysdeps/x86_64/multiarch/memmove-vec-unaligned-erms.S.
__memcpy_avx_unaligned_erms () at ../sysdeps/x86_64/multiarch/memmove-vec-unaligned-erms.S:660
warning: 660    ../sysdeps/x86_64/multiarch/memmove-vec-unaligned-erms.S: No such file or directory
(gdb) bt
#0  __memcpy_avx_unaligned_erms () at ../sysdeps/x86_64/multiarch/memmove-vec-unaligned-erms.S:660
#1  0x000055555555c53c in write_callbacks ()
#2  0x00007ffff7f5741c in ?? () from /lib/x86_64-linux-gnu/libcurl.so.4
#3  0x00007ffff7f557f8 in ?? () from /lib/x86_64-linux-gnu/libcurl.so.4
#4  0x00007ffff7f6c4b5 in ?? () from /lib/x86_64-linux-gnu/libcurl.so.4
#5  0x00007ffff7f50fb3 in ?? () from /lib/x86_64-linux-gnu/libcurl.so.4
#6  0x00007ffff7f540e5 in curl_multi_perform () from /lib/x86_64-linux-gnu/libcurl.so.4
#7  0x00007ffff7f243ab in curl_easy_perform () from /lib/x86_64-linux-gnu/libcurl.so.4
#8  0x0000555555564883 in dep_http_get_content ()
#9  0x00005555555677bb in wg_install_depends ()
#10 0x0000555555561061 in __command__ ()
#11 0x000055555556177c in chain_goto_main ()
#12 0x000055555555b67d in main ()
(gdb) bt full
#0  __memcpy_avx_unaligned_erms () at ../sysdeps/x86_64/multiarch/memmove-vec-unaligned-erms.S:660
No locals.
#1  0x000055555555c53c in write_callbacks ()
No symbol table info available.
#2  0x00007ffff7f5741c in ?? () from /lib/x86_64-linux-gnu/libcurl.so.4
No symbol table info available.
#3  0x00007ffff7f557f8 in ?? () from /lib/x86_64-linux-gnu/libcurl.so.4
No symbol table info available.
#4  0x00007ffff7f6c4b5 in ?? () from /lib/x86_64-linux-gnu/libcurl.so.4
No symbol table info available.
#5  0x00007ffff7f50fb3 in ?? () from /lib/x86_64-linux-gnu/libcurl.so.4
No symbol table info available.
#6  0x00007ffff7f540e5 in curl_multi_perform () from /lib/x86_64-linux-gnu/libcurl.so.4
No symbol table info available.
#7  0x00007ffff7f243ab in curl_easy_perform () from /lib/x86_64-linux-gnu/libcurl.so.4
No symbol table info available.
#8  0x0000555555564883 in dep_http_get_content ()
No symbol table info available.
#9  0x00005555555677bb in wg_install_depends ()
No symbol table info available.
#10 0x0000555555561061 in __command__ ()
No symbol table info available.
--Type <RET> for more, q to quit, c to continue without paging--exit
#11 0x000055555556177c in chain_goto_main ()
No symbol table info available.
#12 0x000055555555b67d in main ()
No symbol table info available.
```
- with debug build
```js
Thread 1 "watchdogs.debug" received signal SIGSEGV, Segmentation fault.
__memcpy_avx_unaligned_erms () at ../sysdeps/x86_64/multiarch/memmove-vec-unaligned-erms.S:660
warning: 660    ../sysdeps/x86_64/multiarch/memmove-vec-unaligned-erms.S: No such file or directory
(gdb) bt
#0  __memcpy_avx_unaligned_erms () at ../sysdeps/x86_64/multiarch/memmove-vec-unaligned-erms.S:660
#1  0x000055555555cd09 in write_callbacks (ptr=0x55555572e685, size=1, nmemb=8501, userdata=0x7fffffff5a80) at source/curl.c:210
#2  0x00007ffff7f5741c in ?? () from /lib/x86_64-linux-gnu/libcurl.so.4
#3  0x00007ffff7f557f8 in ?? () from /lib/x86_64-linux-gnu/libcurl.so.4
#4  0x00007ffff7f6c4b5 in ?? () from /lib/x86_64-linux-gnu/libcurl.so.4
#5  0x00007ffff7f50fb3 in ?? () from /lib/x86_64-linux-gnu/libcurl.so.4
#6  0x00007ffff7f540e5 in curl_multi_perform () from /lib/x86_64-linux-gnu/libcurl.so.4
#7  0x00007ffff7f243ab in curl_easy_perform () from /lib/x86_64-linux-gnu/libcurl.so.4
#8  0x00005555555698dd in dep_http_get_content (url=0x7fffffff5d00 "https://api.github.com/repos/Y-Less/sscanf/releases/latest", github_token=0x5555556cbf40 "DO_HERE", out_html=0x7fffffff5ce0)
    at source/depends.c:176
#9  0x000055555556ac71 in dep_gh_latest_tag (user=0x7fffffff6230 "Y-Less", repo=0x7fffffff62b0 "sscanf", out_tag=0x7fffffff5fe0 "", deps_put_size=128) at source/depends.c:535
#10 0x000055555556af65 in dep_handle_repo (dep_repo_info=0x7fffffff61d0, deps_put_url=0x7fffffff66e0 "\001", deps_put_size=1024) at source/depends.c:588
#11 0x000055555556e317 in wg_install_depends (depends_string=0x5555557135f0 "Y-Less/sscanf:latest samp-incognito/samp-streamer-plugin:latest") at source/depends.c:1627
#12 0x00005555555611d3 in __command__ (chain_pre_command=0x0) at source/units.c:517
#13 0x0000555555564c64 in chain_goto_main (chain_pre_command=0x0) at source/units.c:1602
#14 0x0000555555564f67 in main (argc=1, argv=0x7fffffffcf48) at source/units.c:1666
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