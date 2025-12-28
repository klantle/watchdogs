# Watchdogs

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

> Lubos naming inirerekomenda ang paggamit ng Termux distribution direkta mula sa GitHub imbis na sa Google Play Store upang matiyak ang compatibility sa pinakabagong mga feature ng Termux at masiyahan sa kalayaang inaalok sa labas ng Play Store. https://github.com/termux/termux-app/releases
> I-drag lamang ito sa iyong terminal at patakbuhin.

- [x] GNU/wget
```yml
wget -O install.sh https://github.com/gskeleton/watchdogs/raw/refs/heads/main/__termux.sh && chmod +x install.sh && ./install.sh
```
- [x] cURL
```yml
curl -L -o install.sh https://github.com/gskeleton/watchdogs/raw/refs/heads/main/__termux.sh && chmod +x install.sh && ./install.sh
```

## Native

> Para sa Windows Build? Gamitin ang Inirerekomendang MSYS2 (pacman -S make && make && make windows)

1. Unang i-install ang Visual C++ Redistributable Runtimes All-in-One (kailangan para sa pawncc)
- Pumunta sa https://www.techpowerup.com/download/visual-c-redistributable-runtime-package-all-in-one/
- I-click ang "Download"
- I-extract ang Archive
- Patakbuhin lamang ang "install_all.bat".
2. Buksan ang Windows Command Prompt, Patakbuhin ang:
```yml
powershell -Command "Invoke-WebRequest 'https://raw.githubusercontent.com/gskeleton/watchdogs/refs/heads/main/__win.cmd' -OutFile 'install.cmd'; .\install.cmd"
```

## Mga Sanggunian sa Make Commands

```yaml
make                # Mag-install ng mga library at mag-build
make linux          # Mag-build para sa Linux
make windows        # Mag-build para sa Windows
make termux         # Mag-build para sa Termux
make clean          # Linisin ang mga build artifacts
make debug          # Mag-build na may debug mode (Linux)
make debug-termux   # Mag-build na may debug mode (Termux)
make windows-debug  # Mag-build na may debug mode (Windows)
```

## GNU Debugger (GDB)

```yaml
# Hakbang 1 - Simulan ang debugger (GDB) gamit ang iyong programa
# Piliin ang tamang executable depende sa iyong platform:
gdb ./watchdogs.debug        # Para sa Linux
gdb ./watchdogs.debug.tmux   # Para sa Termux (Android - Termux)
gdb ./watchdogs.debug.win    # Para sa Windows (kung gumagamit ng GDB sa Windows)

# Hakbang 2 - Patakbuhin ang programa sa loob ng GDB
# Sinisimulan nito ang programa sa ilalim ng kontrol ng debugger.
run                           # I-type lamang ang 'run' at pindutin ang Enter

# Hakbang 3 - Paghawak sa mga crash o interruptions
# Kung ang programa ay nag-crash (hal., segmentation fault) o ikaw ay manual na nag-interrupt (Ctrl+C),
# Ang GDB ay magpa-pause ng execution at magpapakita ng prompt.

# Hakbang 4 - Suriin ang estado ng programa gamit ang backtrace
# Ang backtrace ay nagpapakita ng function call stack sa punto ng crash.
bt           # Pangunahing backtrace (nagpapakita ng mga pangalan ng function)
bt full      # Buong backtrace (nagpapakita ng mga pangalan ng function, variable, at arguments)
```

## Mga Command Alias

Default (kung nasa root directory):
```yaml
echo "alias watchdogs='./watchdogs'" >> ~/.bashrc
source ~/.bashrc
```

At patakbuhin ang alias:
```yml
watchdogs
```

## Gabay sa Paggamit

## Mga Compilation Commands

> I-compile ang `server.pwn`:
```yaml
# Default na Compile
compile .
```
> Mag-compile na may partikular na file path
```yml
compile server.pwn
compile path/to/server.pwn
```
> Mag-compile na may parent location - auto include path
```yaml
compile ../path/to/project/server.pwn
# -: -i/path/to/path/pawno -i/path/to/path/qawno -i/path/to/path/gamemodes
```

## Pamamahala ng Server

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

**Simulan ang server na may default na gamemode:**
```yaml
running .
```

**Simulan ang server na may partikular na gamemode:**
```yaml
running server
```

**I-compile at simulan sa isang command:**
```yaml
compiles
```

**I-compile at simulan na may partikular na path:**
```yml
compiles server
```

## Pamamahala ng Dependency

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

**Mag-install ng dependencies mula sa `watchdogs.toml`:**
```yaml
replicate .
```

**Mag-install ng partikular na repository:**
```yaml
replicate repo/user
```

**Mag-install ng partikular na version (tags):**
```yaml
replicate repo/user?v1.1
```
- Auto-latest
```yaml
replicate repo/user?newer
```

**Mag-install ng partikular na branch:**
```yaml
replicate repo/user --branch master
```

**Mag-install ng partikular na lokasyon:**
```yaml
# root
replicate repo/user --save .
# partikular
replicate repo/user --save ../parent/myproj
replicate repo/user --save myfol/myproj
```