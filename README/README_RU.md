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

1. Скачать Termux с GitHub<br>
   - Android 7 и выше:<br>
   [https://github.com/termux/termux-app/releases/download/v0.119.0-beta.3/termux-app_v0.119.0-beta.3+apt-android-7-github-debug_universal.apk](https://github.com/termux/termux-app/releases/download/v0.119.0-beta.3/termux-app_v0.119.0-beta.3+apt-android-7-github-debug_universal.apk)<br>
   - Android 5/6:<br>
   [https://github.com/termux/termux-app/releases/download/v0.119.0-beta.3/termux-app_v0.119.0-beta.3+apt-android-5-github-debug_universal.apk](https://github.com/termux/termux-app/releases/download/v0.119.0-beta.3/termux-app_v0.119.0-beta.3+apt-android-5-github-debug_universal.apk)
2. Скачанный файл .apk установить, затем запустить Termux.
3. Сначала выполните следующую команду внутри установленного Termux:

```yml
curl -L -o install.sh https://github.com/gskeleton/watchdogs/raw/refs/heads/main/__termux.sh && chmod +x install.sh && ./install.sh
```

4. Если вы увидите вопрос, подобный следующему (Симуляция):

```yml
Enter the path you want to switch to location in storage/downloads
  ^ example my_folder my_project my_server
  ^ enter if you want install the watchdogs in home (recommended)..
```

> Нажмите Enter, ничего не вводя. и если появятся другие вопросы, например выбор зеркала termux, выберите самый верхний вариант или all (просто нажмите enter).

1. Признак того, что Watchdogs успешно установлен, выглядит следующим образом:

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

> Пожалуйста, используйте команду `pawncc` для подготовки инструмента компиляции (Симуляция):

```yml
~ pawncc # выполнить
== Select a Platform ==
  [l] Linux
  [w] Windows
  [t] Termux
==> t # выбрать t [termux]
== Select PawnCC Version ==
  A) Pawncc 3.10.11
  B) Pawncc 3.10.10
  C) Pawncc 3.10.9
  D) Pawncc 3.10.8
  E) Pawncc 3.10.7
> a # выбрать версию компилятора
* Try Downloading ? * Enable HTTP debugging? (y/n): n
** Downloading... 100% Done! % successful: 417290 bytes to ?
 Try Extracting ? archive file...
==> Remove archive ?? (y/n) > n # на усмотрение — удалить архив позже
==> Apply pawncc?
   answer (y/n) > y # применить pawncc в root — установить его в папку pawno/ ; просто enter или введите y и enter
> I: moved without sudo: '?' -> '?'
> I: moved without sudo: '?' -> '?'
> I: Congratulations! - Done.
```

> И если вы увидите символ `>` цвета cyan/серого/синего (визуально может отличаться), просто нажмите enter и ничего не вводите. кроме случаев, когда требуется ответ, например apply pawncc? да.<br>
> Для шагов компиляции, пожалуйста, изучите: [https://github.com/gskeleton/watchdogs/blob/main/README/README_ID.md#panduan-penggunaan](https://github.com/gskeleton/watchdogs/blob/main/README/README_ID.md#panduan-penggunaan)

## Native

> Для сборки под Windows? Используйте рекомендуемый MSYS2 (pacman -S make && make && make windows)

1. Сначала установите Visual C++ Redistributable Runtimes All-in-One (требуется для pawncc)

* Перейдите на [https://www.techpowerup.com/download/visual-c-redistributable-runtime-package-all-in-one/](https://www.techpowerup.com/download/visual-c-redistributable-runtime-package-all-in-one/)
* Нажмите "Download"
* Распакуйте архив
* Просто запустите "install_all.bat".

2. Откройте Command Prompt Windows и выполните:

```yml
powershell -Command "Invoke-WebRequest 'https://raw.githubusercontent.com/gskeleton/watchdogs/refs/heads/main/__win.cmd' -OutFile 'install.cmd'; .\install.cmd"
```

## Справка по командам Make

```yaml
make                # Установить библиотеки и собрать
make linux          # Сборка для Linux
make windows        # Сборка для Windows
make termux         # Сборка для Termux
make clean          # Очистить артефакты сборки
make debug          # Сборка в режиме отладки (Linux)
make debug-termux   # Сборка в режиме отладки (Termux)
make windows-debug  # Сборка в режиме отладки (Windows)
```

## GNU Debugger (GDB)

```yaml
# Шаг 1 — Запуск отладчика (GDB) с вашей программой
# Выберите правильный исполняемый файл для вашей платформы:
gdb ./watchdogs.debug        # Для Linux
gdb ./watchdogs.debug.tmux   # Для Termux (Android - Termux)
gdb ./watchdogs.debug.win    # Для Windows (если используется GDB в Windows)

# Шаг 2 — Запуск программы внутри GDB
# Это запускает программу под управлением отладчика.
run                           # Просто введите 'run' и нажмите Enter

# Шаг 3 — Обработка сбоев или прерываний
# Если программа аварийно завершилась (например: segmentation fault) или вы остановили её вручную (Ctrl+C),
# GDB остановит выполнение и покажет приглашение.

# Шаг 4 — Проверка состояния программы с помощью backtrace
# Backtrace показывает стек вызовов функций в момент сбоя.
bt           # Базовый backtrace (показывает имена функций)
bt full      # Полный backtrace (показывает имена функций, переменные и аргументы)
```

## Алиасы команд

По умолчанию (если в корневом каталоге):

```yaml
echo "alias watchdogs='./watchdogs'" >> ~/.bashrc
source ~/.bashrc
```

И запустите алиас:

```yml
watchdogs
```

## Руководство по использованию

## Команда компиляции — с родительским каталогом в Termux

```yml
~ compile ../storage/downloads/_NAMA_FOLDER_GAMEMODE_/gamemodes/_NAMA_FILE_PAWN_.pwn
```

Например, у меня есть папка gamemode с именем induk в ZArchiver, расположенная точно в каталоге Downloads,
и у меня есть только основной файл проекта, например pain.pwn, который находится в gamemodes/ внутри папки induk, расположенной в Downloads.
тогда, конечно, я буду использовать папку induk как путь после Downloads, а pain.pwn — после gamemodes.

```yml
~ compile ../storage/downloads/induk/pain.pwn
```

## Команда компиляции — общая

> Компиляция `server.pwn`:

```yaml
# Компиляция по умолчанию
compile .
```

> Компиляция с указанием конкретного пути файла

```yml
compile server.pwn
compile path/ke/server.pwn
```

> Компиляция с родительским расположением — автоматические include-пути

```yaml
compile ../path/ke/project/server.pwn
# -: -i/path/ke/path/pawno -i/path/ke/path/qawno -i/path/ke/path/gamemodes
```

## Управление сервером

* **Алгоритм**

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

**Запуск сервера с gamemode по умолчанию:**

```yaml
running .
```

**Запуск сервера с конкретным gamemode:**

```yaml
running server
```

**Компиляция и запуск одной командой:**

```yaml
compiles
```

**Компиляция и запуск с конкретным путём:**

```yml
compiles server
```

## Управление зависимостями

* **Алгоритм**

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

**Установка зависимостей из `watchdogs.toml`:**

```yaml
replicate .
```

**Установка конкретного репозитория:**

```yaml
replicate repo/user
```

**Установка конкретной версии (tags):**

```yaml
replicate repo/user?v1.1
```

* Автоматически самая новая

```yaml
replicate repo/user?newer
```

**Установка конкретной ветки:**

```yaml
replicate repo/user --branch master
```

**Установка в конкретное расположение:**

```yaml
# root
replicate repo/user --save .
# конкретное
replicate repo/user --save ../parent/myproj
replicate repo/user --save myfol/myproj
```
