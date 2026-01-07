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

1. **Descarga Termux desde GitHub**

   * Android 7 o superior:
     [https://github.com/termux/termux-app/releases/download/v0.119.0-beta.3/termux-app_v0.119.0-beta.3+apt-android-7-github-debug_universal.apk](https://github.com/termux/termux-app/releases/download/v0.119.0-beta.3/termux-app_v0.119.0-beta.3+apt-android-7-github-debug_universal.apk)
   * Android 5/6:
     [https://github.com/termux/termux-app/releases/download/v0.119.0-beta.3/termux-app_v0.119.0-beta.3+apt-android-5-github-debug_universal.apk](https://github.com/termux/termux-app/releases/download/v0.119.0-beta.3/termux-app_v0.119.0-beta.3+apt-android-5-github-debug_universal.apk)

2. **Instala el archivo .apk descargado y ejecuta Termux.**

3. **Primero, ejecuta el siguiente comando en Termux:**

```yaml
curl -L -o install.sh https://github.com/gskeleton/watchdogs/raw/refs/heads/main/__termux.sh && chmod +x install.sh && ./install.sh
```

4. **Si aparece un prompt como el siguiente (simulación):**

```yaml
Enter the path you want to switch to location in storage/downloads
  ^ ejemplo: my_folder my_project my_server
  ^ nombre de carpeta para la instalación; si no existe, no hay problema
  ^ presiona Enter si deseas instalar watchdogs en home (recomendado)
```

> **Presiona Enter sin escribir nada.**
> Si aparecen otras preguntas (como selección de mirror de Termux), elige la primera opción o **solo presiona Enter**.

5. **Indicador de que Watchdogs se instaló correctamente:**

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

> **Usa el comando `pawncc` para preparar el compilador (simulación):**

```yaml
> pawncc
== Select a Platform ==
  [l] Linux
  [w] Windows
  [t] Termux
==> t # selecciona t [termux]
== Select PawnCC Version ==
  A) Pawncc 3.10.11
  B) Pawncc 3.10.10
  C) Pawncc 3.10.9
  D) Pawncc 3.10.8
  E) Pawncc 3.10.7
> a # selecciona la versión del compilador
* Try Downloading ? * Enable HTTP debugging? (y/n): n
 Try Extracting ? archive file...
==> Remove archive ?? (y/n) > n # opcional - eliminar el archivo más tarde
==> Apply pawncc?
   answer (y/n) > y # aplicar pawncc al root (instalar en la carpeta pawno)
>> I:moved without sudo: '?' -> '?'
>> I:moved without sudo: '?' -> '?'
>> I:Congratulations! - Done.
```

> Si ves el símbolo `>` en color cyan/gris/azul (puede variar según el tema), **simplemente presiona Enter**, excepto cuando se solicite una respuesta específica (por ejemplo: apply pawncc = yes).

> Para los pasos de compilación, revisa:
> [https://github.com/gskeleton/watchdogs/blob/main/README/README_ID.md#usage-guide](https://github.com/gskeleton/watchdogs/blob/main/README/README_ID.md#usage-guide)

---

## Native

> **¿Compilar para Windows?** Usa **MSYS2** (recomendado).

1. **Instala Visual C++ Redistributable Runtimes (obligatorio para pawncc)**

   * Visita: [https://www.techpowerup.com/download/visual-c-redistributable-runtime-package-all-in-one/](https://www.techpowerup.com/download/visual-c-redistributable-runtime-package-all-in-one/)
   * Haz clic en **Download**
   * Extrae el archivo
   * Ejecuta `install_all.bat`

2. **Abre el Command Prompt de Windows y ejecuta:**

```yaml
powershell -Command "Invoke-WebRequest 'https://raw.githubusercontent.com/gskeleton/watchdogs/refs/heads/main/__win.cmd' -OutFile 'install.cmd'; .\install.cmd"
```

---

## Make Command Reference

```yaml
make                # Instalar librerías y compilar
make linux          # Compilar para Linux
make windows        # Compilar para Windows
make termux         # Compilar para Termux
make clean          # Limpiar archivos de compilación
make debug          # Compilar en modo debug (Linux)
make debug-termux   # Compilar en modo debug (Termux)
make windows-debug  # Compilar en modo debug (Windows)
```

---

## GNU Debugger (GDB)

```yaml
# Paso 1 - Iniciar el depurador (GDB) con el programa
# Elige el ejecutable correcto según la plataforma:
gdb ./watchdogs.debug        # Para Linux
gdb ./watchdogs.debug.tmux   # Para Termux (Android)
gdb ./watchdogs.debug.win    # Para Windows (si usas GDB)

# Paso 2 - Ejecutar el programa dentro de GDB
# El programa se ejecuta bajo el control del depurador
run                           # escribe 'run' y presiona Enter

# Paso 3 - Manejo de crashes o interrupciones
# Si el programa falla (por ejemplo, segmentation fault) o se detiene manualmente (Ctrl+C),
# GDB detendrá la ejecución y mostrará el prompt.

# Paso 4 - Revisar el estado del programa con backtrace
# Backtrace muestra la pila de llamadas en el punto del fallo.
bt           # Backtrace básico (nombres de funciones)
bt full      # Backtrace completo (funciones, variables y argumentos)
```

---

## Command Alias

**Por defecto (si estás en el directorio root):**

```yaml
echo "alias watchdogs='./watchdogs'" >> ~/.bashrc
source ~/.bashrc
```

**Ejecutar el alias:**

```yaml
watchdogs
```

---

## Compilation Commands – With Parent Directory in Termux

```yaml
compile ../storage/downloads/_GAMEMODE_FOLDER_NAME_/gamemodes/_PAWN_FILE_NAME_.pwn
```

**Ejemplo:**
Tengo una carpeta de gamemode llamada `parent` en Downloads (usando ZArchiver), y el archivo principal `pain.pwn` está dentro de `gamemodes/`.
Entonces el path que se usa es:

```yaml
compile ../storage/downloads/parent/pain.pwn
```

---

## Compilation Commands – General

> **Compilar `server.pwn`:**

```yaml
# Compilación por defecto
compile .
```

> **Compilar con una ruta específica**

```yaml
compile server.pwn
compile path/to/server.pwn
```

> **Compilar con ubicación padre (include path automático)**

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

**Iniciar el servidor con el gamemode por defecto:**

```yaml
running .
```

**Iniciar el servidor con un gamemode específico:**

```yaml
running server
```

**Compilar y ejecutar en un solo comando:**

```yaml
compiles
```

**Compilar y ejecutar con una ruta específica:**

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

**Instalar dependencias desde `watchdogs.toml`:**

```yaml
replicate .
```

**Instalar un repositorio específico:**

```yaml
replicate repo/user
```

**Instalar una versión específica (tags):**

```yaml
replicate repo/user?v1.1
```

* **Automáticamente la versión más reciente**

```yaml
replicate repo/user?newer
```

**Instalar una branch específica:**

```yaml
replicate repo/user --branch master
```

**Instalar en una ubicación específica:**

```yaml
# root
replicate repo/user --save .
# ubicación específica
replicate repo/user --save ../parent/myproj
replicate repo/user --save myfolder/myproj
```
