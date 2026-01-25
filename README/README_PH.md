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

1. **I-download ang Termux mula sa GitHub**

   * Android 7 pataas:
     [https://github.com/termux/termux-app/releases/download/v0.119.0-beta.3/termux-app_v0.119.0-beta.3+apt-android-7-github-debug_universal.apk](https://github.com/termux/termux-app/releases/download/v0.119.0-beta.3/termux-app_v0.119.0-beta.3+apt-android-7-github-debug_universal.apk)
   * Android 5/6:
     [https://github.com/termux/termux-app/releases/download/v0.119.0-beta.3/termux-app_v0.119.0-beta.3+apt-android-5-github-debug_universal.apk](https://github.com/termux/termux-app/releases/download/v0.119.0-beta.3/termux-app_v0.119.0-beta.3+apt-android-5-github-debug_universal.apk)

2. **I-install ang na-download na .apk file at pagkatapos ay patakbuhin ang Termux.**

3. **Sa unang pagkakataon, patakbuhin ang sumusunod na command sa Termux:**

> pumili ng isa.

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

> Kung may ibang tanong (halimbawa: pagpili ng mirror ng Termux `?` (-openssl.cnf (Y/I/N/O/D/Z [default=N] ?)-), piliin ang pinakataas o **pindutin lang ang Enter**.

![watchdogs](https://raw.githubusercontent.com/gskeleton/dogdog/refs/heads/main/mirror.png)

4. **Indikasyon na matagumpay na na-install ang Watchdogs:**

![watchdogs](https://raw.githubusercontent.com/gskeleton/dogdog/refs/heads/main/indicate.png)

> **Gamitin ang command na `pawncc` para mag-set up ng compiler (simulation):**

```yaml
> pawncc
== Pumili ng Platform ==
  [l] Linux
  [w] Windows
  [t] Termux
==> t # piliin ang t [termux]
== Pumili ng Bersyon ng PawnCC ==
  A) Pawncc 3.10.11
  B) Pawncc 3.10.10
  C) Pawncc 3.10.9
  D) Pawncc 3.10.8
  E) Pawncc 3.10.7
> a # pumili ng bersyon ng compiler
* Subukan i-download? * I-enable ang HTTP debugging? (o/h): h
 Subukan i-extract? archive file...
==> Alisin ang archive?? (o/h) > h # optional - alisin ang archive mamaya
==> I-apply ang pawncc?
   sagot (o/h) > o # i-install ang pawncc sa root (folder ng pawno)
>> I:inilipat nang walang sudo: '?' -> '?'
>> I:inilipat nang walang sudo: '?' -> '?'
>> I:Binabati kita! - Tapos na.
```

---

![watchdogs](https://raw.githubusercontent.com/gskeleton/dogdog/refs/heads/main/pawncc_install.png)

> Kung makikita mo ang simbolong `>` **pindutin lang ang Enter** maliban kung may hinihinging partikular na sagot (halimbawa: apply pawncc = yes).

> Para sa mga hakbang sa compilation, alamin: [here](#compilation-commands--with-parent-directory-in-termux)

---

## Windows Native

> **Mag-build para sa Windows?** Gamitin ang **MSYS2** (recommended).

1. **I-install ang Visual C++ Redistributable Runtimes (kailangan para sa pawncc)**

   * Bisitahin: [https://www.techpowerup.com/download/visual-c-redistributable-runtime-package-all-in-one/](https://www.techpowerup.com/download/visual-c-redistributable-runtime-package-all-in-one/)
   * I-click ang **Download**
   * I-extract ang archive
   * Patakbuhin ang `install_all.bat`

2. **Buksan ang Windows Command Prompt, patakbuhin:**

```yaml
powershell -Command "Invoke-WebRequest 'https://raw.githubusercontent.com/gskeleton/watchdogs/refs/heads/main/__windows.cmd' -OutFile 'install.cmd'; .\install.cmd"
```

---

## Sanggunian sa Make Command

```yaml
make                # Mag-install ng library at mag-build
make linux          # Mag-build para sa Linux
make windows        # Mag-build para sa Windows
make termux         # Mag-build para sa Termux
make clean          # Linisin ang mga resulta ng build
make debug          # Mag-build gamit ang debug mode (Linux)
make debug-termux   # Mag-build gamit ang debug mode (Termux)
make windows-debug  # Mag-build gamit ang debug mode (Windows)
```

---

## GNU Debugger (GDB)

```yaml
# Hakbang 1 - Patakbuhin ang debugger (GDB) kasama ang programa
# Pumili ng executable ayon sa platform:
gdb ./watchdogs.debug        # Para sa Linux
gdb ./watchdogs.debug.tmux   # Para sa Termux (Android)
gdb ./watchdogs.debug.win    # Para sa Windows (kung gumagamit ng GDB)

# Hakbang 2 - Patakbuhin ang programa sa loob ng GDB
# Ang programa ay pinapatakbo sa ilalim ng kontrol ng debugger
run                           # i-type ang 'run' tapos Enter

# Hakbang 3 - Hawakan ang crash o interruptions
# Kung mag-crash ang programa (halimbawa: segmentation fault) o manual na itinigil (Ctrl+C),
# Ihihinto ng GDB ang execution at magpapakita ng prompt.

# Hakbang 4 - Tingnan ang status ng programa gamit ang backtrace
# Ipinapakita ng backtrace ang sequence ng function calls sa oras ng crash.
bt           # Pangunahing backtrace (mga pangalan ng function)
bt full      # Kumpletong backtrace (mga function, variable, arguments)
```

---

## Pag-execute gamit ang arguments

```yaml
./watchdogs command
./watchdogs command args
./watchdogs help compile
```

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

## Compilation

> Hindi mo kailangan ng partikular na pag-install ng Watchdogs sa folder ng GameMode o sa area ng ~/Downloads. Kailangan mo lang siguraduhin na ang folder na naglalaman ng binary watchdogs tulad ng watchdogs o watchdogs.tmux ay nasa loob ng folder na nasa Downloads, at ang folder ng iyong proyekto ay nasa loob din ng folder na nasa Downloads. (*HINDI ITO NAAANGKOP SA watchdogs.win)
```yml
# Halimbawang estruktura:
Downloads
├── dog
│   ├── watchdogs
└── myproj
    └── gamemodes
        └── proj.p
        # ^ kaya maaari mong patakbuhin ang watchdogs na nasa folder dog/
        # ^ at kailangan mo lang i-compile ito gamit ang parent symbol tulad ng sumusunod
        # ^ compile ../myproj/gamemodes/proj.p
        # ^ ang lokasyong ito ay halimbawa lamang
```

## Mga Command sa Compilation – Gamit ang Parent Directory sa Termux

```yaml
compile ../storage/downloads/_PANGALAN_NG_GAMEMODE_FOLDER_/gamemodes/_PANGALAN_NG_PAWN_FILE_.pwn
```

**Halimbawa:**
Mayroon akong folder ng gamemode na pinangalanang `parent` sa Downloads (sa pamamagitan ng ZArchiver), at ang pangunahing file na `pain.pwn` ay nasa loob ng `gamemodes/`.
Kaya ang path na ginamit ay:

```yaml
compile ../storage/downloads/parent/pain.pwn
```

---

## Mga Command sa Compilation – Pangkalahatan

> **I-compile ang `server.pwn`:**

```yaml
# Default compilation
compile .
compile.
```

> **Mag-compile gamit ang specific na path**

```yaml
compile server.pwn
compile path/to/server.pwn
```

> **Mag-compile gamit ang parent location (automatic include path)**

```yaml
compile ../path/to/project/server.pwn
# automatic: -i/path/to/path/pawno -i/path/to/path/qawno -i/path/to/path/gamemodes
```

---

## Pamamahala ng Server

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
|                  |     |    kung mayroong args  |                -
--------------------     --------------------------                -
```

**Patakbuhin ang server gamit ang default na gamemode:**

```yaml
running .
running.
```

**Patakbuhin ang server gamit ang partikular na gamemode:**

```yaml
running server
```

**I-compile at patakbuhin nang sabay:**

```yaml
compiles .
compiles.
```

**I-compile at patakbuhin gamit ang partikular na path:**

```yaml
compiles server
```

---

## Pamamahala ng Dependency

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

**Mag-install ng dependency mula sa `watchdogs.toml`:**

```yaml
replicate .
replicate.
```

**Mag-install ng partikular na repository:**

```yaml
replicate repo/user
```

**Mag-install ng partikular na bersyon (tag):**

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