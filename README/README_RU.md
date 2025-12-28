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

> Мы настоятельно рекомендуем использовать дистрибутив Termux напрямую с GitHub, а не из Google Play Store, чтобы обеспечить совместимость с последними функциями Termux и наслаждаться свободой, предлагаемой вне Play Store. https://github.com/termux/termux-app/releases
> Просто вставьте это в ваш терминал и запустите.

- [x] GNU/wget
```yml
wget -O install.sh https://github.com/gskeleton/watchdogs/raw/refs/heads/main/__termux.sh && chmod +x install.sh && ./install.sh
```
- [x] cURL
```yml
curl -L -o install.sh https://github.com/gskeleton/watchdogs/raw/refs/heads/main/__termux.sh && chmod +x install.sh && ./install.sh
```

## Native

> Для сборки Windows? Используйте MSYS2 (рекомендуется) (pacman -S make && make && make windows)

1. Сначала установите Visual C++ Redistributable Runtimes All-in-One (необходимо для pawncc)
- Перейдите на https://www.techpowerup.com/download/visual-c-redistributable-runtime-package-all-in-one/
- Нажмите "Download"
- Распакуйте архив
- Просто запустите "install_all.bat".
2. Откройте командную строку Windows, выполните:
```yml
powershell -Command "Invoke-WebRequest 'https://raw.githubusercontent.com/gskeleton/watchdogs/refs/heads/main/__win.cmd' -OutFile 'install.cmd'; .\install.cmd"
```

## Справочник команд Make

```yaml
make                # Установить библиотеки и собрать
make linux          # Собрать для Linux
make windows        # Собрать для Windows
make termux         # Собрать для Termux
make clean          # Очистить артефакты сборки
make debug          # Собрать с режимом отладки (Linux)
make debug-termux   # Собрать с режимом отладки (Termux)
make windows-debug  # Собрать с режимом отладки (Windows)
```

## GNU Debugger (GDB)

```yaml
# Шаг 1 - Запустите отладчик (GDB) с вашей программой
# Выберите правильный исполняемый файл в зависимости от вашей платформы:
gdb ./watchdogs.debug        # Для Linux
gdb ./watchdogs.debug.tmux   # Для Termux (Android - Termux)
gdb ./watchdogs.debug.win    # Для Windows (если используете GDB на Windows)

# Шаг 2 - Запустите программу внутри GDB
# Это запускает программу под контролем отладчика.
run                           # Просто введите 'run' и нажмите Enter

# Шаг 3 - Обработка сбоев или прерываний
# Если программа аварийно завершается (например, segmentation fault) или вы прерываете её вручную (Ctrl+C),
# GDB приостановит выполнение и покажет приглашение.

# Шаг 4 - Изучите состояние программы с помощью backtrace
# Backtrace показывает стек вызовов функций в точке сбоя.
bt           # Базовый backtrace (показывает имена функций)
bt full      # Полный backtrace (показывает имена функций, переменные и аргументы)
```

## Псевдонимы команд

По умолчанию (если в корневом каталоге):
```yaml
echo "alias watchdogs='./watchdogs'" >> ~/.bashrc
source ~/.bashrc
```

И запустите псевдоним:
```yml
watchdogs
```

## Руководство по использованию

## Команды компиляции

> Компиляция `server.pwn`:
```yaml
# Компиляция по умолчанию
compile .
```
> Компиляция с указанием конкретного пути к файлу
```yml
compile server.pwn
compile путь/к/server.pwn
```
> Компиляция с родительским расположением - автоматическое добавление пути include
```yaml
compile ../путь/к/project/server.pwn
# -: -i/путь/к/path/pawno -i/путь/к/path/qawno -i/путь/к/path/gamemodes
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

**Запуск сервера с игровым режимом по умолчанию:**
```yaml
running .
```

**Запуск сервера с конкретным игровым режимом:**
```yaml
running server
```

**Компиляция и запуск одной командой:**
```yaml
compiles
```

**Компиляция и запуск с указанием пути:**
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

**Установка конкретной версии (теги):**
```yaml
replicate repo/user?v1.1
```
- Автоматически последняя
```yaml
replicate repo/user?newer
```

**Установка конкретной ветки:**
```yaml
replicate repo/user --branch master
```

**Установка в конкретное расположение:**
```yaml
# корень
replicate repo/user --save .
# конкретное
replicate repo/user --save ../parent/myproj
replicate repo/user --save myfol/myproj
```