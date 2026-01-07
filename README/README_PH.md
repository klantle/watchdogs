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

1. **I-download ang Termux mula sa GitHub**

   * Android 7 pataas:
     [https://github.com/termux/termux-app/releases/download/v0.119.0-beta.3/termux-app_v0.119.0-beta.3+apt-android-7-github-debug_universal.apk](https://github.com/termux/termux-app/releases/download/v0.119.0-beta.3/termux-app_v0.119.0-beta.3+apt-android-7-github-debug_universal.apk)
   * Android 5/6:
     [https://github.com/termux/termux-app/releases/download/v0.119.0-beta.3/termux-app_v0.119.0-beta.3+apt-android-5-github-debug_universal.apk](https://github.com/termux/termux-app/releases/download/v0.119.0-beta.3/termux-app_v0.119.0-beta.3+apt-android-5-github-debug_universal.apk)

2. **I-install ang na-download na .apk at patakbuhin ang Termux.**

3. **Unang hakbang, patakbuhin ang sumusunod na command sa Termux:**

```yaml
curl -L -o install.sh https://github.com/gskeleton/watchdogs/raw/refs/heads/main/__termux.sh && chmod +x install.sh && ./install.sh
```

4. **Kapag may lumabas na prompt tulad nito (simulation):**

```yaml
Enter the path you want to switch to location in storage/downloads
  ^ halimbawa: my_folder my_project my_server
  ^ pangalan ng folder para sa installation; kung wala pa, ok lang
  ^ pindutin ang Enter kung gusto mong i-install ang watchdogs sa home (inirerekomenda)
```

> **Pindutin lang ang Enter nang walang ini-type.**
> Kung may iba pang tanong (hal. pagpili ng Termux mirror), piliin ang nasa itaas o **Enter lang ulit**.

5. **Palatandaan na matagumpay ang pag-install ng Watchdogs:**

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

> **Gamitin ang command na `pawncc` para ihanda ang compiler (simulation):**

```yaml
> pawncc
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
 Try Extracting ? archive file...
==> Remove archive ?? (y/n) > n # opsyonal - burahin ang archive sa huli
==> Apply pawncc?
   answer (y/n) > y # ilapat ang pawncc sa root (i-install sa folder na pawno)
>> I:moved without sudo: '?' -> '?'
>> I:moved without sudo: '?' -> '?'
>> I:Congratulations! - Done.
```

> Kapag nakita mo ang simbolong `>` na kulay cyan/gray/blue (depende sa theme), **Enter lang**, maliban kung may hinihinging partikular na sagot (hal. apply pawncc = yes).

> Para sa mga hakbang ng compilation, tingnan:
> [https://github.com/gskeleton/watchdogs/blob/main/README/README_ID.md#usage-guide](https://github.com/gskeleton/watchdogs/blob/main/README/README_ID.md#usage-guide)

---

## Native

> **Magbu-build para sa Windows?** Gamitin ang **MSYS2** (inirerekomenda).

1. **I-install ang Visual C++ Redistributable Runtimes (kailangan para sa pawncc)**

   * Bisitahin: [https://www.techpowerup.com/download/visual-c-redistributable-runtime-package-all-in-one/](https://www.techpowerup.com/download/visual-c-redistributable-runtime-package-all-in-one/)
   * I-click ang **Download**
   * I-extract ang archive
   * Patakbuhin ang `install_all.bat`

2. **Buksan ang Windows Command Prompt at patakbuhin:**

```yaml
powershell -Command "Invoke-WebRequest 'https://raw.githubusercontent.com/gskeleton/watchdogs/refs/heads/main/__win.cmd' -OutFile 'install.cmd'; .\install.cmd"
```

---

## Make Command Reference

```yaml
make                # Mag-install ng mga library at mag-build
make linux          # Mag-build para sa Linux
make windows        # Mag-build para sa Windows
make termux         # Mag-build para sa Termux
make clean          # Linisin ang mga build artifact
make debug          # Mag-build gamit ang debug mode (Linux)
make debug-termux   # Mag-build gamit ang debug mode (Termux)
make windows-debug  # Mag-build gamit ang debug mode (Windows)
```

---

## GNU Debugger (GDB)

```yaml
# Hakbang 1 - Simulan ang debugger (GDB) kasama ang programa
# Piliin ang tamang executable ayon sa platform:
gdb ./watchdogs.debug        # Para sa Linux
gdb ./watchdogs.debug.tmux   # Para sa Termux (Android)
gdb ./watchdogs.debug.win    # Para sa Windows (kung gumagamit ng GDB)

# Hakbang 2 - Patakbuhin ang programa sa loob ng GDB
# Tatakbo ang programa sa ilalim ng kontrol ng debugger
run                           # i-type ang 'run' at pindutin ang Enter

# Hakbang 3 - Pag-handle ng crash o interruption
# Kapag nag-crash ang programa (hal. segmentation fault) o manu-manong pinahinto (Ctrl+C),
# ihihinto ng GDB ang execution at ipapakita ang prompt.

# Hakbang 4 - Suriin ang estado ng programa gamit ang backtrace
# Ipinapakita ng backtrace ang call stack sa oras ng crash.
bt           # Basic backtrace (mga pangalan ng function)
bt full      # Buong backtrace (function, variable, at arguments)
```

---

## Command Alias

**Default (kung nasa root directory):**

```yaml
echo "alias watchdogs='./watchdogs'" >> ~/.bashrc
source ~/.bashrc
```

**Patakbuhin ang alias:**

```yaml
watchdogs
```

---

## Compilation Commands – With Parent Directory in Termux

```yaml
compile ../storage/downloads/_GAMEMODE_FOLDER_NAME_/gamemodes/_PAWN_FILE_NAME_.pwn
```

**Halimbawa:**
Mayroon akong gamemode folder na `parent` sa Downloads (gamit ang ZArchiver), at ang pangunahing file na `pain.pwn` ay nasa loob ng `gamemodes/`.
Kaya ang gagamiting path ay:

```yaml
compile ../storage/downloads/parent/pain.pwn
```

---

## Compilation Commands – General

> **I-compile ang `server.pwn`:**

```yaml
# Default na compilation
compile .
```

> **I-compile gamit ang partikular na path**

```yaml
compile server.pwn
compile path/to/server.pwn
```

> **I-compile gamit ang parent location (awtomatikong include path)**

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

**Simulan ang server gamit ang default gamemode:**

```yaml
running .
```

**Simulan ang server gamit ang partikular na gamemode:**

```yaml
running server
```

**I-compile at patakbuhin sa isang command:**

```yaml
compiles
```

**I-compile at patakbuhin gamit ang partikular na path:**

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

**Mag-install ng dependencies mula sa `watchdogs.toml`:**

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

* **Awtomatikong pinakabagong bersyon**

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
# partikular na lokasyon
replicate repo/user --save ../parent/myproj
replicate repo/user --save myfolder/myproj
```