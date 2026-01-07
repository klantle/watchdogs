# Watchdogs

## Linux

* [x] GNU/wget

```yml
wget -O install.sh https://github.com/gskeleton/watchdogs/raw/refs/heads/main/__linux.sh && chmod +x install.sh && ./install.sh
```

* [x] cURL

```yml
curl -L -o install.sh https://github.com/gskeleton/watchdogs/raw/refs/heads/main/__linux.sh && chmod +x install.sh && ./install.sh
```

## Termux

1. Muat turun Termux daripada GitHub<br>
   - Android 7 ke atas:<br>
   [https://github.com/termux/termux-app/releases/download/v0.119.0-beta.3/termux-app_v0.119.0-beta.3+apt-android-7-github-debug_universal.apk](https://github.com/termux/termux-app/releases/download/v0.119.0-beta.3/termux-app_v0.119.0-beta.3+apt-android-7-github-debug_universal.apk)<br>
   - Android 5/6:<br>
   [https://github.com/termux/termux-app/releases/download/v0.119.0-beta.3/termux-app_v0.119.0-beta.3+apt-android-5-github-debug_universal.apk](https://github.com/termux/termux-app/releases/download/v0.119.0-beta.3/termux-app_v0.119.0-beta.3+apt-android-5-github-debug_universal.apk)
2. Fail dot apk yang telah dimuat turun kemudian pasang dan jalankan Termux.
3. Mula-mula jalankan perintah berikut ini di dalam Termux yang telah dipasang:

```yml
curl -L -o install.sh https://github.com/gskeleton/watchdogs/raw/refs/heads/main/__termux.sh && chmod +x install.sh && ./install.sh
```

4. Jika anda menemui soalan seperti berikut (Simulasi):

```yml
Enter the path you want to switch to location in storage/downloads
  ^ example my_folder my_project my_server
  ^ enter if you want install the watchdogs in home (recommended)..
```

> Tekan Enter tanpa memasukkan apa-apa. dan jika ada soalan lagi apa-apa sahaja seperti pilihan mirror termux, anda pilih yang paling atas atau all (hanya tekan enter sahaja).

1. Petunjuk bahawa Watchdogs berjaya dipasang, seperti berikut:

```yml
  \/%#z.       \/.%#z./       ,z#%\/
   \X##k      /X#####X\      d##X/
    \888\    /888/ \888\    /888/
     `v88;  ;88v'   `v88;  ;88v'
       \77xx77/       \77xx77/
        `::::'         `::::'
Type "help" for more information.
~
```

> Sila gunakan perintah `pawncc` untuk menyediakan alat peng-kompilasian (Simulasi):

```yml
~ pawncc # jalankan
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
** Downloading... 100% Done! % successful: 417290 bytes to ?
 Try Extracting ? archive file...
==> Remove archive ?? (y/n) > n # bebas - memadam arkib kemudian
==> Apply pawncc?
   answer (y/n) > y # aplikasikan pawncc di root - memasangnya ke dalam folder pawno/ ; tekan enter sahaja atau masukkan y dan enter
> I: moved without sudo: '?' -> '?'
> I: moved without sudo: '?' -> '?'
> I: Congratulations! - Done.
```

> Dan jika anda menemui simbol `>` berwarna cyan/kelabu/biru (visual berbeza-beza) maka anda hanya perlu tekan enter tanpa memasukkan apa-apa. kecuali yang diperlukan, seperti apply pawncc? ya.<br>
> Untuk langkah kompilasi sila pelajari: [https://github.com/gskeleton/watchdogs/blob/main/README/README_ID.md#panduan-penggunaan](https://github.com/gskeleton/watchdogs/blob/main/README/README_ID.md#panduan-penggunaan)

## Native

> Untuk Build Windows? Gunakan MSYS2 yang Disyorkan (pacman -S make && make && make windows)

1. Pasang Visual C++ Redistributable Runtimes All-in-One terlebih dahulu (diperlukan untuk pawncc)

* Lawati [https://www.techpowerup.com/download/visual-c-redistributable-runtime-package-all-in-one/](https://www.techpowerup.com/download/visual-c-redistributable-runtime-package-all-in-one/)
* Klik "Download"
* Ekstrak arkibnya
* Jalankan sahaja "install_all.bat".

2. Buka Command Prompt Windows, Jalankan:

```yml
powershell -Command "Invoke-WebRequest 'https://raw.githubusercontent.com/gskeleton/watchdogs/refs/heads/main/__win.cmd' -OutFile 'install.cmd'; .\install.cmd"
```

## Rujukan Perintah Make

```yaml
make                # Pasang pustaka dan bina
make linux          # Bina untuk Linux
make windows        # Bina untuk Windows
make termux         # Bina untuk Termux
make clean          # Bersihkan artifak binaan
make debug          # Bina dengan mod debug (Linux)
make debug-termux   # Bina dengan mod debug (Termux)
make windows-debug  # Bina dengan mod debug (Windows)
```

## GNU Debugger (GDB)

```yaml
# Langkah 1 - Mulakan debugger (GDB) dengan program anda
# Pilih eksekutabel yang betul mengikut platform anda:
gdb ./watchdogs.debug        # Untuk Linux
gdb ./watchdogs.debug.tmux   # Untuk Termux (Android - Termux)
gdb ./watchdogs.debug.win    # Untuk Windows (jika menggunakan GDB di Windows)

# Langkah 2 - Jalankan program di dalam GDB
# Ini memulakan program di bawah kawalan debugger.
run                           # Taip sahaja 'run' dan tekan Enter

# Langkah 3 - Menangani crash atau gangguan
# Jika program crash (contoh: segmentation fault) atau anda menghentikannya secara manual (Ctrl+C),
# GDB akan menghentikan eksekusi dan memaparkan prompt.

# Langkah 4 - Periksa status program dengan backtrace
# Backtrace menunjukkan stack panggilan fungsi pada titik crash.
bt           # Backtrace asas (menunjukkan nama fungsi)
bt full      # Backtrace penuh (menunjukkan nama fungsi, pembolehubah, dan argumen)
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

## Perintah Kompilasi - Dengan Parent dir di Termux

```yml
~ compile ../storage/downloads/_NAMA_FOLDER_GAMEMODE_/gamemodes/_NAMA_FILE_PAWN_.pwn
```

Contohnya saya mempunyai folder gamemode bernama induk di ZArchiver tepat pada lokasi Downloads
dan saya mempunyai fail utama projek sahaja seperti pain.pwn yang berada di gamemodes/ di dalam folder induk yang berada di Downloads.
maka sudah tentu saya akan menjadikan folder induk sebagai lokasi selepas Downloads dan pain.pwn selepas gamemodes.

```yml
~ compile ../storage/downloads/induk/pain.pwn
```

## Perintah Kompilasi - Umum

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

**Kompilasi dan mulakan dalam satu perintah:**

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

**Pasang dependensi daripada `watchdogs.toml`:**

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

* Automatik terkini

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
