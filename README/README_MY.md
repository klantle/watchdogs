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

> Kami sangat mengesyorkan menggunakan distribusi Termux terus dari GitHub berbanding Google Play Store untuk memastikan keserasian dengan ciri Termux terkini dan menikmati kebebasan yang ditawarkan di luar Play Store. https://github.com/termux/termux-app/releases
> Hanya seret ini ke terminal anda dan jalankannya.

- [x] GNU/wget
```yml
wget -O install.sh https://github.com/gskeleton/watchdogs/raw/refs/heads/main/__termux.sh && chmod +x install.sh && ./install.sh
```
- [x] cURL
```yml
curl -L -o install.sh https://github.com/gskeleton/watchdogs/raw/refs/heads/main/__termux.sh && chmod +x install.sh && ./install.sh
```

## Native

> Untuk Windows Build? Gunakan MSYS2 yang Disyorkan (pacman -S make && make && make windows)

1. Pasang Visual C++ Redistributable Runtimes All-in-One dahulu (diperlukan untuk pawncc)
- Pergi ke https://www.techpowerup.com/download/visual-c-redistributable-runtime-package-all-in-one/
- Klik "Download"
- Ekstrak Arkib
- Hanya jalankan "install_all.bat".
2. Buka Windows Command Prompt, Jalankan:
```yml
powershell -Command "Invoke-WebRequest 'https://raw.githubusercontent.com/gskeleton/watchdogs/refs/heads/main/__win.cmd' -OutFile 'install.cmd'; .\install.cmd"
```

## Rujukan Arahan Make

```yaml
make                # Pasang pustaka dan bina
make linux          # Bina untuk Linux
make windows        # Bina untuk Windows
make termux         # Bina untuk Termux
make clean          # Bersihkan artefak binaan
make debug          # Bina dengan mod debug (Linux)
make debug-termux   # Bina dengan mod debug (Termux)
make windows-debug  # Bina dengan mod debug (Windows)
```

## GNU Debugger (GDB)

```yaml
# Langkah 1 - Mulakan debugger (GDB) dengan program anda
# Pilih fail boleh laksana yang betul bergantung pada platform anda:
gdb ./watchdogs.debug        # Untuk Linux
gdb ./watchdogs.debug.tmux   # Untuk Termux (Android - Termux)
gdb ./watchdogs.debug.win    # Untuk Windows (jika menggunakan GDB di Windows)

# Langkah 2 - Jalankan program di dalam GDB
# Ini memulakan program di bawah kawalan debugger.
run                           # Hanya taip 'run' dan tekan Enter

# Langkah 3 - Mengendalikan kerosakan atau gangguan
# Jika program rosak (contoh: segmentation fault) atau anda mengganggunya secara manual (Ctrl+C),
# GDB akan menghentikan pelaksanaan dan menunjukkan prompt.

# Langkah 4 - Periksa keadaan program dengan backtrace
# Backtrace menunjukkan timbunan panggilan fungsi pada titik kerosakan.
bt           # Backtrace asas (menunjukkan nama fungsi)
bt full      # Backtrace penuh (menunjukkan nama fungsi, pembolehubah, dan argumen)
```

## Alias Arahan

Default (jika dalam direktori root):
```yaml
echo "alias watchdogs='./watchdogs'" >> ~/.bashrc
source ~/.bashrc
```

Dan jalankan alias:
```yml
watchdogs
```

## Panduan Penggunaan

## Arahan Kompilasi

> Kompilasi `server.pwn`:
```yaml
# Kompilasi Default
compile .
```
> Kompilasi dengan laluan fail spesifik
```yml
compile server.pwn
compile path/ke/server.pwn
```
> Kompilasi dengan lokasi induk - laluan include automatik
```yaml
compile ../path/ke/project/server.pwn
# -: -i/path/ke/path/pawno -i/path/ke/path/qawno -i/path/ke/path/gamemodes
```

## Pengurusan Server

* **Algoritma**
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

**Mulakan server dengan gamemode default:**
```yaml
running .
```

**Mulakan server dengan gamemode spesifik:**
```yaml
running server
```

**Kompilasi dan mulakan dalam satu arahan:**
```yaml
compiles
```

**Kompilasi dan mulakan dengan laluan spesifik:**
```yml
compiles server
```

## Pengurusan Kebergantungan

* **Algoritma**
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

**Pasang kebergantungan dari `watchdogs.toml`:**
```yaml
replicate .
```

**Pasang repositori spesifik:**
```yaml
replicate repo/user
```

**Pasang versi spesifik (tags):**
```yaml
replicate repo/user?v1.1
```
- Automatik terkini
```yaml
replicate repo/user?newer
```

**Pasang branch spesifik:**
```yaml
replicate repo/user --branch master
```

**Pasang lokasi spesifik:**
```yaml
# root
replicate repo/user --save .
# spesifik
replicate repo/user --save ../parent/myproj
replicate repo/user --save myfol/myproj
```