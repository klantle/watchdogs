<!-- markdownlint-disable MD033 MD041 -->
<div align="center">
  <h3>
    <a href="https://gitlab.com/mywatchdogs/watchdogs" >
    	🛹 W A T C H D O G S 🛹
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

![img](https://raw.githubusercontent.com/klantle/watchdogs/main/watchdogs.png)

---

## Page

1. [Introduction](#introduction)
2. [Quick Installation](#quick-installation)
3. [Platform-Specific Installation](#platform-specific-installation)
   - [Docker](#docker)
   - [Termux](#termux)
   - [MSYS2](#msys2)
   - [Linux (Bash)](#linux-bash)
4. [Configuration](#configuration)
6. [Usage Guide](#usage-guide)
6. [Compiler Reference](#compiler-reference)

---

## Introduction

**WATCHDOGS** is a comprehensive development toolchain for SA-MP (San Andreas Multiplayer) gamemode compilation and management. It provides a unified interface across multiple platforms for compiling, debugging, and running PAWN scripts.

### Supported Platforms
- [x] Linux (Debian/Ubuntu based distributions)
- [x] Windows (via MSYS2, WSL, or Docker)
- [x] macOS (via Docker or native environment)
- [x] Android (via Termux)

### Supported Architectures
- [x] Qualcomm Snapdragon
- [x] MediaTek  
- [x] Intel
- [x] AMD

### Prerequisites
- GNU Make
- Git
- Bash shell environment

## Quick Installation

### One-Line Installation (Linux/Debian)
> Non Sudo
```bash
apt update && apt install make git -y && git clone https://github.com/klantle/watchdogs watch && cd watch && make init && mv watchdogs .. && cd .. && ./watchdogs
```
> Sudo
```bash
sudo apt update && apt install make git -y && git clone https://github.com/klantle/watchdogs watch && cd watch && make init && mv watchdogs .. && cd .. && ./watchdogs
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

#### Build and Run
```bash
# Navigate to Docker directory
cd Docker/dockerfile

# Build Docker image
docker build -t docker_usage .

# Run container
docker run --rm -it docker_usage
```

#### Common Docker Commands
```bash
docker ps -a                               # List all containers
docker start <container-name>              # Start a container
docker exec -it <container-name> /bin/bash # Enter a running container
docker stop <container-name>               # Stop a container
docker rm -f <container-name>              # Remove a container
```

### Termux

#### Installation Steps
```bash
# 1. Setup storage permissions
termux-setup-storage

# 2. Change repository mirror (if needed)
termux-change-repo

# 3. Update package lists
apt update

# 4. Upgrade packages (optional)
apt upgrade

# 5. Install required packages
pkg install make git

# 6. Clone repository
git clone https://github.com/klantle/watchdogs watch

# 7. Navigate to directory
cd watch

# 8. Installing Library & Build from source
make init

# 9. Move executable and run
mv watchdogs.tmux .. && cd ..
./watchdogs.tmux
```

### MSYS2

#### Installation Steps
```bash
# 1. Sync package database
pacman -Sy

# 2. Install required packages
pacman -S make git

# 3. Clone repository
git clone https://github.com/klantle/watchdogs watch

# 4. Navigate to directory
cd watch

# 5. Installing Library & Build from source
make init

# 6. Move executable and run
mv watchdogs.win .. && cd ..
./watchdogs.win
```

#### Mirror Configuration (if needed)
Edit mirror lists:
```bash
nano /etc/pacman.d/mirrorlist.mingw64
nano /etc/pacman.d/mirrorlist.msys
nano /etc/pacman.d/mirrorlist.ucrt64
```

Add mirrors:
```
Server = https://repo.msys2.org/msys/$arch
Server = https://mirror.msys2.org/msys/$arch
Server = https://mirror.selfnet.de/msys2/msys/$arch
```

### Linux (Bash)

#### Installation Steps
```bash
# 1. Update package lists
apt update

# 2. Install required packages
apt install make git

# 3. Clone repository
git clone https://github.com/klantle/watchdogs watch

# 4. Navigate to directory
cd watch

# 5. Installing Library & Build from source
make init

# 6. Move executable and run
mv watchdogs .. && cd ..
./watchdogs
```

## Configuration

### watchdogs.toml Structure

```toml
[general]
# Operating system type
os = "linux"

[compiler]
# Compiler options
option = ["-Z+", "-O2", "-d2", "-;+", "-(+"]

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
# Dependency repositories (max 101)
aio_repo = [
    "Y-Less/sscanf:v2.13.8",
    "samp-incognito/samp-streamer-plugin:v2.9.6"
]
```

### VS Code Integration

Add to `.vscode/tasks.json`:

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

For WSL environments:

```json
{
  "version": "2.0.0",
  "tasks": [
    {
      "label": "watchdogs",
      "type": "shell",
      "command": "wsl ${workspaceRoot}/watchdogs",
      "group": {
        "kind": "build",
        "isDefault": true
      }
    }
  ]
}
```

## Usage Guide

### Basic Operations

Get help:
```bash
Usage: help | help [<command>]
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
compile path/to/yourmode.pwn
```

### Server Management

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
crunn
```

### Debugging

**Debug default gamemode:**
```bash
debug .
```

**Debug specific gamemode:**
```bash
debug yourmode
```

### Dependency Management

**Install dependencies from configuration:**
```bash
install
```

**Install specific repository:**
```bash
install repo/user
```

**Install specific version:**
```bash
install repo/user:v1.1
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

Based on PAWN Compiler 3.10.10

### General Options

| Option | Description | Default |
|--------|-------------|---------|
| `-A<num>` | Alignment in bytes of data segment and stack | - |
| `-a` | Output assembler code | - |
| `-C[+/-]` | Compact encoding for output file | `+` |
| `-c<name>` | Codepage name or number | - |

### Debugging Options

| Option | Description | Levels |
|--------|-------------|---------|
| `-d<num>` | Debugging level | `0-3` |
| `-d0` | No symbolic information, no run-time checks | - |
| `-d1` | Run-time checks, no symbolic information | - |
| `-d2` | Full debug information and dynamic checking | - |
| `-d3` | Same as -d2, but implies `-O0` | - |

### Optimization Options

| Option | Description | Levels |
|--------|-------------|---------|
| `-O<num>` | Optimization level | `0-2` |
| `-O0` | No optimization | - |
| `-O1` | JIT-compatible optimizations only | - |
| `-O2` | Full optimizations | - |

### Syntax Options

| Option | Description | Default |
|--------|-------------|---------|
| `-;[+/-]` | Require semicolon to end each statement | `-` |
| `-([+/-]` | Require parentheses for function invocation | `-` |
| `-E[+/-]` | Turn warnings into errors | - |

### File Options

| Option | Description |
|--------|-------------|
| `-e<name>` | Set error file name (quiet compile) |
| `-i<name>` | Path for include files |
| `-o<name>` | Set base name of output file |
| `-p<name>` | Set name of "prefix" file |

### Miscellaneous Options

| Option | Description |
|--------|-------------|
| `-v<num>` | Verbosity level (`0=quiet`, `1=normal`, `2=verbose`) |
| `-w<num>` | Disable specific warning by number |
| `-R[+/-]` | Add detailed recursion report with call chains |
| `-r[name]` | Write cross reference report |
| `-S<num>` | Stack/heap size in cells |
| `-X<num>` | Abstract machine size limit in bytes |
| `sym=val` | Define constant `sym` with value `val` |