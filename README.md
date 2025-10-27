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

---

### Arch Support
- [x] [Qualcomm Snapdragon](https://en.wikipedia.org/wiki/Qualcomm_Snapdragon)
- [x] [MediaTek](https://en.wikipedia.org/wiki/MediaTek)
- [x] [Intel](https://en.wikipedia.org/wiki/Intel)
- [x] [AMD](https://en.wikipedia.org/wiki/AMD)

### [DOCKER](https://www.docker.com/) USAGE - for Mac/ALT Platform

> NOTE
Run this inside the Docker/dockerfile directory.

- For Linux setup:
```sh
sudo usermod -aG docker $USER
newgrp docker
sudo systemctl enable docker
sudo systemctl start docker
```

- Build and run:
```sh
docker build -t docker_usage .
docker run --rm -it docker_usage
```

- Additional Docker Commands
```sh
docker ps -a                               # check a container
docker start <container-name>              # start a container
docker exec -it <container-name> /bin/bash # enter a container
docker stop <container-name>               # stop a container
docker rm -f <container-name>              # remove a container
```
- build & install Watchdogs - [click here](https://github.com/klantle/watchdogs?tab=readme-ov-file#bash---linux-usage)

### [TERMUX](https://github.com/termux/termux-app/releases) USAGE
- note: Please use termux from https://github.com/termux/termux-app/releases
```sh
# @@@ 1
termux-setup-storage       # setup your storage
# @@@ 2
termux-change-repo         # change repo
# @@@ 3
apt update                 # sync repo
# @@@ 4
apt upgrade                # upgrade - optional
# @@@ 5
pkg install make git
# @@@ 6
git clone https://github.com/klantle/watchdogs watch
# @@@ 7
cd watch
# @@@ 8
make                       # install library & build from source
# @@@ 9
./watchdogs.tmux           # run
```

### [MSYS2](https://www.msys2.org) USAGE
- note: Please see https://www.msys2.org for introduction of MSYS2
```sh
# @@@ 1
pacman -Sy                 # sync repo
# @@@ 2 - add more mirror (if failed in install packages)
nano /etc/pacman.d/mirrorlist.mingw64
nano /etc/pacman.d/mirrorlist.msys
nano /etc/pacman.d/mirrorlist.ucrt64
# -- nano; CTRL + X & ENTER
Server = https://repo.msys2.org/msys/$arch
Server = https://mirror.msys2.org/msys/$arch
Server = https://mirror.selfnet.de/msys2/msys/$arch
Server = https://mirrors.rit.edu/msys2/msys/$arch
Server = https://ftp.osuosl.org/pub/msys2/msys/$arch
# sync mirror
pacman -Sy
# @@@ 3
pacman -S make git
# @@@ 4
git clone https://github.com/klantle/watchdogs watch
# @@@ 5
cd watch
# @@@ 6
make                      # install library & build from source
# @@@ 7
./watcdogs.win            # run
```

### [BASH - LINUX](https://en.wikipedia.org/wiki/Bash_(Unix_shell)) USAGE
- note: this only in debian/ubuntu based
```sh
# @@@ 1
apt update                # sync repo
# @@@ 2
apt install make git
# @@@ 3
git clone https://github.com/klantle/watchdogs watch
# @@@ 4
cd watch
# @@@ 5
make                      # install library & build from source
# @@@ 6
./watchdogs               # run
```

### [XTERM](https://invisible-island.net/xterm/) USAGE
```sh
xterm -hold -e ./watchdogs             # linux
xterm -hold -e ./watchdogs.win         # windows
xterm -hold -e ./watchdogs.tmux        # termux
```

### Base Make command
```sh
make                # install library (apt/pkg/msys2)
make linux          # build from source for linux
make windows        # build from source for windows (msys2)
make termux         # build from source for termux
make clean          # clean make
make debug          # build from source with debug mode (linux)
make windows-debug  # build from source with debug mode (windows)
# `./__make.sh` for more.
```

---
![image](https://gitlab.com/mywatchdogs/watchdogs/-/raw/main/IMG/__PATH.png)

---

### BASIC OPERATION
```sh
Usage: help | help [<command>]
```

---

### `watchdogs.toml` Struct
```toml
[general]
	os = "linux"                   # os type
[compiler]
	option = ["-d3", "-;+", "-(+"] # compiler options
	include_path = [
      # folders that are also automatically added
      "gamemodes",
      "gamemodes/x", 
      "gamemodes/y", 
      "gamemodes/z", 
      "pawno/include",
      "pawno/include/x",
      "pawno/include/y",
      "pawno/include/z",
  ]
  input = "gamemodes/bare.pwn"   # input compiler
  output = "gamemodes/bare.amx"  # output compiler
[depends]                        # depends url - max depends: 101
	aio_repo = ["Y-Less/sscanf:v2.13.8", "samp-incognito/samp-streamer-plugin:v2.9.6"]
```

### [VSCODE Usage](https://code.visualstudio.com/docs/debugtest/tasks)
```json
{
  "version": "2.0.0",
  "tasks": [
    {
      "label": "watchdogs", "type": "shell",
      "command": "${workspaceRoot}/watchdogs",
      "group": {
        "kind": "build", "isDefault": true
      }
    }
  ]
}
```
WSL
```json
{
  "version": "2.0.0",
  "tasks": [
    {
      "label": "watchdogs", "type": "shell",
      "command": "wsl ${workspaceRoot}/watchdogs",
      "group": {
        "kind": "build", "isDefault": true
      }
    }
  ]
}
```

---

#### INTRO 1.1 (INSTALL)
Supported Platforms: Linux, Windows, or macOS via:
- [Docker Container](https://www.docker.com/)
- [WSL or WSL2](https://learn.microsoft.com/en-us/windows/wsl/install)
- [MSYS2](https://www.msys2.org/)
- [Desktop environment](https://en.m.wikipedia.org/wiki/Desktop_environment)
- [VPS (Virtual Private Server)](https://en.m.wikipedia.org/wiki/Virtual_private_server)
- [Termux (Android)](https://github.com/termux/termux-app)
- [xterm](https://invisible-island.net/xterm/)
- and so on, based on an environment that can run GNU/Bash.

---

Alias (running a program with a phrase.)
```
alias watch='./path/to/watchdogs'
```
Default (on root):
```
alias watch='./watchdogs'
```
run: `~$: watch`

---

### INTRO 1.2 (COMPILATION [¶](https://en.m.wikipedia.org/wiki/Compilation))

Default compilation:
```
compile .
```
![image](https://gitlab.com/mywatchdogs/watchdogs/-/raw/main/IMG/__C.png)

Compile specific file:
```
compile yourmode.pwn
```

Compile specific path:
```
compile path/to/yourmode.pwn
```

### INTRO 1.3 (STARTING [¶](https://en.m.wikipedia.org/wiki/Run_command))

Run yourmode of server.cfg/config.json from parameter `gamemode0` or `main_scripts`
```
running .
```
![image](https://gitlab.com/mywatchdogs/watchdogs/-/raw/main/IMG/__R.png)

Running specific `".amx"` with `[<args>]`
```
running yourmode
```

### INTRO 1.4 (COMPILE & STARTING)

One Step - (o)basic compile & starting:
```
crunn
```
![image](https://gitlab.com/mywatchdogs/watchdogs/-/raw/main/IMG/__CR.png)

### INTRO 1.5 (DEBUGGING [¶](https://en.m.wikipedia.org/wiki/Debugging))

Debug yourmode of server.cfg/config.json from parameter `gamemode0` or `main_scripts`
```
debug .
```
![image](https://gitlab.com/mywatchdogs/watchdogs/-/raw/main/IMG/__D.png)

Debug specific `".amx"` with `[<args>]`
```
debug yourmode
```

### INTRO 1.6 (DEPENDS INSTALLER)

```sh
install                 # automatic installing from watchdogs.toml
install repo/user       # installing depends from (github/gitlab/gitea/sourceforge) by input
install repo/user:v1.1  # installing depends from (github/gitlab/gitea/sourceforge) by input with specific tags version
```
![image](https://gitlab.com/mywatchdogs/watchdogs/-/raw/main/IMG/__DEP.png)

### INTRO 1.7 (COMPILER OPTIONS) [3.10.10](https://github.com/pawn-lang/compiler/releases/tag/v3.10.10) (^/v)

| Option          | Description                                                        |
|-----------------|--------------------------------------------------------------------|
| `-A<num>`       | Alignment in bytes of the data segment and the stack               |
| `-a`            | Output assembler code                                              |
| `-C[+/-]`       | Compact encoding for output file (default=+)                       |
| `-c<name>`      | Codepage name or number; e.g. 1252 for Windows Latin-1             |
| `-Dpath`        | Active directory path                                              |
| `-d<num>`       | Debugging level (default=-d1):                                     |
|                 | `0` no symbolic information, no run-time checks                    |
|                 | `1` run-time checks, no symbolic information                       |
|                 | `2` full debug information and dynamic checking                    |
|                 | `3` same as -d2, but implies `-O0`                                 |
| `-e<name>`      | Set name of error file (quiet compile)                             |
| `-H<hwnd>`      | Window handle to send a notification message on finish             |
| `-i<name>`      | Path for include files                                             |
| `-l`            | Create list file (preprocess only)                                 |
| `-o<name>`      | Set base name of (P-code) output file                              |
| `-O<num>`       | Optimization level (default=-O1):                                  |
|                 | `0` no optimization                                                |
|                 | `1` JIT-compatible optimizations only                              |
|                 | `2` full optimizations                                             |
| `-p<name>`      | Set name of "prefix" file                                          |
| `-R[+/-]`       | Add detailed recursion report with call chains (default=-)         |
| `-r[name]`      | Write cross reference report to console or to specified file       |
| `-S<num>`       | Stack/heap size in cells (default=4096)                            |
| `-s<num>`       | Skip lines from the input file                                     |
| `-t<num>`       | TAB indent size (in character positions, default=8)                |
| `-v<num>`       | Verbosity level; `0=quiet`, `1=normal`, `2=verbose` (default=1)    |
| `-w<num>`       | Disable a specific warning by its number                           |
| `-X<num>`       | Abstract machine size limit in bytes                               |
| `-XD<num>`      | Abstract machine data/stack size limit in bytes                    |
| `-Z[+/-]`       | Run in compatibility mode (default=-)                              |
| `-E[+/-]`       | Turn warnings into errors                                          |
| `-\`            | Use '`\`' for escape characters                                    |
| `-^`            | Use '^' for escape characters                                      |
| `-;[+/-]`       | Require a semicolon to end each statement (default=-)              |
| `-([+/-]`       | Require parentheses for function invocation (default=-)            |
| `sym=val`       | Define constant `sym` with value `val`                             |
| `sym=`          | Define constant `sym` with value 0                                 |
