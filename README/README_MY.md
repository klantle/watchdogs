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

> **Gunakan arahan `pawncc` untuk menyediakan compiler (simulasi):**

```yaml
> pawncc
== Pilih Platform ==
  [l] Linux
  [w] Windows
  [t] Termux
==> t # pilih t [termux]
== Pilih Versi PawnCC ==
  A) Pawncc 3.10.11
  B) Pawncc 3.10.10
  C) Pawncc 3.10.9
  D) Pawncc 3.10.8
  E) Pawncc 3.10.7
> a # pilih versi compiler
* Cuba Muat Turun ? * Dayakan HTTP debugging? (y/n): n
 Cuba Ekstrak ? fail arkib...
==> Buang arkib ?? (y/n) > n # pilihan - buang arkib kemudian
==> Guna pawncc?
   jawapan (y/n) > y # pasang pawncc ke root (folder pawno)
>> I:dipindahkan tanpa sudo: '?' -> '?'
>> I:dipindahkan tanpa sudo: '?' -> '?'
>> I:Tahniah! - Selesai.
```

---

![watchdogs](https://raw.githubusercontent.com/gskeleton/dogdog/refs/heads/main/pawncc_install.png)

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