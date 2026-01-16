# Watchdogs

![watchdogs](https://raw.githubusercontent.com/gskeleton/dogdog/refs/heads/main/image.png)

## Linux

- GNU/wget
```yaml
apt update && apt upgrade && wget -O install.sh https://github.com/gskeleton/watchdogs/raw/refs/heads/main/__gnu_linux.sh && chmod +x install.sh && ./install.sh
```
- cURL
```yaml
apt update && apt upgrade && curl -L -o install.sh https://github.com/gskeleton/watchdogs/raw/refs/heads/main/__gnu_linux.sh && chmod +x install.sh && ./install.sh
```

## Termux

1. Download Termux from GitHub<br>
\- Android 7 and above:<br>
https://github.com/termux/termux-app/releases/download/v0.119.0-beta.3/termux-app_v0.119.0-beta.3+apt-android-7-github-debug_universal.apk<br>
\- Android 5/6:<br>
https://github.com/termux/termux-app/releases/download/v0.119.0-beta.3/termux-app_v0.119.0-beta.3+apt-android-5-github-debug_universal.apk
2. Install the downloaded .apk file and run Termux.
3. First, run the following command in the installed Termux:
```yaml
curl -L -o install.sh https://github.com/gskeleton/watchdogs/raw/refs/heads/main/__termux.sh && chmod +x install.sh && ./install.sh
```
4. If you encounter a prompt like the following (Simulation):
```yaml
Enter the path you want to switch to location in storage/downloads
  ^ example my_folder my_project my_server
  ^ a folder name for install; the folder doesn't exist?, don't worry..
  ^ enter if you want install the watchdogs in home (recommended)..
```
\> Press Enter without entering anything. If there are any other questions, such as Termux mirror selection, choose the top one or all (just press Enter).
5. An indication that Watchdogs has been successfully installed, as follows:
```diff
  \/%#z.       \/.%#z./       ,z#%\/
   \X##k      /X#####X\      d##X/
    \888\    /888/ \888\    /888/
     `v88;  ;88v'   `v88;  ;88v'
       \77xx77/       \77xx77/
        `::::'         `::::'
Type "help" for more information.
> 
```
\> Use the `pawncc` command to prepare the compilation tool (Simulation):
```yaml
> pawncc
== Select a Platform ==
  [l] Linux
  [w] Windows
  [t] Termux
==> t # select t [termux]
== Select PawnCC Version ==
  A) Pawncc 3.10.11
  B) Pawncc 3.10.10
  C) Pawncc 3.10.9
  D) Pawncc 3.10.8
  E) Pawncc 3.10.7
> a # select compiler version
* Try Downloading ? * Enable HTTP debugging? (y/n): n
 Try Extracting ? archive file...
==> Remove archive ?? (y/n) > n # optional - delete the archive later
==> Apply pawncc?
   answer (y/n) > y # apply pawncc to root - install it into the pawno/ folder; press Enter or enter y and press Enter
>> I:moved without sudo: '?' -> '?'
>> I:moved without sudo: '?' -> '?'
>> I:Congratulations! - Done.
```
\> If you see the `?` (-openssl.cnf (Y/I/N/O/D/Z [default=N] ?) symbol in cyan/gray/blue (visuals may vary), you can just press Enter without entering anything. Except when required, such as apply pawncc? yes.<br>
\> For compilation steps, please study: https://github.com/gskeleton/watchdogs/blob/main/README/README_ID.md#usage-guide

## Native

> To Build for Windows? Use the Recommended MSYS2 (pacman -S make && make && make windows)

1. First, install Visual C++ Redistributable Runtimes All-in-One (required for pawncc)
- Visit https://www.techpowerup.com/download/visual-c-redistributable-runtime-package-all-in-one/
- Click "Download"
- Extract the archive
- Just run "install_all.bat".
2. Open Windows Command Prompt, Run:
```yaml
powershell -Command "Invoke-WebRequest 'https://raw.githubusercontent.com/gskeleton/watchdogs/refs/heads/main/__win.cmd' -OutFile 'install.cmd'; .\install.cmd"
```

## Make Command Reference

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
# Choose the correct executable for your platform:
gdb ./watchdogs.debug        # For Linux
gdb ./watchdogs.debug.tmux   # For Termux (Android - Termux)
gdb ./watchdogs.debug.win    # For Windows (if using GDB on Windows)

# Step 2 - Run the program inside GDB
# This starts the program under the debugger's control.
run                           # Just type 'run' and press Enter

# Step 3 - Handling crashes or interruptions
# If the program crashes (e.g., segmentation fault) or you manually stop it (Ctrl+C),
# GDB will halt execution and display the prompt.

# Step 4 - Check program status with backtrace
# Backtrace shows the function call stack at the crash point.
bt           # Basic backtrace (shows function names)
bt full      # Full backtrace (shows function names, variables, and arguments)
```

## Command Alias

Default (if in root directory):
```yaml
echo "alias watchdogs='./watchdogs'" >> ~/.bashrc
source ~/.bashrc
```

And run the alias:
```yaml
watchdogs
```

## Usage Guide

## Compilation Commands - With Parent Directory in Termux

```yaml
compile ../storage/downloads/_GAMEMODE_FOLDER_NAME_/gamemodes/_PAWN_FILE_NAME_.pwn
```
Example: I have a gamemode folder named "parent" in ZArchiver located precisely in Downloads, and I have the main project file like "pain.pwn" which is inside gamemodes/ within the "parent" folder located in Downloads.
So, I will make the "parent" folder the location after Downloads and "pain.pwn" after gamemodes.
```yaml
compile ../storage/downloads/parent/pain.pwn
```

## Compilation Commands - General

> Compile `server.pwn`:
```yaml
# Default Compilation
compile .
```
> Compile with specific file path
```yaml
compile server.pwn
compile path/to/server.pwn
```
> Compile with parent location - automatic include path
```yaml
compile ../path/to/project/server.pwn
# -: -i/path/to/path/pawno -i/path/to/path/qawno -i/path/to/path/gamemodes
```

## Server Management

* **Algorithm**

```diff
--------------------     --------------------------                -
|                  |     |                        |                -
|       ARGS       | --> |        FILTERING       |                -
|                  |     |                        |                -
--------------------     --------------------------                -
                                     |
                                     v
---------------------    --------------------------                -
|                   |    |                        |                -
|  LOGGING OUTPUT   |    |   EXIST FILE VALIDATE  |                -
|                   |    |                        |                -
---------------------    --------------------------                -
         ^                           |
         |                           v
--------------------     --------------------------                -
|                  |     |                        |                -
|  RUNNING BINARY  | <-- |     EDITING CONFIG     |                -
|                  |     |    if args is exist    |                -
--------------------     --------------------------                -
```

**Start the server with default gamemode:**
```yaml
running .
```

**Start the server with a specific gamemode:**
```yaml
running server
```

**Compile and start in one command:**
```yaml
compiles
```

**Compile and start with a specific path:**
```yaml
compiles server
```

## Dependency Management

* **Algorithm**

```diff
--------------------     --------------------------                -
|                  |     |                        |                -
|     BASE URL     | --> |      URL CHECKING      |                -
|                  |     |                        |                -
--------------------     --------------------------                -
                                    |
                                    v
---------------------    --------------------------                -
|                   |    |                        |                -
|     APPLYING      |    |  PATTERNS - FILTERING  |                -
|                   |    |                        |                -
---------------------    --------------------------                -
         ^                          |
         |                          v
--------------------     --------------------------                -
|                  |     |                        |                -
|  FILES CHECKING  | <-- |       INSTALLING       |                -
|                  |     |                        |                -
--------------------     --------------------------                -
```

**Install dependencies from `watchdogs.toml`:**
```yaml
replicate .
```

**Install a specific repository:**
```yaml
replicate repo/user
```

**Install a specific version (tags):**
```yaml
replicate repo/user?v1.1
```
- Automatically the latest
```yaml
replicate repo/user?newer
```

**Install a specific branch:**
```yaml
replicate repo/user --branch master
```

**Install to a specific location:**
```yaml
# root
replicate repo/user --save .
# specific
replicate repo/user --save ../parent/myproj
replicate repo/user --save myfolder/myproj
```
