@echo off
setlocal

net session >nul 2>&1
if %errorLevel% neq 0 (
    echo Requesting administrator privileges...
    powershell -Command "Start-Process '%~f0' -Verb RunAs"
    exit /b
)

dir "%USERPROFILE%\Downloads"

echo.
echo Enter the path you want to switch to location in %USERPROFILE%\Downloads:
echo  ^^ example: my_folder my_project my_server
set /p TARGET_DIR=Path:

if not exist "%USERPROFILE%\Downloads\%TARGET_DIR%" (
    echo Directory not found: "%USERPROFILE%\Downloads\%TARGET_DIR%"
    echo Creating directory...

    mkdir "%USERPROFILE%\Downloads\%TARGET_DIR%"
    if errorlevel 1 (
        echo Failed to create directory: "%USERPROFILE%\Downloads\%TARGET_DIR%"
        pause
        exit /b 1
    )

    echo Successfully created directory: "%USERPROFILE%\Downloads\%TARGET_DIR%"
) else (
    echo Directory already exists: "%USERPROFILE%\Downloads\%TARGET_DIR%"
)

cd /d "%USERPROFILE%\Downloads\%TARGET_DIR%" || (
    echo Failed to change directory.
    pause
    exit /b 1
)

echo Now in: %CD%
echo.

powershell -Command "Invoke-WebRequest -Uri 'https://github.com/gskeleton/watchdogs/releases/download/DOG-260101-1.1/watchdogs.win' -OutFile 'watchdogs.win'"

if exist "dog" (
    rmdir /s /q "dog"
)

powershell -NoProfile -Command "Invoke-WebRequest 'https://github.com/gskeleton/libdog/archive/refs/heads/main.zip' -OutFile 'main.zip'"
powershell -NoProfile -Command "Expand-Archive 'main.zip' -DestinationPath 'dog' -Force"

cd /d dog\libdog-main || exit /b 1

if exist "C:\libdog" (
    rmdir /s /q "C:\libdog"
)

move /Y "libdog" "C:\"
move /Y "windows-native.cmd" "..\..\"

cd /d "%USERPROFILE%\Downloads\%TARGET_DIR%"

del /q "main.zip"
rmdir /s /q "dog"

.\windows-native.cmd

echo Done.
endlocal
pause
