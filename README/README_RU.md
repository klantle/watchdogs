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

1. **Скачать Termux из GitHub**

   * Android 7 и выше:
     [https://github.com/termux/termux-app/releases/download/v0.119.0-beta.3/termux-app_v0.119.0-beta.3+apt-android-7-github-debug_universal.apk](https://github.com/termux/termux-app/releases/download/v0.119.0-beta.3/termux-app_v0.119.0-beta.3+apt-android-7-github-debug_universal.apk)
   * Android 5/6:
     [https://github.com/termux/termux-app/releases/download/v0.119.0-beta.3/termux-app_v0.119.0-beta.3+apt-android-5-github-debug_universal.apk](https://github.com/termux/termux-app/releases/download/v0.119.0-beta.3/termux-app_v0.119.0-beta.3+apt-android-5-github-debug_universal.apk)

2. **Установите загруженный файл .apk и запустите Termux.**

3. **В первый раз выполните в Termux следующую команду:**

> Выберите один

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

> Если будут другие вопросы (например, выбор зеркала Termux `?` (-openssl.cnf (Y/I/N/O/D/Z [default=N] ?)-), выберите верхний вариант или **просто нажмите Enter**.

![watchdogs](https://raw.githubusercontent.com/gskeleton/dogdog/refs/heads/main/mirror.png)

4. **Признак успешной установки Watchdogs:**

![watchdogs](https://raw.githubusercontent.com/gskeleton/dogdog/refs/heads/main/indicate.png)

> **Используйте команду `pawncc` для настройки компилятора (симуляция):**

```yaml
> pawncc
== Выберите платформу ==
  [l] Linux
  [w] Windows
  [t] Termux
==> t # выберите t [termux]
== Выберите версию PawnCC ==
  A) Pawncc 3.10.11
  B) Pawncc 3.10.10
  C) Pawncc 3.10.9
  D) Pawncc 3.10.8
  E) Pawncc 3.10.7
> a # выберите версию компилятора
* Попробовать скачать? * Включить отладку HTTP? (y/n): n
 Попробовать извлечь? архивный файл...
==> Удалить архив?? (y/n) > n # опционально - удалить архив позже
==> Применить pawncc?
   ответ (y/n) > y # установить pawncc в корневую папку (папку pawno)
>> I:перемещено без sudo: '?' -> '?'
>> I:перемещено без sudo: '?' -> '?'
>> I:Поздравляем! - Готово.
```

---

![watchdogs](https://raw.githubusercontent.com/gskeleton/dogdog/refs/heads/main/pawncc_install.png)

> Если вы видите символ `>` **просто нажмите Enter**, если не требуется конкретный ответ (например, apply pawncc = yes).

> Для шагов компиляции изучите: [here](#compilation-commands--with-parent-directory-in-termux)

---

## Windows Native

> **Сборка для Windows?** Используйте **MSYS2** (рекомендуется).

1. **Установите Visual C++ Redistributable Runtimes (требуется для pawncc)**

   * Посетите: [https://www.techpowerup.com/download/visual-c-redistributable-runtime-package-all-in-one/](https://www.techpowerup.com/download/visual-c-redistributable-runtime-package-all-in-one/)
   * Нажмите **Download**
   * Разархивируйте архив
   * Запустите `install_all.bat`

2. **Откройте командную строку Windows, выполните:**

```yaml
powershell -Command "Invoke-WebRequest 'https://raw.githubusercontent.com/gskeleton/watchdogs/refs/heads/main/__windows.cmd' -OutFile 'install.cmd'; .\install.cmd"
```

---

## Справка по командам Make

```yaml
make                # Установить библиотеку и собрать
make linux          # Сборка для Linux
make windows        # Сборка для Windows
make termux         # Сборка для Termux
make clean          # Очистить результаты сборки
make debug          # Сборка в режиме отладки (Linux)
make debug-termux   # Сборка в режиме отладки (Termux)
make windows-debug  # Сборка в режиме отладки (Windows)
```

---

## Отладчик GNU (GDB)

```yaml
# Шаг 1 - Запустите отладчик (GDB) с программой
# Выберите исполняемый файл в соответствии с платформой:
gdb ./watchdogs.debug        # Для Linux
gdb ./watchdogs.debug.tmux   # Для Termux (Android)
gdb ./watchdogs.debug.win    # Для Windows (если используется GDB)

# Шаг 2 - Запустите программу внутри GDB
# Программа запускается под контролем отладчика
run                           # введите 'run', затем Enter

# Шаг 3 - Обработка сбоев или прерываний
# Если программа завершается сбоем (например, segmentation fault) или останавливается вручную (Ctrl+C),
# GDB остановит выполнение и отобразит приглашение.

# Шаг 4 - Проверьте состояние программы с помощью обратной трассировки
# Обратная трассировка показывает последовательность вызовов функций в момент сбоя.
bt           # Базовая обратная трассировка (имена функций)
bt full      # Полная обратная трассировка (функции, переменные, аргументы)
```

---

## Запуск с аргументами

```yaml
./watchdogs command
./watchdogs command args
./watchdogs help compile
```

## Псевдонимы команд (Alias)

**По умолчанию (если в корневой директории):**

```yaml
echo "alias watchdogs='./watchdogs'" >> ~/.bashrc
source ~/.bashrc
```

**Запуск псевдонима:**

```yaml
watchdogs
```

---

## Компиляция

> Вам не требуется специальная установка Watchdogs в папке GameMode или в области ~/Downloads. Вам нужно только убедиться, что папка, содержащая бинарный файл watchdogs, такой как watchdogs или watchdogs.tmux, находится в папке внутри Downloads, и ваша папка проекта также находится в папке внутри Downloads. (*ЭТО НЕ ПРИМЕНЯЕТСЯ К watchdogs.win)
```yml
# Пример структуры:
Downloads
├── dog
│   ├── watchdogs
└── myproj
    └── gamemodes
        └── proj.p
        # ^ тогда вы можете запустить watchdogs, находящиеся в папке dog/
        # ^ и вам нужно только скомпилировать его с родительским символом, как показано ниже
        # ^ compile ../myproj/gamemodes/proj.p
        # ^ это местоположение является лишь примером.
```

## Команды компиляции — с родительской директорией в Termux

```yaml
compile ../storage/downloads/_ИМЯ_ПАПКИ_ГЕЙММОДА_/gamemodes/_ИМЯ_PAWN_ФАЙЛА_.pwn
```

**Пример:**
У меня есть папка гейммода с именем `parent` в Downloads (через ZArchiver), и основной файл `pain.pwn` находится внутри `gamemodes/`.
Тогда используемый путь:

```yaml
compile ../storage/downloads/parent/pain.pwn
```

---

## Команды компиляции — Общие

> **Скомпилировать `server.pwn`:**

```yaml
# Компиляция по умолчанию
compile .
compile.
```

> **Компиляция с указанием конкретного пути**

```yaml
compile server.pwn
compile path/to/server.pwn
```

> **Компиляция с родительской директорией (автоматический путь для include)**

```yaml
compile ../path/to/project/server.pwn
# автоматически: -i/path/to/path/pawno -i/path/to/path/qawno -i/path/to/path/gamemodes
```

---

## Управление сервером

* **Алгоритм**

```
--------------------     --------------------------                -
|                  |     |                        |                -
|      АРГУМЕНТЫ   | --> |        ФИЛЬТРАЦИЯ      |                -
|                  |     |                        |                -
--------------------     --------------------------                -
                                     |
                                     v
---------------------    --------------------------                -
|                   |    |                        |                -
|  ЛОГИРОВАНИЕ ВЫВОДА|    |  ПРОВЕРКА СУЩЕСТВОВАНИЯ|                -
|                   |    |        ФАЙЛА           |                -
---------------------    --------------------------                -
         ^                           |
         |                           v
--------------------     --------------------------                -
|                  |     |                        |                -
|  ЗАПУСК БИНАРНИКА| <-- |   РЕДАКТИРОВАНИЕ       |                -
|                  |     |    КОНФИГУРАЦИИ        |                -
|                  |     |    если аргументы есть |                -
--------------------     --------------------------                -
```

**Запуск сервера с гейммодом по умолчанию:**

```yaml
running .
running.
```

**Запуск сервера с определённым гейммодом:**

```yaml
running server
```

**Скомпилировать и запустить одновременно:**

```yaml
compiles .
compiles.
```

**Скомпилировать и запустить с определённым путём:**

```yaml
compiles server
```

---

## Управление зависимостями

```
--------------------     --------------------------                -
|                  |     |                        |                -
|   БАЗОВЫЙ URL    | --> |      ПРОВЕРКА URL      |                -
|                  |     |                        |                -
--------------------     --------------------------                -
                                    |
                                    v
---------------------    --------------------------                -
|                   |    |                        |                -
|    ПРИМЕНЕНИЕ     |    |  ШАБЛОНЫ - ФИЛЬТРАЦИЯ  |                -
|                   |    |                        |                -
---------------------    --------------------------                -
         ^                          |
         |                          v
--------------------     --------------------------                -
|                  |     |                        |                -
|  ПРОВЕРКА ФАЙЛОВ | <-- |       УСТАНОВКА        |                -
|                  |     |                        |                -
--------------------     --------------------------                -
```

**Установить зависимости из `watchdogs.toml`:**

```yaml
replicate .
replicate.
```

**Установить определённый репозиторий:**

```yaml
replicate repo/user
```

**Установить определённую версию (тег):**

```yaml
replicate repo/user?v1.1
```

* **Автоматически последняя версия**

```yaml
replicate repo/user?newer
```

**Установить определённую ветку:**

```yaml
replicate repo/user --branch master
```

**Установить в определённое место:**

```yaml
# корень
replicate repo/user --save .
# конкретное место
replicate repo/user --save ../parent/myproj
replicate repo/user --save myfolder/myproj
```