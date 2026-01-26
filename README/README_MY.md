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

1. **Muat turun Termux dari GitHub**

   * Android 7 ke atas:
     [https://github.com/termux/termux-app/releases/download/v0.119.0-beta.3/termux-app_v0.119.0-beta.3+apt-android-7-github-debug_universal.apk](https://github.com/termux/termux-app/releases/download/v0.119.0-beta.3/termux-app_v0.119.0-beta.3+apt-android-7-github-debug_universal.apk)
   * Android 5/6:
     [https://github.com/termux/termux-app/releases/download/v0.119.0-beta.3/termux-app_v0.119.0-beta.3+apt-android-5-github-debug_universal.apk](https://github.com/termux/termux-app/releases/download/v0.119.0-beta.3/termux-app_v0.119.0-beta.3+apt-android-5-github-debug_universal.apk)

2. **Pasang fail .apk yang telah dimuat turun kemudian jalankan Termux.**

3. **Kali pertama, jalankan arahan berikut dalam Termux:**

> Sila pilih salah satu.

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

> **Tekan Enter tanpa menaip apa-apa.**
> Jika ada soalan lain (contoh: pemilihan mirror Termux `?` (-openssl.cnf (Y/I/N/O/D/Z [default=N] ?)-), pilih yang paling atas atau **tekan Enter sahaja**.

![watchdogs](https://raw.githubusercontent.com/gskeleton/dogdog/refs/heads/main/mirror.png)

4. **Petunjuk Watchdogs berjaya dipasang:**

![watchdogs](https://raw.githubusercontent.com/gskeleton/dogdog/refs/heads/main/indicate.png)

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

> Jika melihat simbol `>` **cukup tekan Enter** kecuali diminta jawapan tertentu (contoh: apply pawncc = yes).

> Untuk langkah kompilasi, pelajari: [here](#compilation-commands--with-parent-directory-in-termux)

---

## Windows Native

> **Bina untuk Windows?** Gunakan **MSYS2** (disyorkan).

1. **Pasang Visual C++ Redistributable Runtimes (wajib untuk pawncc)**

   * Lawati: [https://www.techpowerup.com/download/visual-c-redistributable-runtime-package-all-in-one/](https://www.techpowerup.com/download/visual-c-redistributable-runtime-package-all-in-one/)
   * Klik **Download**
   * Ekstrak arkib
   * Jalankan `install_all.bat`

2. **Buka Command Prompt Windows, jalankan:**

```yaml
powershell -Command "Invoke-WebRequest 'https://raw.githubusercontent.com/gskeleton/watchdogs/refs/heads/main/__windows.cmd' -OutFile 'install.cmd'; .\install.cmd"
```

---

## Rujukan Arahan Make

```yaml
make                # Pasang pustaka dan bina
make linux          # Bina untuk Linux
make windows        # Bina untuk Windows
make termux         # Bina untuk Termux
make clean          # Bersihkan hasil binaan
make debug          # Bina dengan mod debug (Linux)
make debug-termux   # Bina dengan mod debug (Termux)
make windows-debug  # Bina dengan mod debug (Windows)
```

---

## GNU Debugger (GDB)

```yaml
# Langkah 1 - Jalankan debugger (GDB) dengan program
# Pilih executable mengikut platform:
gdb ./watchdogs.debug        # Untuk Linux
gdb ./watchdogs.debug.tmux   # Untuk Termux (Android)
gdb ./watchdogs.debug.win    # Untuk Windows (jika menggunakan GDB)

# Langkah 2 - Jalankan program dalam GDB
# Program dijalankan di bawah kawalan debugger
run                           # taip 'run' kemudian Enter

# Langkah 3 - Tangani crash atau gangguan
# Jika program crash (contoh: segmentation fault) atau dihentikan manual (Ctrl+C),
# GDB akan menghentikan eksekusi dan memaparkan prompt.

# Langkah 4 - Semak status program dengan backtrace
# Backtrace memaparkan urutan panggilan fungsi semasa crash.
bt           # Backtrace asas (nama fungsi)
bt full      # Backtrace lengkap (fungsi, pembolehubah, hujah)
```

---

## Execute dengan arguments

```yaml
./watchdogs command
./watchdogs command args
./watchdogs help compile
```

## Alias Arahan

**Default (jika berada di direktori root):**

```yaml
echo "alias watchdogs='./watchdogs'" >> ~/.bashrc
source ~/.bashrc
```

**Jalankan alias:**

```yaml
watchdogs
```

---

## Kompilasi

> Anda tidak memerlukan pemasangan Watchdogs dalam folder GameMode secara khusus atau di kawasan ~/Downloads. Anda hanya perlu memastikan folder yang mengandungi binari watchdogs seperti watchdogs atau watchdogs.tmux berada dalam folder di Downloads, dan folder projek anda juga berada dalam folder di Downloads. (*INI TIDAK BERGUNA UNTUK watchdogs.win)
```yml
# Struktur contoh:
Downloads
├── dog
│   ├── watchdogs
└── myproj
    └── gamemodes
        └── proj.p
        # ^ maka anda boleh menjalankan watchdogs yang ada dalam folder dog/
        # ^ dan anda hanya perlu mengkompilasinya dengan simbol induk seperti berikut
        # ^ compile ../myproj/gamemodes/proj.p
        # ^ lokasi ini hanyalah contoh.
```

## Arahan Kompilasi – Dengan Direktori Parent dalam Termux

```yaml
compile ../storage/downloads/_NAMA_FOLDER_GAMEMODE_/gamemodes/_NAMA_FAIL_PAWN_.pwn
```

**Contoh:**
Saya ada folder gamemode bernama `parent` dalam Downloads (melalui ZArchiver), dan fail utama `pain.pwn` berada di dalam `gamemodes/`.
Maka laluan yang digunakan ialah:

```yaml
compile ../storage/downloads/parent/pain.pwn
```

---

## Arahan Kompilasi – Umum

> **Kompilasi `server.pwn`:**

> Peraturan asas
```yaml
compile
```

![watchdogs](https://raw.githubusercontent.com/gskeleton/dogdog/refs/heads/main/compile.png)

```yaml
# Kompilasi default
compile .
compile.
```

> **Kompilasi dengan laluan spesifik**

```yaml
compile server.pwn
compile path/to/server.pwn
```

> **Kompilasi dengan lokasi parent (laluan include automatik)**

```yaml
compile ../path/to/project/server.pwn
# automatik: -i/path/to/path/pawno -i/path/to/path/qawno -i/path/to/path/gamemodes
```

---

## Pengurusan Pelayan

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
|                  |     |    jika args wujud     |                -
--------------------     --------------------------                -
```

**Jalankan pelayan dengan gamemode default:**

```yaml
running .
running.
```

**Jalankan pelayan dengan gamemode tertentu:**

```yaml
running server
```

**Kompilasi dan jalankan serentak:**

```yaml
compiles .
compiles.
```

**Kompilasi dan jalankan dengan laluan tertentu:**

```yaml
compiles server
```

---

## Pengurusan Dependency

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

**Pasang dependency dari `watchdogs.toml`:**

```yaml
replicate .
replicate.
```

**Pasang repositori tertentu:**

```yaml
replicate repo/user
```

**Pasang versi tertentu (tag):**

```yaml
replicate repo/user?v1.1
```

* **Versi terbaru automatik**

```yaml
replicate repo/user?newer
```

**Pasang branch tertentu:**

```yaml
replicate repo/user --branch master
```

**Pasang ke lokasi tertentu:**

```yaml
# root
replicate repo/user --save .
# lokasi spesifik
replicate repo/user --save ../parent/myproj
replicate repo/user --save myfolder/myproj
```