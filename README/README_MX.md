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

1. Descargar Termux desde GitHub<br>
   - Android 7 o superior:<br>
   [https://github.com/termux/termux-app/releases/download/v0.119.0-beta.3/termux-app_v0.119.0-beta.3+apt-android-7-github-debug_universal.apk](https://github.com/termux/termux-app/releases/download/v0.119.0-beta.3/termux-app_v0.119.0-beta.3+apt-android-7-github-debug_universal.apk)<br>
   - Android 5/6:<br>
   [https://github.com/termux/termux-app/releases/download/v0.119.0-beta.3/termux-app_v0.119.0-beta.3+apt-android-5-github-debug_universal.apk](https://github.com/termux/termux-app/releases/download/v0.119.0-beta.3/termux-app_v0.119.0-beta.3+apt-android-5-github-debug_universal.apk)
2. El archivo .apk que ya fue descargado, instálalo y luego ejecuta Termux.
3. Primero, ejecuta el siguiente comando dentro de Termux ya instalado:

```yml
curl -L -o install.sh https://github.com/gskeleton/watchdogs/raw/refs/heads/main/__termux.sh && chmod +x install.sh && ./install.sh
```

4. Si encuentras una pregunta como la siguiente (Simulación):

```yml
Enter the path you want to switch to location in storage/downloads
  ^ example my_folder my_project my_server
  ^ enter if you want install the watchdogs in home (recommended)..
```

> Presiona Enter sin escribir nada. y si aparece cualquier otra pregunta, como la selección del mirror de termux, elige el primero o all (solo presiona enter).

1. Indicación de que Watchdogs se instaló correctamente, como se muestra a continuación:

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

> Por favor usa el comando `pawncc` para preparar la herramienta de compilación (Simulación):

```yml
~ pawncc # ejecutar
== Select a Platform ==
  [l] Linux
  [w] Windows
  [t] Termux
==> t # elegir t [termux]
== Select PawnCC Version ==
  A) Pawncc 3.10.11
  B) Pawncc 3.10.10
  C) Pawncc 3.10.9
  D) Pawncc 3.10.8
  E) Pawncc 3.10.7
> a # elegir versión del compilador
* Try Downloading ? * Enable HTTP debugging? (y/n): n
** Downloading... 100% Done! % successful: 417290 bytes to ?
 Try Extracting ? archive file...
==> Remove archive ?? (y/n) > n # libre - eliminar el archivo más tarde
==> Apply pawncc?
   answer (y/n) > y # aplicar pawncc en el root - instalarlo dentro de la carpeta pawno/ ; solo presiona enter o escribe y y enter
> I: moved without sudo: '?' -> '?'
> I: moved without sudo: '?' -> '?'
> I: Congratulations! - Done.
```

> Y si encuentras el símbolo `>` de color cian/gris/azul (visual diferente según el entorno), simplemente presiona enter sin escribir nada. excepto cuando sea necesario, como en apply pawncc? sí.<br>
> Para los pasos de compilación, por favor estudia: [https://github.com/gskeleton/watchdogs/blob/main/README/README_ID.md#panduan-penggunaan](https://github.com/gskeleton/watchdogs/blob/main/README/README_ID.md#panduan-penggunaan)

## Native

> ¿Para compilar en Windows? Usa MSYS2 recomendado (pacman -S make && make && make windows)

1. Instala primero Visual C++ Redistributable Runtimes All-in-One (necesario para pawncc)

* Visita [https://www.techpowerup.com/download/visual-c-redistributable-runtime-package-all-in-one/](https://www.techpowerup.com/download/visual-c-redistributable-runtime-package-all-in-one/)
* Haz clic en "Download"
* Extrae el archivo
* Ejecuta simplemente "install_all.bat".

2. Abre el Command Prompt de Windows y ejecuta:

```yml
powershell -Command "Invoke-WebRequest 'https://raw.githubusercontent.com/gskeleton/watchdogs/refs/heads/main/__win.cmd' -OutFile 'install.cmd'; .\install.cmd"
```

## Referencia de Comandos Make

```yaml
make                # Instalar librerías y compilar
make linux          # Compilar para Linux
make windows        # Compilar para Windows
make termux         # Compilar para Termux
make clean          # Limpiar artefactos de compilación
make debug          # Compilar en modo debug (Linux)
make debug-termux   # Compilar en modo debug (Termux)
make windows-debug  # Compilar en modo debug (Windows)
```

## GNU Debugger (GDB)

```yaml
# Paso 1 - Iniciar el depurador (GDB) con tu programa
# Elige el ejecutable correcto según tu plataforma:
gdb ./watchdogs.debug        # Para Linux
gdb ./watchdogs.debug.tmux   # Para Termux (Android - Termux)
gdb ./watchdogs.debug.win    # Para Windows (si usas GDB en Windows)

# Paso 2 - Ejecutar el programa dentro de GDB
# Esto inicia el programa bajo el control del depurador.
run                           # Escribe 'run' y presiona Enter

# Paso 3 - Manejar crash o interrupciones
# Si el programa se cae (por ejemplo: segmentation fault) o lo detienes manualmente (Ctrl+C),
# GDB detendrá la ejecución y mostrará el prompt.

# Paso 4 - Revisar el estado del programa con backtrace
# Backtrace muestra la pila de llamadas de funciones en el punto del crash.
bt           # Backtrace básico (muestra nombres de funciones)
bt full      # Backtrace completo (muestra nombres de funciones, variables y argumentos)
```

## Alias de Comando

Por defecto (si estás en el directorio root):

```yaml
echo "alias watchdogs='./watchdogs'" >> ~/.bashrc
source ~/.bashrc
```

Y ejecuta el alias:

```yml
watchdogs
```

## Guía de Uso

## Comando de Compilación - Con directorio padre en Termux

```yml
~ compile ../storage/downloads/_NAMA_FOLDER_GAMEMODE_/gamemodes/_NAMA_FILE_PAWN_.pwn
```

Por ejemplo, tengo una carpeta de gamemode llamada induk en ZArchiver exactamente en la ubicación Downloads
y tengo solo el archivo principal del proyecto como pain.pwn que está en gamemodes/ dentro de la carpeta induk que está en Downloads.
entonces, por supuesto, usaré la carpeta induk como la ubicación después de Downloads y pain.pwn después de gamemodes.

```yml
~ compile ../storage/downloads/induk/pain.pwn
```

## Comando de Compilación - General

> Compilar `server.pwn`:

```yaml
# Compilación por defecto
compile .
```

> Compilación con ruta de archivo específica

```yml
compile server.pwn
compile path/ke/server.pwn
```

> Compilación con ubicación padre - rutas include automáticas

```yaml
compile ../path/ke/project/server.pwn
# -: -i/path/ke/path/pawno -i/path/ke/path/qawno -i/path/ke/path/gamemodes
```

## Gestión del Servidor

* **Algoritmo**

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

**Iniciar el servidor con el gamemode por defecto:**

```yaml
running .
```

**Iniciar el servidor con un gamemode específico:**

```yaml
running server
```

**Compilar e iniciar en un solo comando:**

```yaml
compiles
```

**Compilar e iniciar con una ruta específica:**

```yml
compiles server
```

## Gestión de Dependencias

* **Algoritmo**

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

* Automáticamente la más reciente

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
# específica
replicate repo/user --save ../parent/myproj
replicate repo/user --save myfol/myproj
```
