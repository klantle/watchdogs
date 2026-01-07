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

1. Download Termux dari GitHub<br>
\- Android 7 keatas:<br>
https://github.com/termux/termux-app/releases/download/v0.119.0-beta.3/termux-app_v0.119.0-beta.3+apt-android-7-github-debug_universal.apk<br>
\- Android 5/6:<br>
https://github.com/termux/termux-app/releases/download/v0.119.0-beta.3/termux-app_v0.119.0-beta.3+apt-android-5-github-debug_universal.apk
2. File dot apk yang sudah di download lalu install dan jalankan Termux.
3. Pertama-tama jalankan perintah berikut ini didalam Termux yang sudah di-install:
```yml
curl -L -o install.sh https://github.com/gskeleton/watchdogs/raw/refs/heads/main/__termux.sh && chmod +x install.sh && ./install.sh
```
4. Jika kamu menemukan Pertanyaan seperti berikut (Simulasi):
```yml
Enter the path you want to switch to location in storage/downloads
  ^ example my_folder my_project my_server
  ^ enter if you want install the watchdogs in home (recommended)..
```
\> Tekan Enter tanpa memasukan apa-apa. dan jika ada pertanyaan lagi apapun itu seperti pilihan mirror termux kalian pilih yang paling atas atau all (tinggal enter saja).
1. Indikasi bahwa Watchdogs berhasil di install, seperti berikut:
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
\> Silahkan gunakan perintah `pawncc` untuk mempersiapkan alat peng-kompilasi (Simulasi):
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
==> Remove archive ?? (y/n) > n # bebas - menghapus arsip nantinya
==> Apply pawncc?
   answer (y/n) > y # aplikasikan pawncc di root - memasangnya kedalam folder pawno/ ; enter saja atau masukan y dan enter
> I: moved without sudo: '?' -> '?'
> I: moved without sudo: '?' -> '?'
> I: Congratulations! - Done.
```
\> Dan jika anda menemukan simbol `>` berwarna cyan/abu-abu/biru (berbeda-beda visual) maka kalian cukup enter tidak perlu memasukan apapun. kecuali yang diperlukan, seperti apply pawncc? ya.<br>
\> Untuk langkah kompilasi silahkan pelajari: https://github.com/gskeleton/watchdogs/blob/main/README/README_ID.md#panduan-penggunaan

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

## Perintah Kompilasi - Dengan Parent dir di Termux

```yml
~ compile ../storage/downloads/_NAMA_FOLDER_GAMEMODE_/gamemodes/_NAMA_FILE_PAWN_.pwn
```
Contohnya saya memiliki folder gamemode bernama induk di ZArchiver tepat pada lokasi Downloads
dan saya memiliki file utama project saja seperti pain.pwn yang ada pada gamemodes/ di dalam folder induk yang berada di Downloads.
maka tentu Saya akan menjadikan folder induk sebagai lokasi setelah Downloads dan pain.pwn setelah gamemodes.
```yml
~ compile ../storage/downloads/induk/pain.pwn
```

## Perintah Kompilasi - Umum

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
