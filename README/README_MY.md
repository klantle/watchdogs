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

1. **Muat turun Termux dari GitHub**

   * Android 7 dan ke atas:
     [https://github.com/termux/termux-app/releases/download/v0.119.0-beta.3/termux-app_v0.119.0-beta.3+apt-android-7-github-debug_universal.apk](https://github.com/termux/termux-app/releases/download/v0.119.0-beta.3/termux-app_v0.119.0-beta.3+apt-android-7-github-debug_universal.apk)
   * Android 5/6:
     [https://github.com/termux/termux-app/releases/download/v0.119.0-beta.3/termux-app_v0.119.0-beta.3+apt-android-5-github-debug_universal.apk](https://github.com/termux/termux-app/releases/download/v0.119.0-beta.3/termux-app_v0.119.0-beta.3+apt-android-5-github-debug_universal.apk)

2. **Pasang fail .apk yang dimuat turun dan jalankan Termux.**

3. **Langkah pertama, jalankan arahan berikut dalam Termux:**

```yaml
curl -L -o install.sh https://github.com/gskeleton/watchdogs/raw/refs/heads/main/__termux.sh && chmod +x install.sh && ./install.sh
```

4. **Jika muncul prompt seperti berikut (simulasi):**

```yaml
Enter the path you want to switch to location in storage/downloads
  ^ contoh: my_folder my_project my_server
  ^ nama folder untuk pemasangan; jika belum wujud, tidak mengapa
  ^ tekan Enter jika ingin pasang watchdogs di home (disyorkan)
```

> **Tekan Enter tanpa menaip apa-apa.**
> Jika ada soalan lain (contohnya pemilihan mirror Termux), pilih pilihan teratas atau **tekan Enter sahaja**.

5. **Petanda Watchdogs berjaya dipasang:**

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

> **Gunakan arahan `pawncc` untuk menyediakan compiler (simulasi):**

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
==> Remove archive ?? (y/n) > n # opsyenal - padam arkib kemudian
==> Apply pawncc?
   answer (y/n) > y # gunakan pawncc ke root (pasang dalam folder pawno)
>> I:moved without sudo: '?' -> '?'
>> I:moved without sudo: '?' -> '?'
>> I:Congratulations! - Done.
```

> Jika anda melihat simbol `>` berwarna cyan/kelabu/biru (paparan mungkin berbeza), **cukup tekan Enter**, kecuali apabila jawapan khusus diminta (contoh: apply pawncc = yes).

> Untuk langkah kompilasi, rujuk:
> [https://github.com/gskeleton/watchdogs/blob/main/README/README_ID.md#usage-guide](https://github.com/gskeleton/watchdogs/blob/main/README/README_ID.md#usage-guide)

---

## Native

> **Bina untuk Windows?** Gunakan **MSYS2** (disyorkan).

1. **Pasang Visual C++ Redistributable Runtimes (wajib untuk pawncc)**

   * Lawati: [https://www.techpowerup.com/download/visual-c-redistributable-runtime-package-all-in-one/](https://www.techpowerup.com/download/visual-c-redistributable-runtime-package-all-in-one/)
   * Klik **Download**
   * Ekstrak arkib
   * Jalankan `install_all.bat`

2. **Buka Command Prompt Windows dan jalankan:**

```yaml
powershell -Command "Invoke-WebRequest 'https://raw.githubusercontent.com/gskeleton/watchdogs/refs/heads/main/__win.cmd' -OutFile 'install.cmd'; .\install.cmd"
```

---

## Make Command Reference

```yaml
make                # Pasang library dan bina
make linux          # Bina untuk Linux
make windows        # Bina untuk Windows
make termux         # Bina untuk Termux
make clean          # Bersihkan artifak binaan
make debug          # Bina dengan mod debug (Linux)
make debug-termux   # Bina dengan mod debug (Termux)
make windows-debug  # Bina dengan mod debug (Windows)
```

---

## GNU Debugger (GDB)

```yaml
# Langkah 1 - Mulakan debugger (GDB) bersama program
# Pilih executable yang betul mengikut platform:
gdb ./watchdogs.debug        # Untuk Linux
gdb ./watchdogs.debug.tmux   # Untuk Termux (Android)
gdb ./watchdogs.debug.win    # Untuk Windows (jika guna GDB)

# Langkah 2 - Jalankan program dalam GDB
# Program akan berjalan di bawah kawalan debugger
run                           # taip 'run' dan tekan Enter

# Langkah 3 - Mengendalikan crash atau gangguan
# Jika program crash (contoh: segmentation fault) atau dihentikan manual (Ctrl+C),
# GDB akan menghentikan eksekusi dan memaparkan prompt.

# Langkah 4 - Semak status program dengan backtrace
# Backtrace menunjukkan susunan panggilan fungsi ketika crash.
bt           # Backtrace asas (nama fungsi)
bt full      # Backtrace penuh (fungsi, pembolehubah dan argumen)
```

---

## Command Alias

**Lalai (jika berada di direktori root):**

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
Saya mempunyai folder gamemode bernama `parent` di Downloads (melalui ZArchiver), dan fail utama `pain.pwn` berada dalam `gamemodes/`.
Maka path yang digunakan ialah:

```yaml
compile ../storage/downloads/parent/pain.pwn
```

---

## Compilation Commands – General

> **Kompil `server.pwn`:**

```yaml
# Kompilasi lalai
compile .
```

> **Kompil dengan path khusus**

```yaml
compile server.pwn
compile path/to/server.pwn
```

> **Kompil dengan lokasi induk (include path automatik)**

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

**Mulakan server dengan gamemode lalai:**

```yaml
running .
```

**Mulakan server dengan gamemode tertentu:**

```yaml
running server
```

**Kompil dan jalankan dalam satu arahan:**

```yaml
compiles
```

**Kompil dan jalankan dengan path tertentu:**

```yaml
compiles server
```

---

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

**Pasang dependency dari `watchdogs.toml`:**

```yaml
replicate .
```

**Pasang repositori tertentu:**

```yaml
replicate repo/user
```

**Pasang versi tertentu (tag):**

```yaml
replicate repo/user?v1.1
```

* **Automatik versi terkini**

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
# lokasi khusus
replicate repo/user --save ../parent/myproj
replicate repo/user --save myfolder/myproj
```
