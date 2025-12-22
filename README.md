<h1 style="text-align:center;">Watchdogs</h1>

## Page

1. [Introduction](#introduction)
2. [Platform-Specific Installation](#platform-specific-installation)
   - [Docker](#docker)
   - [Linux](#linux)
   - [Termux](#termux)
   - [Windows Native](#Native)
   - [MSYS2](#msys2)
   - [GitHub Codespaces](#codespaces)
3. [Usage Guide](#usage-guide)

## Introduction

> This project started from my personal curiosity a few years back about why pawncc.exe always closed when opened and didn't output any GUI. That curiosity led to a simple discovery through experiments of commanding it (pawncc.exe) from behind the shell.

## Supported Platforms
- Linux (Debian/Ubuntu based distributions)
- Windows ([MSYS2](https://www.msys2.org/), [WSL](https://github.com/microsoft/WSL), or [Docker](https://www.docker.com/))
- macOS (via [Docker](https://www.docker.com/))
- Android (via [Termux](https://github.com/termux/termux-app/releases))
- [Virtual Private Server](https://en.wikipedia.org/wiki/Virtual_private_server)
- [Pterodactyl Egg](https://pterodactyl.io/community/config/eggs/creating_a_custom_egg.html)
- [GitLab CI/CD](https://docs.gitlab.com/ci/)
- [GitHub Actions](https://github.com/features/actions)
- [GitHub Codespaces](https://github.com/features/codespaces)

## Platform-Specific Installation

## Codespaces

1. Click the "**<> Code**" button.
2. Select "**Codespaces**".
3. Choose "**Create codespace on main**" to create a new Codespace on the *main* branch.
4. Once the Codespace opens in the VSCode interface:
- Click the **three-line menu** (≡) in the top-left corner.
- Select **Terminal**.
- Click **New Terminal**.
- In the terminal, you can drag & paste:
```yaml
sudo apt update && sudo apt upgrade &&
sudo apt install make &&
sudo make && make linux &&
chmod +x watchdogs &&
./watchdogs
```

## Docker

#### Prerequisites

- Docker installed and running
- User added to docker group

#### Setup Commands

```yaml
# Downloading - apt << Ubuntu >>
sudo apt install docker.io
```

```yaml
# Add user to docker group (Linux)
sudo usermod -aG docker $USER
newgrp docker

# Start Docker service
sudo systemctl enable docker
sudo systemctl start docker
```

#### Run Ubuntu

```yaml
docker run -it ubuntu
```

### Saving image

```yaml
# Specific session name
docker run -it --name session_name ubuntu
# Sample
docker run -it --name my_ubuntu ubuntu
# And you can running again
docker start my_ubuntu
docker exec -it my_ubuntu /bin/bash
# Exec with specific user
sudo docker exec -it --user system my_ubuntu sh
```

#### Common Docker Commands

```yaml
docker ps -a                               # List all containers
docker start <container-name>              # Start the container
docker exec -it <container-name> /bin/bash # Enter the running container
docker stop <container-name>               # Stop the container
docker rm -f <container-name>              # Force-remove the container
```

## Linux

```yaml
# 1. Update package lists
apt update

# 2. Install required packages
apt install curl make git

# 3. Installing cURL cacert.pem
curl -L -o /etc/ssl/certs/cacert.pem "https://github.com/gskeleton/libwatchdogs/raw/refs/heads/main/libwatchdogs/cacert.pem"

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

> One Command (Automatic):
```sh
wget -O install.sh https://github.com/gskeleton/watchdogs/raw/refs/heads/main/__linux.sh && chmod +x install.sh && ./install.sh
```

## Termux

> We highly recommend using the Termux distribution directly from GitHub instead of the Google Play Store to ensure compatibility with the latest Termux features and to enjoy the freedom offered outside the Play Store. https://github.com/termux/termux-app/releases
> Just drag this into your terminal and run it.

```yaml
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
curl -L -o $HOME/cacert.pem "https://github.com/gskeleton/libwatchdogs/raw/refs/heads/main/libwatchdogs/cacert.pem"

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

> One Command (Automatic):
```sh
wget -O install.sh https://github.com/gskeleton/watchdogs/raw/refs/heads/main/__termux.sh && chmod +x install.sh && ./install.sh
```

## Native

1. Installing Visual C++ Redistributable Runtimes All-in-One first (needed for pawncc)
- Go to https://www.techpowerup.com/download/visual-c-redistributable-runtime-package-all-in-one/
- Click the "Download"
- Extract the Archive
- Just run the "install_all.bat".
2. Open Windows Powershell
- Run:
```yml
Invoke-WebRequest "https://raw.githubusercontent.com/gskeleton/watchdogs/refs/heads/main/__windows.cmd" -OutFile "install.cmd"; cmd /c install.cmd
```
- Exit from Windows Powershell.
- Run file: "run-native.bat".
- Done.

## MSYS2 (For Windows Build)

```yaml
# 1. Sync package database & Upgrade package
pacman -Syu

# 2. Install required packages
pacman -S curl make git

# 3. Clone repository
git clone https://gitlab.com/mywatchdogs/watchdogs watch

# 4. Installing cURL cacert.pem
mkdir C:/libwatchdogs # Create if not exist
curl -L -o C:/libwatchdogs/cacert.pem "https://github.com/gskeleton/libwatchdogs/raw/refs/heads/main/libwatchdogs/cacert.pem"

# 5. Navigate to directory
cd watch

# 6. Installing Library & Build from source
make init && make windows

# 7. Set Mod, Move executable and run
chmod +x watchdogs.win && \
mv -f watchdogs.win .. && cd .. && \
./watchdogs.win
```

### More mirror list (if needed)

- Available for: `/etc/pacman.d/mirrorlist.mingw64` | `/etc/pacman.d/mirrorlist.msys` | `/etc/pacman.d/mirrorlist.ucrt64`
```yaml
nano /etc/pacman.d/mirrorlist.mingw64 && nano /etc/pacman.d/mirrorlist.msys && nano /etc/pacman.d/mirrorlist.ucrt64
```

- Add mirrors:
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

## Known Issue

| Platform | Issue | Fix |
|---|---|---|
| **Linux/Docker** | Permission denied on apt lock file | `sudo make init` or `sudo make` |
| **Windows (MSYS)** | GDB: No module named 'libstdcxx' | Run: `sed -i '/libstdcxx/d' /etc/gdbinit 2>/dev/null; echo "set auto-load safe-path /" > ~/.gdbinit` |
| **Windows (MSYS)** | GDB gets stuck at startup | Install winpty: `pacman -S winpty`<br>Then use: `winpty gdb ./watchdogs.debug.win` |
| **Android/Termux** | Storage setup fails (Android 11+) | Install: `pkg install termux-am` |
| **Android/Termux** | Partial package download errors | Clean cache: `apt clean && apt update` |
| **Android/Termux** | Repository/Clearsigned file error | Change repo: `termux-change-repo` |
| **Android** | "Problem parsing the package" error | Try different APK from GitHub releases<br>Or use installer: **SAI (Split APKs Installer)**<br>Or use **GitHub Codespaces** in browser |
| **Android** | APK installation blocked by OEM | Use: **Shizuku** (non-root) or **Magisk/KingoRoot** (root)<br>Or alternative installers: APKMirror, APKPure, MT Manager |

## `watchdogs.toml` Configuration

**Key Points:**
- **API Keys**: For Gemini or Groq AI. (optional)
- **Webhooks**: Discord webhooks for `send` command. (optional)
- **GitHub Tokens**: For private repositories. (optional)
- **Compiler**: PAWN compiler settings for SA-MP/Open.MP
- **Root Patterns**: This is useful for hard list keywords, specifically for plugin names (dll/so) like sscanf amxsscanf (amx)sscanf or mysql log-core log(core), which will later be installed and still recognized as being installed by the root user into the gamemode root area, rather than being installed within plugins/.

## Command Aliases

Default (if in root directory):
```yaml
echo "alias watchdogs='./watchdogs'" >> ~/.bashrc
source ~/.bashrc
```

And run the alias:
```yml
watchdogs
```

## Usage Guide

## Basic Operations

> Watchdogs inline:

```yaml
./watchdogs help
./watchdogs whoami
./watchdogs ls -a
./watchdogs compile server.pwn
```

## Compiler Installer

> Use `pawncc` command to easily download the pawn compiler from pawn-lang.

![img](https://raw.githubusercontent.com/gskeleton/watchdogs/refs/heads/main/images/compiler.png)

## Compilation Commands

> Compile `server.pwn`:
```yaml
# Default Compile
compile .
```
> Compile with specific file path
```yml
compile server.pwn
compile path/to/server.pwn
```
![img](https://raw.githubusercontent.com/gskeleton/watchdogs/refs/heads/main/images/compile_path.png)

> Compile with specific options
```yml
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
compile path/to/server --opt1 --opt2 --opt3 --opt4
```

1. If you're trying to compile your existing gamemode that's outside the current watchdogs directory, it might still work. This is because watchdogs automatically adds the include path for the area you want to compile in the compile command.

2. for example, if you run `compile ../gamemodes/bare.pwn`, watchdogs will later provide the path for `../pawno/include/` and `../qawno/include/` also `/gamemodes` so that the includes in `../pawno/include/ `and `../qawno/include` also `../gamemodes` can be detected, searched for, and used in `../gamemodes/bare.pwn.`

3. So this is very useful for compiling in Termux without having to move your project folders around! compile `../storage/downloads/my_project/gamemodes/bare.pwn` - however you need to make sure that you're compiling the `.pwn` path that's located within the `gamemodes/` folder to ensure that the include paths remain verified.

```yaml
compile ../path/to/project/server.pwn
```

![img](https://raw.githubusercontent.com/gskeleton/watchdogs/refs/heads/main/images/parent_dir.png)

## Server Management

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
running server
```

**Compile and start in one command:**
```yaml
compiles
```

**Compile and start with specific path:**
```yml
compiles server
```

## Dependency Management

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

1. Watchdogs-depends now scans HTML from GitHub tags (`user/repo?tag`) instead of manual parsing.
2. Automatically picks Windows/Linux archives based on OS detection
3. Entire any include folder moved as-is to include directory
4. Plugin names automatically added to `server.cfg` (SA-MP) or `config.json` (Open.MP)
5. Plugins in archive root go to server root directory
6. Automatically adds includes to main gamemode file
7. Keep `pawno/` or `qawno/` folder names unchanged for proper detection

![img](https://raw.githubusercontent.com/gskeleton/watchdogs/refs/heads/main/images/replicate.png)

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

## Make Commands Reference

```yaml
make                # Install libraries and build
make linux          # Build for Linux
make windows        # Build for Windows
make termux         # Build for Termux
make termux-fast    # Build for Termux [fast-build]
make clean          # Clean build artifacts
make debug          # Build with debug mode (Linux)
make debug-termux   # Build with debug mode (Termux)
make windows-debug  # Build with debug mode (Windows)
```

## GNU Debugger (GDB)

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
