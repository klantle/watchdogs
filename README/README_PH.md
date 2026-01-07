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

1. I-download ang Termux mula sa GitHub<br>
   - Android 7 pataas:<br>
   [https://github.com/termux/termux-app/releases/download/v0.119.0-beta.3/termux-app_v0.119.0-beta.3+apt-android-7-github-debug_universal.apk](https://github.com/termux/termux-app/releases/download/v0.119.0-beta.3/termux-app_v0.119.0-beta.3+apt-android-7-github-debug_universal.apk)<br>
   - Android 5/6:<br>
   [https://github.com/termux/termux-app/releases/download/v0.119.0-beta.3/termux-app_v0.119.0-beta.3+apt-android-5-github-debug_universal.apk](https://github.com/termux/termux-app/releases/download/v0.119.0-beta.3/termux-app_v0.119.0-beta.3+apt-android-5-github-debug_universal.apk)
2. Ang na-download na dot apk file ay i-install at pagkatapos ay patakbuhin ang Termux.
3. Una sa lahat, patakbuhin ang sumusunod na command sa loob ng Termux na naka-install na:

```yml
curl -L -o install.sh https://github.com/gskeleton/watchdogs/raw/refs/heads/main/__termux.sh && chmod +x install.sh && ./install.sh
```

4. Kung makakita ka ng tanong tulad ng sumusunod (Simulasyon):

```yml
Enter the path you want to switch to location in storage/downloads
  ^ example my_folder my_project my_server
  ^ enter if you want install the watchdogs in home (recommended)..
```

> Pindutin ang Enter nang walang inilalagay kahit ano. at kung may iba pang tanong tulad ng pagpili ng mirror ng termux, piliin ninyo ang pinaka-itaas o all (Enter lang).

1. Palatandaan na matagumpay na na-install ang Watchdogs, tulad ng sumusunod:

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

> Pakigamit ang command na `pawncc` upang ihanda ang tool sa pagko-kompile (Simulasyon):

```yml
~ pawncc # patakbuhin
== Select a Platform ==
  [l] Linux
  [w] Windows
  [t] Termux
==> t # piliin ang t [termux]
== Select PawnCC Version ==
  A) Pawncc 3.10.11
  B) Pawncc 3.10.10
  C) Pawncc 3.10.9
  D) Pawncc 3.10.8
  E) Pawncc 3.10.7
> a # piliin ang bersyon ng compiler
* Try Downloading ? * Enable HTTP debugging? (y/n): n
** Downloading... 100% Done! % successful: 417290 bytes to ?
 Try Extracting ? archive file...
==> Remove archive ?? (y/n) > n # malaya - tanggalin ang archive sa ibang pagkakataon
==> Apply pawncc?
   answer (y/n) > y # ilapat ang pawncc sa root - i-install ito sa loob ng folder pawno/ ; enter lang o maglagay ng y at enter
> I: moved without sudo: '?' -> '?'
> I: moved without sudo: '?' -> '?'
> I: Congratulations! - Done.
```

> At kung makakita kayo ng simbolong `>` na may kulay cyan/abo/asul (iba-iba ang visual), pindutin lang ang enter at huwag maglagay ng kahit ano. maliban na lang kung kinakailangan, tulad ng apply pawncc? oo.<br>
> Para sa mga hakbang ng pagko-kompile, pakibasa: [https://github.com/gskeleton/watchdogs/blob/main/README/README_ID.md#panduan-penggunaan](https://github.com/gskeleton/watchdogs/blob/main/README/README_ID.md#panduan-penggunaan)

## Native

> Para sa Build ng Windows? Gamitin ang Inirerekomendang MSYS2 (pacman -S make && make && make windows)

1. I-install muna ang Visual C++ Redistributable Runtimes All-in-One (kinakailangan para sa pawncc)

* Bisitahin ang [https://www.techpowerup.com/download/visual-c-redistributable-runtime-package-all-in-one/](https://www.techpowerup.com/download/visual-c-redistributable-runtime-package-all-in-one/)
* I-click ang "Download"
* I-extract ang archive
* Patakbuhin lang ang "install_all.bat".

2. Buksan ang Command Prompt ng Windows, patakbuhin:

```yml
powershell -Command "Invoke-WebRequest 'https://raw.githubusercontent.com/gskeleton/watchdogs/refs/heads/main/__win.cmd' -OutFile 'install.cmd'; .\install.cmd"
```

## Sanggunian ng Make Commands

```yaml
make                # I-install ang mga library at mag-build
make linux          # Build para sa Linux
make windows        # Build para sa Windows
make termux         # Build para sa Termux
make clean          # Linisin ang mga build artifact
make debug          # Build gamit ang debug mode (Linux)
make debug-termux   # Build gamit ang debug mode (Termux)
make windows-debug  # Build gamit ang debug mode (Windows)
```

## GNU Debugger (GDB)

```yaml
# Hakbang 1 - Simulan ang debugger (GDB) gamit ang iyong programa
# Piliin ang tamang executable ayon sa iyong platform:
gdb ./watchdogs.debug        # Para sa Linux
gdb ./watchdogs.debug.tmux   # Para sa Termux (Android - Termux)
gdb ./watchdogs.debug.win    # Para sa Windows (kung gumagamit ng GDB sa Windows)

# Hakbang 2 - Patakbuhin ang programa sa loob ng GDB
# Sinisimulan nito ang programa sa ilalim ng kontrol ng debugger.
run                           # I-type lang ang 'run' at pindutin ang Enter

# Hakbang 3 - Paghawak ng crash o interrupt
# Kung ang programa ay nag-crash (hal: segmentation fault) o hininto ninyo ito nang manu-mano (Ctrl+C),
# ihihinto ng GDB ang execution at ipapakita ang prompt.

# Hakbang 4 - Suriin ang estado ng programa gamit ang backtrace
# Ipinapakita ng backtrace ang stack ng mga function call sa punto ng crash.
bt           # Pangunahing backtrace (ipinapakita ang mga pangalan ng function)
bt full      # Buong backtrace (ipinapakita ang mga pangalan ng function, variable, at argumento)
```

## Alias ng Command

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

## Command sa Pagko-kompile - Gamit ang Parent dir sa Termux

```yml
~ compile ../storage/downloads/_NAMA_FOLDER_GAMEMODE_/gamemodes/_NAMA_FILE_PAWN_.pwn
```

Halimbawa, mayroon akong gamemode folder na may pangalang induk sa ZArchiver eksakto sa lokasyong Downloads
at mayroon akong pangunahing project file lamang tulad ng pain.pwn na nasa gamemodes/ sa loob ng induk folder na nasa Downloads.
kaya tiyak na gagawin kong induk folder bilang lokasyon pagkatapos ng Downloads at pain.pwn pagkatapos ng gamemodes.

```yml
~ compile ../storage/downloads/induk/pain.pwn
```

## Command sa Pagko-kompile - Pangkalahatan

> Kompilehin ang `server.pwn`:

```yaml
# Default na Kompilasyon
compile .
```

> Kompilasyon gamit ang partikular na file path

```yml
compile server.pwn
compile path/ke/server.pwn
```

> Kompilasyon gamit ang parent location - awtomatikong include path

```yaml
compile ../path/ke/project/server.pwn
# -: -i/path/ke/path/pawno -i/path/ke/path/qawno -i/path/ke/path/gamemodes
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

**Simulan ang server gamit ang default na gamemode:**

```yaml
running .
```

**Simulan ang server gamit ang partikular na gamemode:**

```yaml
running server
```

**Kompilahin at simulan sa isang command:**

```yaml
compiles
```

**Kompilahin at simulan gamit ang partikular na path:**

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

**Mag-install ng dependency mula sa `watchdogs.toml`:**

```yaml
replicate .
```

**Mag-install ng partikular na repository:**

```yaml
replicate repo/user
```

**Mag-install ng partikular na bersyon (tags):**

```yaml
replicate repo/user?v1.1
```

* Awtomatikong pinakabago

```yaml
replicate repo/user?newer
```

**Mag-install ng partikular na branch:**

```yaml
replicate repo/user --branch master
```

**Mag-install sa partikular na lokasyon:**

```yaml
# root
replicate repo/user --save .
# partikular
replicate repo/user --save ../parent/myproj
replicate repo/user --save myfol/myproj
```
