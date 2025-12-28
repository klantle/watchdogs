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

> Sangat disarankan menggunakan distribusi Termux langsung dari GitHub, bukan dari Google Play Store, untuk memastikan kompatibilitas dengan fitur Termux terbaru dan menikmati kebebasan yang ditawarkan di luar Play Store. https://github.com/termux/termux-app/releases
> Cukup taruh perintah ini ke terminal dan jalankan.

- [x] GNU/wget
```yml
wget -O install.sh https://github.com/gskeleton/watchdogs/raw/refs/heads/main/__termux.sh && chmod +x install.sh && ./install.sh
```
- [x] cURL
```yml
curl -L -o install.sh https://github.com/gskeleton/watchdogs/raw/refs/heads/main/__termux.sh && chmod +x install.sh && ./install.sh
```

## Native

> Untuk Build Windows? Gunakan MSYS2 yang Direkomendasikan (pacman -S make && make && make windows)

1. Instal Visual C++ Redistributable Runtimes All-in-One terlebih dahulu (diperlukan untuk pawncc)
- Kunjungi https://www.techpowerup.com/download/visual-c-redistributable-runtime-package-all-in-one/
- Klik "Download"
- Ekstrak arsipnya
- Jalankan saja "install_all.bat".
2. Buka Command Prompt Windows, Jalankan:
```yml
powershell -Command "Invoke-WebRequest 'https://raw.githubusercontent.com/gskeleton/watchdogs/refs/heads/main/__win.cmd' -OutFile 'install.cmd'; .\install.cmd"
```

## Referensi Perintah Make

```yaml
make                # Instal pustaka dan build
make linux          # Build untuk Linux
make windows        # Build untuk Windows
make termux         # Build untuk Termux
make clean          # Bersihkan artefak build
make debug          # Build dengan mode debug (Linux)
make debug-termux   # Build dengan mode debug (Termux)
make windows-debug  # Build dengan mode debug (Windows)
```

## GNU Debugger (GDB)

```yaml
# Langkah 1 - Mulai debugger (GDB) dengan program Anda
# Pilih eksekutabel yang benar sesuai platform Anda:
gdb ./watchdogs.debug        # Untuk Linux
gdb ./watchdogs.debug.tmux   # Untuk Termux (Android - Termux)
gdb ./watchdogs.debug.win    # Untuk Windows (jika menggunakan GDB di Windows)

# Langkah 2 - Jalankan program di dalam GDB
# Ini memulai program di bawah kendali debugger.
run                           # Ketik saja 'run' dan tekan Enter

# Langkah 3 - Menangani crash atau interupsi
# Jika program crash (misal: segmentation fault) atau Anda menghentikannya secara manual (Ctrl+C),
# GDB akan menghentikan eksekusi dan menampilkan prompt.

# Langkah 4 - Periksa status program dengan backtrace
# Backtrace menunjukkan stack panggilan fungsi pada titik crash.
bt           # Backtrace dasar (menunjukkan nama fungsi)
bt full      # Backtrace lengkap (menunjukkan nama fungsi, variabel, dan argumen)
```

## Alias Perintah

Default (jika di direktori root):
```yaml
echo "alias watchdogs='./watchdogs'" >> ~/.bashrc
source ~/.bashrc
```

Dan jalankan aliasnya:
```yml
watchdogs
```

## Panduan Penggunaan

## Perintah Kompilasi

> Kompilasi `server.pwn`:
```yaml
# Kompilasi Default
compile .
```
> Kompilasi dengan jalur file spesifik
```yml
compile server.pwn
compile path/ke/server.pwn
```
> Kompilasi dengan lokasi induk - jalur include otomatis
```yaml
compile ../path/ke/project/server.pwn
# -: -i/path/ke/path/pawno -i/path/ke/path/qawno -i/path/ke/path/gamemodes
```

## Manajemen Server

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

**Mulai server dengan gamemode default:**
```yaml
running .
```

**Mulai server dengan gamemode spesifik:**
```yaml
running server
```

**Kompilasi dan mulai dalam satu perintah:**
```yaml
compiles
```

**Kompilasi dan mulai dengan jalur spesifik:**
```yml
compiles server
```

## Manajemen Dependensi

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

**Instal dependensi dari `watchdogs.toml`:**
```yaml
replicate .
```

**Instal repositori spesifik:**
```yaml
replicate repo/user
```

**Instal versi spesifik (tags):**
```yaml
replicate repo/user?v1.1
```
- Otomatis terbaru
```yaml
replicate repo/user?newer
```

**Instal branch spesifik:**
```yaml
replicate repo/user --branch master
```

**Instal lokasi spesifik:**
```yaml
# root
replicate repo/user --save .
# spesifik
replicate repo/user --save ../parent/myproj
replicate repo/user --save myfol/myproj
```