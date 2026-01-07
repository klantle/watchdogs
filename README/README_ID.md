## Linux

* GNU/wget

```yaml
wget -O install.sh https://github.com/gskeleton/watchdogs/raw/refs/heads/main/__gnu_linux.sh && chmod +x install.sh && ./install.sh
```

* cURL

```yaml
curl -L -o install.sh https://github.com/gskeleton/watchdogs/raw/refs/heads/main/__gnu_linux.sh && chmod +x install.sh && ./install.sh
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

```yaml
curl -L -o install.sh https://github.com/gskeleton/watchdogs/raw/refs/heads/main/__termux.sh && chmod +x install.sh && ./install.sh
```

4. **Jika muncul prompt seperti berikut (simulasi):**

```yaml
Enter the path you want to switch to location in storage/downloads
  ^ contoh: my_folder my_project my_server
  ^ nama folder untuk instalasi; jika belum ada, tidak masalah
  ^ tekan Enter jika ingin meng-install watchdogs di home (disarankan)
```

> **Tekan Enter tanpa mengetik apa pun.**
> Jika ada pertanyaan lain (misalnya pemilihan mirror Termux), pilih yang paling atas atau **tekan Enter saja**.

5. **Indikasi Watchdogs berhasil terpasang:**

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

> **Gunakan perintah `pawncc` untuk menyiapkan compiler (simulasi):**

```yaml
> pawncc
== Select a Platform ==
  [l] Linux
  [w] Windows
  [t] Termux
==> t # pilih t [termux]
== Select PawnCC Version ==
  A) Pawncc 3.10.11
  B) Pawncc 3.10.10
  C) Pawncc 3.10.9
  D) Pawncc 3.10.8
  E) Pawncc 3.10.7
> a # pilih versi compiler
* Try Downloading ? * Enable HTTP debugging? (y/n): n
 Try Extracting ? archive file...
==> Remove archive ?? (y/n) > n # opsional - hapus arsip nanti
==> Apply pawncc?
   answer (y/n) > y # pasang pawncc ke root (folder pawno)
>> I:moved without sudo: '?' -> '?'
>> I:moved without sudo: '?' -> '?'
>> I:Congratulations! - Done.
```

> Jika melihat simbol `>` berwarna cyan/abu-abu/biru (tampilan bisa berbeda), **cukup tekan Enter** kecuali diminta jawaban tertentu (misalnya apply pawncc = yes).

> Untuk langkah kompilasi, pelajari:
> [https://github.com/gskeleton/watchdogs/blob/main/README/README_ID.md#usage-guide](https://github.com/gskeleton/watchdogs/blob/main/README/README_ID.md#usage-guide)

---

## Native

> **Build untuk Windows?** Gunakan **MSYS2** (disarankan).

1. **Install Visual C++ Redistributable Runtimes (wajib untuk pawncc)**

   * Kunjungi: [https://www.techpowerup.com/download/visual-c-redistributable-runtime-package-all-in-one/](https://www.techpowerup.com/download/visual-c-redistributable-runtime-package-all-in-one/)
   * Klik **Download**
   * Ekstrak arsip
   * Jalankan `install_all.bat`

2. **Buka Command Prompt Windows, jalankan:**

```yaml
powershell -Command "Invoke-WebRequest 'https://raw.githubusercontent.com/gskeleton/watchdogs/refs/heads/main/__win.cmd' -OutFile 'install.cmd'; .\install.cmd"
```

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

> **Compile `server.pwn`:**

```yaml
# Kompilasi default
compile .
```

> **Compile dengan path spesifik**

```yaml
compile server.pwn
compile path/to/server.pwn
```

> **Compile dengan parent location (include path otomatis)**

```yaml
compile ../path/to/project/server.pwn
# -: -i/path/to/path/pawno -i/path/to/path/qawno -i/path/to/path/gamemodes
```

---

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

**Menjalankan server dengan gamemode default:**

```yaml
running .
```

**Menjalankan server dengan gamemode tertentu:**

```yaml
running server
```

**Compile dan jalankan sekaligus:**

```yaml
compiles
```

**Compile dan jalankan dengan path tertentu:**

```yaml
compiles server
```

---

## Dependency Management

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

**Install dependency dari `watchdogs.toml`:**

```yaml
replicate .
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
