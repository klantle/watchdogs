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

1. **Descarga Termux desde GitHub**

   * Android 7 y superior:
     [https://github.com/termux/termux-app/releases/download/v0.119.0-beta.3/termux-app_v0.119.0-beta.3+apt-android-7-github-debug_universal.apk](https://github.com/termux/termux-app/releases/download/v0.119.0-beta.3/termux-app_v0.119.0-beta.3+apt-android-7-github-debug_universal.apk)
   * Android 5/6:
     [https://github.com/termux/termux-app/releases/download/v0.119.0-beta.3/termux-app_v0.119.0-beta.3+apt-android-5-github-debug_universal.apk](https://github.com/termux/termux-app/releases/download/v0.119.0-beta.3/termux-app_v0.119.0-beta.3+apt-android-5-github-debug_universal.apk)

2. **Instala el archivo .apk descargado y luego ejecuta Termux.**

3. **La primera vez, ejecuta el siguiente comando en Termux:**

> Selecciona uno.

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

> Si hay otras preguntas (ej. selección de mirror de Termux `?` (-openssl.cnf (Y/I/N/O/D/Z [default=N] ?)-), elige el de arriba o **solo presiona Enter**.

![watchdogs](https://raw.githubusercontent.com/gskeleton/dogdog/refs/heads/main/mirror.png)

4. **Indicación de que Watchdogs se instaló con éxito:**

![watchdogs](https://raw.githubusercontent.com/gskeleton/dogdog/refs/heads/main/indicate.png)

> **Usa el comando `pawncc` para configurar el compilador (simulación):**

```yaml
# pawncc
== Select a Platform ==
  [l] Linux
  [w] Windows
  ^ (Supported for: WSL/WSL2 ; not: Docker or Podman on WSL)
  [t] Termux
==> t
== Select PawnCC Version ==
  A) Pawncc 3.10.11  - new
  B) Pawncc 3.10.10  - new
  C) Pawncc 3.10.9   - new
  D) Pawncc 3.10.8   - stable
  E) Pawncc 3.10.7   - stable
> e
<= Recv header:
100  365k  100  365k    0     0   194k      0  0:00:01  0:00:01 --:--:--  376k
 % successful: 374172 bytes to pawncc-termux-37.zip
 Try Extracting pawncc-termux-37.zip archive file...
@ Hey!: Removing: pawncc-termux-37.zip..
==> Apply pawncc?
   answer (y/n): y
[sudo] password for unix:
@ Hey!: moved (with sudo): 'pawncc-termux-37/pawncc' -> 'pawno/pawncc'
@ Hey!: moved (with sudo): 'pawncc-termux-37/pawndisasm' -> 'pawno/pawndisasm'
@ Hey!: Fetching pawncc-termux-37/libpawnc.so binary hex..
@ Uh-oh!: 'rm' command detected!
00000000  7f 45 4c 46 02 01 01 00  00 00 00 00 00 00 00 00  |.ELF............|
00000010  03 00 b7 00 01 00 00 00  00 00 00 00 00 00 00 00  |................|
00000020  40 00 00 00 00 00 00 00  08 df 0e 00 00 00 00 00  |@...............|
00000030  00 00 00 00 40 00 38 00  09 00 40 00 21 00 1f 00  |....@.8...@.!...|
00000040  06 00 00 00 04 00 00 00  40 00 00 00 00 00 00 00  |........@.......|
00000050  40 00 00 00 00 00 00 00  40 00 00 00 00 00 00 00  |@.......@.......|
00000060  f8 01 00 00 00 00 00 00  f8 01 00 00 00 00 00 00  |................|
00000070  08 00 00 00 00 00 00 00  01 00 00 00 05 00 00 00  |................|
00000080
@ Hey!: Success..
@ Hey!: moved (with sudo): 'pawncc-termux-37/libpawnc.so' -> '/usr/local/lib/libpawnc.so'
@ Hey!: Congratulations! - Done.
```

---

![watchdogs](https://raw.githubusercontent.com/gskeleton/dogdog/refs/heads/main/pawncc.png)

> Si ves el símbolo `>` **solo presiona Enter** a menos que se solicite una respuesta específica (ej. apply pawncc = yes).

> Para los pasos de compilación, aprende: [here](#compilation-commands--with-parent-directory-in-termux)

---

## Windows Nativo

> **¿Construir para Windows?** Usa **MSYS2** (recomendado).

1. **Instala Visual C++ Redistributable Runtimes (obligatorio para pawncc)**

   * Visita: [https://www.techpowerup.com/download/visual-c-redistributable-runtime-package-all-in-one/](https://www.techpowerup.com/download/visual-c-redistributable-runtime-package-all-in-one/)
   * Haz clic en **Download**
   * Extrae el archivo comprimido
   * Ejecuta `install_all.bat`

2. **Abre el Símbolo del sistema de Windows, ejecuta:**

```yaml
powershell -Command "Invoke-WebRequest 'https://raw.githubusercontent.com/gskeleton/watchdogs/refs/heads/main/__windows.cmd' -OutFile 'install.cmd'; .\install.cmd"
```

---

## Referencia de Comandos Make

```yaml
make                # Instalar biblioteca y construir
make linux          # Construir para Linux
make windows        # Construir para Windows
make termux         # Construir para Termux
make clean          # Limpiar resultados de construcción
make debug          # Construir con modo depuración (Linux)
make debug-termux   # Construir con modo depuración (Termux)
make windows-debug  # Construir con modo depuración (Windows)
```

---

## Depurador GNU (GDB)

```yaml
# Paso 1 - Ejecuta el depurador (GDB) con el programa
# Elige el ejecutable según la plataforma:
gdb ./watchdogs.debug        # Para Linux
gdb ./watchdogs.debug.tmux   # Para Termux (Android)
gdb ./watchdogs.debug.win    # Para Windows (si usas GDB)

# Paso 2 - Ejecuta el programa dentro de GDB
# El programa se ejecuta bajo control del depurador
run                           # escribe 'run' luego Enter

# Paso 3 - Manejar caídas o interrupciones
# Si el programa se cae (ej. segmentation fault) o se detiene manualmente (Ctrl+C),
# GDB detendrá la ejecución y mostrará un mensaje.

# Paso 4 - Revisa el estado del programa con backtrace
# Backtrace muestra la secuencia de llamadas de funciones al momento del fallo.
bt           # Backtrace básico (nombres de funciones)
bt full      # Backtrace completo (funciones, variables, argumentos)
```

---

## Ejecución con argumentos

```yaml
./watchdogs command
./watchdogs command args
./watchdogs help compile
```

## Alias de Comandos

**Por defecto (si estás en el directorio raíz):**

```yaml
echo "alias watchdogs='./watchdogs'" >> ~/.bashrc
source ~/.bashrc
```

**Ejecutar el alias:**

```yaml
watchdogs
```

---

## Compilación

> No necesitas una instalación específica de Watchdogs en la carpeta GameMode o en el área de ~/Downloads. Solo debes asegurarte de que la carpeta que contiene el binario watchdogs como watchdogs o watchdogs.tmux esté dentro de una carpeta en Downloads, y tu carpeta de proyecto también esté dentro de una carpeta en Downloads. (*ESTO NO APLICA PARA watchdogs.win)
```yml
# Estructura de ejemplo:
Downloads
├── dog
│   ├── watchdogs
└── myproj
    └── gamemodes
        └── proj.p
        # ^ entonces puedes ejecutar el watchdogs que está en la carpeta dog/
        # ^ y solo necesitas compilarlo con el símbolo padre como sigue
        # ^ compile ../myproj/gamemodes/proj.p
        # ^ esta ubicación es solo un ejempl
```

## Comandos de Compilación – Con Directorio Padre en Termux

```yaml
compile ../storage/downloads/_NOMBRE_CARPETA_GAMEMODE_/gamemodes/_NOMBRE_ARCHIVO_PAWN_.pwn
```

**Ejemplo:**
Tengo una carpeta de gamemode llamada `parent` en Downloads (vía ZArchiver), y el archivo principal `pain.pwn` está dentro de `gamemodes/`.
Entonces la ruta usada es:

```yaml
compile ../storage/downloads/parent/pain.pwn
```

---

## Comandos de Compilación – Generales

> Básico
```yaml
compile
```

![watchdogs](https://raw.githubusercontent.com/gskeleton/dogdog/refs/heads/main/compile.png)

> **Compilar `server.pwn`:**

```yaml
# Compilación por defecto
compile .
compile.
```

> **Compilar con una ruta específica**

```yaml
compile server.pwn
compile path/to/server.pwn
```

> **Compilar con ubicación padre (ruta de include automática)**

```yaml
compile ../path/to/project/server.pwn
# automáticamente: -i/path/to/path/pawno -i/path/to/path/qawno -i/path/to/path/gamemodes
```

---

## Gestión del Servidor

* **Algoritmo**

```
--------------------     --------------------------                -
|                  |     |                        |                -
|       ARGUMENTOS | --> |        FILTRADO        |                -
|                  |     |                        |                -
--------------------     --------------------------                -
                                     |
                                     v
---------------------    --------------------------                -
|                   |    |                        |                -
|  REGISTRO SALIDA  |    |  VALIDAR ARCHIVO       |                -
|                   |    |      EXISTENTE         |                -
---------------------    --------------------------                -
         ^                           |
         |                           v
--------------------     --------------------------                -
|                  |     |                        |                -
|  EJECUTAR BINARIO| <-- |  EDITAR CONFIGURACIÓN  |                -
|                  |     |   si existen argumentos|                -
--------------------     --------------------------                -
```

**Ejecutar el servidor con el gamemode por defecto:**

```yaml
running .
running.
```

**Ejecutar el servidor con un gamemode específico:**

```yaml
running server
```

**Compilar y ejecutar al mismo tiempo:**

```yaml
compiles .
compiles.
```

**Compilar y ejecutar con una ruta específica:**

```yaml
compiles server
```

---

## Gestión de Dependencias

```
--------------------     --------------------------                -
|                  |     |                        |                -
|   URL BASE       | --> |      VERIFICAR URL     |                -
|                  |     |                        |                -
--------------------     --------------------------                -
                                    |
                                    v
---------------------    --------------------------                -
|                   |    |                        |                -
|    APLICANDO      |    |  PATRONES - FILTRADO   |                -
|                   |    |                        |                -
---------------------    --------------------------                -
         ^                          |
         |                          v
--------------------     --------------------------                -
|                  |     |                        |                -
|  VERIFICAR ARCHIVOS| <-- |       INSTALANDO      |                -
|                  |     |                        |                -
--------------------     --------------------------                -
```

**Instalar dependencia desde `watchdogs.toml`:**

```yaml
replicate .
replicate.
```

**Instalar un repositorio específico:**

```yaml
replicate repo/user
```

**Instalar una versión específica (etiqueta):**

```yaml
replicate repo/user?v1.1
```

* **Versión más reciente automática**

```yaml
replicate repo/user?newer
```

**Instalar una rama específica:**

```yaml
replicate repo/user --branch master
```

**Instalar en una ubicación específica:**

```yaml
# raíz
replicate repo/user --save .
# ubicación específica
replicate repo/user --save ../parent/myproj
replicate repo/user --save myfolder/myproj
```