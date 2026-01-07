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

1. **Скачайте Termux с GitHub**

   * Android 7 и выше:
     [https://github.com/termux/termux-app/releases/download/v0.119.0-beta.3/termux-app_v0.119.0-beta.3+apt-android-7-github-debug_universal.apk](https://github.com/termux/termux-app/releases/download/v0.119.0-beta.3/termux-app_v0.119.0-beta.3+apt-android-7-github-debug_universal.apk)
   * Android 5/6:
     [https://github.com/termux/termux-app/releases/download/v0.119.0-beta.3/termux-app_v0.119.0-beta.3+apt-android-5-github-debug_universal.apk](https://github.com/termux/termux-app/releases/download/v0.119.0-beta.3/termux-app_v0.119.0-beta.3+apt-android-5-github-debug_universal.apk)

2. **Установите загруженный файл .apk и запустите Termux.**

3. **Сначала выполните следующую команду в Termux:**

```yaml
curl -L -o install.sh https://github.com/gskeleton/watchdogs/raw/refs/heads/main/__termux.sh && chmod +x install.sh && ./install.sh
```

4. **Если появится запрос вида (симуляция):**

```yaml
Enter the path you want to switch to location in storage/downloads
  ^ пример: my_folder my_project my_server
  ^ имя папки для установки; если её нет — не проблема
  ^ нажмите Enter, если хотите установить watchdogs в home (рекомендуется)
```

> **Нажмите Enter, ничего не вводя.**
> Если появятся другие вопросы (например, выбор зеркала Termux), выберите первый вариант или просто **нажмите Enter**.

5. **Признак успешной установки Watchdogs:**

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

> **Используйте команду `pawncc` для подготовки компилятора (симуляция):**

```yaml
> pawncc
== Select a Platform ==
  [l] Linux
  [w] Windows
  [t] Termux
==> t # выберите t [termux]
== Select PawnCC Version ==
  A) Pawncc 3.10.11
  B) Pawncc 3.10.10
  C) Pawncc 3.10.9
  D) Pawncc 3.10.8
  E) Pawncc 3.10.7
> a # выберите версию компилятора
* Try Downloading ? * Enable HTTP debugging? (y/n): n
 Try Extracting ? archive file...
==> Remove archive ?? (y/n) > n # опционально — удалить архив позже
==> Apply pawncc?
   answer (y/n) > y # применить pawncc к root (установить в папку pawno)
>> I:moved without sudo: '?' -> '?'
>> I:moved without sudo: '?' -> '?'
>> I:Congratulations! - Done.
```

> Если вы видите символ `>` голубого/серого/синего цвета (оформление может отличаться), **просто нажмите Enter**, кроме случаев, когда требуется конкретный ответ (например: apply pawncc = yes).

> Инструкции по компиляции см.:
> [https://github.com/gskeleton/watchdogs/blob/main/README/README_ID.md#usage-guide](https://github.com/gskeleton/watchdogs/blob/main/README/README_ID.md#usage-guide)

---

## Native

> **Сборка под Windows?** Используйте **MSYS2** (рекомендуется).

1. **Установите Visual C++ Redistributable Runtimes (обязательно для pawncc)**

   * Перейдите: [https://www.techpowerup.com/download/visual-c-redistributable-runtime-package-all-in-one/](https://www.techpowerup.com/download/visual-c-redistributable-runtime-package-all-in-one/)
   * Нажмите **Download**
   * Распакуйте архив
   * Запустите `install_all.bat`

2. **Откройте Windows Command Prompt и выполните:**

```yaml
powershell -Command "Invoke-WebRequest 'https://raw.githubusercontent.com/gskeleton/watchdogs/refs/heads/main/__win.cmd' -OutFile 'install.cmd'; .\install.cmd"
```

---

## Make Command Reference

```yaml
make                # Установить библиотеки и выполнить сборку
make linux          # Сборка для Linux
make windows        # Сборка для Windows
make termux         # Сборка для Termux
make clean          # Очистка артефактов сборки
make debug          # Сборка в режиме отладки (Linux)
make debug-termux   # Сборка в режиме отладки (Termux)
make windows-debug  # Сборка в режиме отладки (Windows)
```

---

## GNU Debugger (GDB)

```yaml
# Шаг 1 — Запуск отладчика (GDB) с программой
# Выберите подходящий исполняемый файл для вашей платформы:
gdb ./watchdogs.debug        # Для Linux
gdb ./watchdogs.debug.tmux   # Для Termux (Android)
gdb ./watchdogs.debug.win    # Для Windows (если используется GDB)

# Шаг 2 — Запуск программы внутри GDB
# Программа будет выполняться под контролем отладчика
run                           # введите 'run' и нажмите Enter

# Шаг 3 — Обработка сбоев или прерываний
# Если программа аварийно завершится (например, segmentation fault)
# или будет остановлена вручную (Ctrl+C),
# GDB остановит выполнение и покажет prompt.

# Шаг 4 — Проверка состояния программы с помощью backtrace
# Backtrace показывает стек вызовов в момент сбоя.
bt           # Базовый backtrace (имена функций)
bt full      # Полный backtrace (функции, переменные и аргументы)
```

---

## Command Alias

**По умолчанию (если вы находитесь в корневом каталоге):**

```yaml
echo "alias watchdogs='./watchdogs'" >> ~/.bashrc
source ~/.bashrc
```

**Запуск alias:**

```yaml
watchdogs
```

---

## Compilation Commands – With Parent Directory in Termux

```yaml
compile ../storage/downloads/_GAMEMODE_FOLDER_NAME_/gamemodes/_PAWN_FILE_NAME_.pwn
```

**Пример:**
У меня есть папка gamemode с именем `parent` в Downloads (через ZArchiver), а основной файл `pain.pwn` находится в `gamemodes/`.
В этом случае используется путь:

```yaml
compile ../storage/downloads/parent/pain.pwn
```

---

## Compilation Commands – General

> **Компиляция `server.pwn`:**

```yaml
# Компиляция по умолчанию
compile .
```

> **Компиляция с указанием конкретного пути**

```yaml
compile server.pwn
compile path/to/server.pwn
```

> **Компиляция с родительской директорией (автоматический include path)**

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

**Компиляция и запуск с указанием пути:**

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

**Установка зависимостей из `watchdogs.toml`:**

```yaml
replicate .
```

**Установка конкретного репозитория:**

```yaml
replicate repo/user
```

**Установка конкретной версии (теги):**

```yaml
replicate repo/user?v1.1
```

* **Автоматически — последняя версия**

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
# конкретное расположение
replicate repo/user --save ../parent/myproj
replicate repo/user --save myfolder/myproj
```