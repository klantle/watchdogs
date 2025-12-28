<h1 style="text-align:center;">Watchdogs</h1>

## Linux

- [x] GNU/wget
```yml
wget -O install.sh https://github.com/gskeleton/watchdogs/raw/refs/heads/main/__linux.sh && chmod +x install.sh && ./install.sh
```
- [x] cURL
```yml
curl -L -o install.sh https://github.com/gskeleton/watchdogs/raw/refs/heads/main/__linux.sh && chmod +x install.sh && ./install.sh
```

## Termux

> We highly recommend using the Termux distribution directly from GitHub instead of the Google Play Store to ensure compatibility with the latest Termux features and to enjoy the freedom offered outside the Play Store. https://github.com/termux/termux-app/releases
> Just drag this into your terminal and run it.

- [x] GNU/wget
```yml
wget -O install.sh https://github.com/gskeleton/watchdogs/raw/refs/heads/main/__termux.sh && chmod +x install.sh && ./install.sh
```
- [x] cURL
```yml
curl -L -o install.sh https://github.com/gskeleton/watchdogs/raw/refs/heads/main/__termux.sh && chmod +x install.sh && ./install.sh
```

## Native

> For Windows Build? Use MSYS2 Recommended (pacman -S make && make && make windows)

1. Installing Visual C++ Redistributable Runtimes All-in-One first (needed for pawncc)
- Go to https://www.techpowerup.com/download/visual-c-redistributable-runtime-package-all-in-one/
- Click the "Download"
- Extract the Archive
- Just run the "install_all.bat".
2. Open Windows Command Prompt, Run:
```yml
powershell -Command "Invoke-WebRequest 'https://raw.githubusercontent.com/gskeleton/watchdogs/refs/heads/main/__win.cmd' -OutFile 'install.cmd'; .\install.cmd"
```

## Make Commands Reference

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
> Compile with parent location - auto include path
```yaml
compile ../path/to/project/server.pwn
# -: -i/path/to/path/pawno -i/path/to/path/qawno -i/path/to/path/gamemodes
```

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

**Install specific location:**
```yaml
# root
replicate repo/user --save .
# specific
replicate repo/user --save ../parent/myproj
replicate repo/user --save myfol/myproj
```


