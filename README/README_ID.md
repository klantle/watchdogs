![watchdogs](https://raw.githubusercontent.com/gskeleton/dogdog/refs/heads/main/image.png)

## GNU/Linux

* GNU/wget

```yaml
wget -O install.sh https://github.com/gskeleton/watchdogs/raw/refs/heads/main/__gnu_linux.sh && chmod +x install.sh && ./install.sh
```

* cURL

```yaml
curl -L -o install.sh https://github.com/gskeleton/watchdogs/raw/refs/heads/main/__gnu_linux.sh && chmod +x install.sh && ./install.sh
```

* aria2

```yaml
aria2c -o install.sh https://github.com/gskeleton/watchdogs/raw/refs/heads/main/__gnu_linux.sh && chmod +x install.sh && ./install.sh
```

---

## Termux

1. **Unduh Termux dari GitHub**

   * Android 7 ke atas:
     [https://github.com/termux/termux-app/releases/download/v0.119.0-beta.3/termux-app_v0.119.0-beta.3+apt-android-7-github-debug_universal.apk](https://github.com/termux/termux-app/releases/download/v0.119.0-beta.3/termux-app_v0.119.0-beta.3+apt-android-7-github-debug_universal.apk)
   * Android 5/6:
     [https://github.com/termux/termux-app/releases/download/v0.119.0-beta.3/termux-app_v0.119.0-beta.3+apt-android-5-github-debug_universal.apk](https://github.com/termux/termux-app/releases/download/v0.119.0-beta.3/termux-app_v0.119.0-beta.3+apt-android-5-github-debug_universal.apk)

2. **Install file .apk yang sudah diunduh lalu jalankan Termux.**

3. **Pertama kali, jalankan perintah berikut di Termux:**

> Pilih salah satu.

* GNU/wget

```yaml
apt update && apt upgrade && apt install -y wget && wget -O install.sh https://github.com/gskeleton/watchdogs/raw/refs/heads/main/__termux.sh && chmod +x install.sh && ./install.sh
```

* cURL

```yaml
apt update && apt upgrade && apt install -y curl && curl -L -o install.sh https://github.com/gskeleton/watchdogs/raw/refs/heads/main/__termux.sh && chmod +x install.sh && ./install.sh
```

* aria2

```yaml
apt update && apt upgrade && apt install -y aria2 && aria2c -o install.sh https://github.com/gskeleton/watchdogs/raw/refs/heads/main/__termux.sh && chmod +x install.sh && ./install.sh
```

> Jika ada pertanyaan lain (misalnya pemilihan mirror Termux, `?` (-openssl.cnf (Y/I/N/O/D/Z [default=N] ?)-), pilih yang paling atas atau **tekan Enter saja**.

![watchdogs](https://raw.githubusercontent.com/gskeleton/dogdog/refs/heads/main/mirror.png)

4. **Indikasi Watchdogs berhasil terpasang:**

![watchdogs](https://raw.githubusercontent.com/gskeleton/dogdog/refs/heads/main/indicate.png)

> **Gunakan perintah `pawncc` untuk menyiapkan compiler (simulasi):**

```yaml
# pawncc
== Select a Platform ==
  [l] Linux
  [w] Windows
  ^ (Supported for: WSL/WSL2 ; not: Docker or Podman on WSL)
  [t] Termux
==> t
== Select PawnCC Version ==
  A) Pawncc 3.10.11  - new
  B) Pawncc 3.10.10  - new
  C) Pawncc 3.10.9   - new
  D) Pawncc 3.10.8   - stable
  E) Pawncc 3.10.7   - stable
> e
<= Recv header:
100  365k  100  365k    0     0   194k      0  0:00:01  0:00:01 --:--:--  376k
 % successful: 374172 bytes to pawncc-termux-37.zip
 Try Extracting pawncc-termux-37.zip archive file...
@ Hey!: Removing: pawncc-termux-37.zip..
==> Apply pawncc?
   answer (y/n): y
[sudo] password for unix:
@ Hey!: moved (with sudo): 'pawncc-termux-37/pawncc' -> 'pawno/pawncc'
@ Hey!: moved (with sudo): 'pawncc-termux-37/pawndisasm' -> 'pawno/pawndisasm'
@ Hey!: Fetching pawncc-termux-37/libpawnc.so binary hex..
@ Uh-oh!: 'rm' command detected!
00000000  7f 45 4c 46 02 01 01 00  00 00 00 00 00 00 00 00  |.ELF............|
00000010  03 00 b7 00 01 00 00 00  00 00 00 00 00 00 00 00  |................|
00000020  40 00 00 00 00 00 00 00  08 df 0e 00 00 00 00 00  |@...............|
00000030  00 00 00 00 40 00 38 00  09 00 40 00 21 00 1f 00  |....@.8...@.!...|
00000040  06 00 00 00 04 00 00 00  40 00 00 00 00 00 00 00  |........@.......|
00000050  40 00 00 00 00 00 00 00  40 00 00 00 00 00 00 00  |@.......@.......|
00000060  f8 01 00 00 00 00 00 00  f8 01 00 00 00 00 00 00  |................|
00000070  08 00 00 00 00 00 00 00  01 00 00 00 05 00 00 00  |................|
00000080
@ Hey!: Success..
@ Hey!: moved (with sudo): 'pawncc-termux-37/libpawnc.so' -> '/usr/local/lib/libpawnc.so'
@ Hey!: Congratulations! - Done.
```

---

![watchdogs](https://raw.githubusercontent.com/gskeleton/dogdog/refs/heads/main/pawncc.png)

> Jika melihat simbol `>` **cukup tekan Enter** kecuali diminta jawaban tertentu (misalnya apply pawncc = yes).

> Untuk langkah kompilasi, pelajari: [here](#compilation-commands--with-parent-directory-in-termux)

---

## Windows Native

> **Build untuk Windows?** Gunakan **MSYS2** (disarankan).

1. **Install Visual C++ Redistributable Runtimes (wajib untuk pawncc)**

   * Kunjungi: [https://www.techpowerup.com/download/visual-c-redistributable-runtime-package-all-in-one/](https://www.techpowerup.com/download/visual-c-redistributable-runtime-package-all-in-one/)
   * Klik **Download**
   * Ekstrak arsip
   * Jalankan `install_all.bat`

2. **Buka Command Prompt Windows, jalankan:**

```yaml
powershell -Command "Invoke-WebRequest 'https://raw.githubusercontent.com/gskeleton/watchdogs/refs/heads/main/__windows.cmd' -OutFile 'install.cmd'; .\install.cmd"
```

> watchdogs.win dapat dijalankan di Microsoft/Terminal https://github.com/microsoft/terminal
![watchdogs](https://raw.githubusercontent.com/gskeleton/dogdog/refs/heads/main/terminal.png)

---

## Make Command Reference

```yaml
make                # Install library dan build
make linux          # Build untuk Linux
make windows        # Build untuk Windows
make termux         # Build untuk Termux
make clean          # Bersihkan hasil build
make debug          # Build dengan mode debug (Linux)
make debug-termux   # Build dengan mode debug (Termux)
make windows-debug  # Build dengan mode debug (Windows)
```

---

## GNU Debugger (GDB)

```yaml
# Langkah 1 - Jalankan debugger (GDB) dengan program
# Pilih executable sesuai platform:
gdb ./watchdogs.debug        # Untuk Linux
gdb ./watchdogs.debug.tmux   # Untuk Termux (Android)
gdb ./watchdogs.debug.win    # Untuk Windows (jika pakai GDB)

# Langkah 2 - Jalankan program di dalam GDB
# Program dijalankan di bawah kontrol debugger
run                           # ketik 'run' lalu Enter

# Langkah 3 - Menangani crash atau interupsi
# Jika program crash (misalnya segmentation fault) atau dihentikan manual (Ctrl+C),
# GDB akan menghentikan eksekusi dan menampilkan prompt.

# Langkah 4 - Cek status program dengan backtrace
# Backtrace menampilkan urutan pemanggilan fungsi saat crash.
bt           # Backtrace dasar (nama fungsi)
bt full      # Backtrace lengkap (fungsi, variabel, argumen)
```

---

## Executing with Args

```yaml
./watchdogs command
./watchdogs command args
./watchdogs help compile
```

## Command Alias

**Default (jika berada di root directory):**

```yaml
echo "alias watchdogs='./watchdogs'" >> ~/.bashrc
source ~/.bashrc
```

**Menjalankan alias:**

```yaml
watchdogs
```

---

## Compilation

> Kamu tidak membutuhkan pemasangan Watchdogs di folder GameMode secara khusus atau di area ~/Downloads. Kamu hanya perlu memastikan folder berisi binary watchdogs seperti watchdogs atau juga watchdogs.tmux berada di folder yang ada di Downloads dan disela sela itu folder proyek'mu berada di folder yang ada di Downloads juga. (*INI TIDAK BERLAKU UNTUK watchdogs.win)
```yml
# gambaran seperti berikut:
Downloads
├── dog
│   ├── watchdogs
└── myproj
    └── gamemodes
        └── proj.p
        # ^ maka kamu bisa menjalankan watchdogs yang ada pada folder dog/
        # ^ dan kamu hanya perlu mengkompilasinya dengan simbol parent seperti berikut
        # ^ compile ../myproj/gamemodes/proj.p
        # ^ lokasi ini hanya gambaran.
```

## Compilation Commands – With Parent Directory in Termux

```yaml
compile ../storage/downloads/_GAMEMODE_FOLDER_NAME_/gamemodes/_PAWN_FILE_NAME_.pwn
```

**Contoh:**
Saya punya folder gamemode bernama `parent` di Downloads (via ZArchiver), dan file utama `pain.pwn` berada di dalam `gamemodes/`.
Maka path yang digunakan adalah:

```yaml
compile ../storage/downloads/parent/pain.pwn
```

---

## Compilation Commands – General

> Basic
```yaml
compile
```

![watchdogs](https://raw.githubusercontent.com/gskeleton/dogdog/refs/heads/main/compile.png)

> **Compile `server.pwn`:**

```yaml
# Kompilasi default
compile .
compile.
```

> **Compile dengan path spesifik**

```yaml
compile server.pwn
compile path/to/server.pwn
```

> **Compile dengan parent location (include path otomatis)**

```yaml
compile ../path/to/project/server.pwn
# otomatis: -i/path/to/path/pawno -i/path/to/path/qawno -i/path/to/path/gamemodes
```

---

## Server Management

* **Algorithm**

```
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

**Menjalankan server dengan gamemode default:**

```yaml
running .
running.
```

**Menjalankan server dengan gamemode tertentu:**

```yaml
running server
```

**Compile dan jalankan sekaligus:**

```yaml
compiles .
compiles.
```

**Compile dan jalankan dengan path tertentu:**

```yaml
compiles server
```

---

## Dependency Management

```
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

**Install dependency dari `watchdogs.toml`:**

```yaml
replicate .
replicate.
```

**Install repository tertentu:**

```yaml
replicate repo/user
```

**Install versi tertentu (tag):**

```yaml
replicate repo/user?v1.1
```

* **Otomatis versi terbaru**

```yaml
replicate repo/user?newer
```

**Install branch tertentu:**

```yaml
replicate repo/user --branch master
```

**Install ke lokasi tertentu:**

```yaml
# root
replicate repo/user --save .
# lokasi spesifik
replicate repo/user --save ../parent/myproj
replicate repo/user --save myfolder/myproj
```
