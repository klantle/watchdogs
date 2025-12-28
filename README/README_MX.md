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

> Recomendamos encarecidamente utilizar la distribución de Termux directamente desde GitHub en lugar de la Google Play Store para garantizar la compatibilidad con las últimas funciones de Termux y disfrutar de la libertad que ofrece fuera de Play Store. https://github.com/termux/termux-app/releases
> Simplemente arrastra esto a tu terminal y ejecútalo.

- [x] GNU/wget
```yml
wget -O install.sh https://github.com/gskeleton/watchdogs/raw/refs/heads/main/__termux.sh && chmod +x install.sh && ./install.sh
```
- [x] cURL
```yml
curl -L -o install.sh https://github.com/gskeleton/watchdogs/raw/refs/heads/main/__termux.sh && chmod +x install.sh && ./install.sh
```

## Native

> ¿Para la compilación de Windows? Usa MSYS2 Recomendado (pacman -S make && make && make windows)

1. Instalar primero Visual C++ Redistributable Runtimes All-in-One (necesario para pawncc)
- Ve a https://www.techpowerup.com/download/visual-c-redistributable-runtime-package-all-in-one/
- Haz clic en "Download"
- Extrae el Archivo
- Solo ejecuta el "install_all.bat".
2. Abre el Símbolo del sistema de Windows, Ejecuta:
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
make debug          # Compilar con modo de depuración (Linux)
make debug-termux   # Compilar con modo de depuración (Termux)
make windows-debug  # Compilar con modo de depuración (Windows)
```

## GNU Debugger (GDB)

```yaml
# Paso 1 - Inicia el depurador (GDB) con tu programa
# Elige el ejecutable correcto dependiendo de tu plataforma:
gdb ./watchdogs.debug        # Para Linux
gdb ./watchdogs.debug.tmux   # Para Termux (Android - Termux)
gdb ./watchdogs.debug.win    # Para Windows (si usas GDB en Windows)

# Paso 2 - Ejecuta el programa dentro de GDB
# Esto inicia el programa bajo el control del depurador.
run                           # Solo escribe 'run' y presiona Enter

# Paso 3 - Manejando fallos o interrupciones
# Si el programa falla (ej., segmentation fault) o lo interrumpes manualmente (Ctrl+C),
# GDB pausará la ejecución y mostrará un prompt.

# Paso 4 - Inspecciona el estado del programa con un backtrace
# Un backtrace muestra la pila de llamadas de funciones en el punto del fallo.
bt           # Backtrace básico (muestra nombres de funciones)
bt full      # Backtrace completo (muestra nombres de funciones, variables y argumentos)
```

## Alias de Comandos

Por defecto (si estás en el directorio raíz):
```yaml
echo "alias watchdogs='./watchdogs'" >> ~/.bashrc
source ~/.bashrc
```

Y ejecuta el alias:
```yml
watchdogs
```

## Guía de Uso

## Comandos de Compilación

> Compilar `server.pwn`:
```yaml
# Compilación Predeterminada
compile .
```
> Compilar con ruta de archivo específica
```yml
compile server.pwn
compile ruta/a/server.pwn
```
> Compilar con ubicación padre - ruta de inclusión automática
```yaml
compile ../ruta/al/proyecto/server.pwn
# -: -i/ruta/a/ruta/pawno -i/ruta/a/ruta/qawno -i/ruta/a/ruta/gamemodes
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

**Iniciar servidor con modo de juego predeterminado:**
```yaml
running .
```

**Iniciar servidor con modo de juego específico:**
```yaml
running server
```

**Compilar e iniciar en un solo comando:**
```yaml
compiles
```

**Compilar e iniciar con ruta específica:**
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

**Instalar repositorio específico:**
```yaml
replicate repo/user
```

**Instalar versión específica (etiquetas):**
```yaml
replicate repo/user?v1.1
```
- Automáticamente la última
```yaml
replicate repo/user?newer
```

**Instalar rama específica:**
```yaml
replicate repo/user --branch master
```

**Instalar ubicación específica:**
```yaml
# raíz
replicate repo/user --save .
# específico
replicate repo/user --save ../parent/myproj
replicate repo/user --save myfol/myproj
```