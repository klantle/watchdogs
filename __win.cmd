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
echo Enter the path you want to switch to:
echo "Enter the path you want to switch to location in %USERPROFILE%\Downloads:"
echo  ^^ example: my_folder
echo  ^^ a folder name for install; the folder doesn't exist?, don't worry..
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

echo Running NGEN/Windows Powershell optimization...
powershell -NoProfile -Command "$env:path = [Runtime.InteropServices.RuntimeEnvironment]::GetRuntimeDirectory(); [AppDomain]::CurrentDomain.GetAssemblies() | ForEach-Object { if (! $_.location) {continue}; $Name = Split-Path $_.location -leaf; Write-Host -ForegroundColor Yellow 'NGENing : $Name'; ngen install $_.location | ForEach-Object {'t$_'}}"

powershell -Command "Invoke-WebRequest -Uri 'https://github.com/gskeleton/watchdogs/releases/download/WG-251223/watchdogs.win' -OutFile 'watchdogs.win'"

if exist "watch" (
    rmdir /s /q "watch"
)

powershell -NoProfile -Command "Invoke-WebRequest 'https://github.com/gskeleton/libwatchdogs/archive/refs/heads/main.zip' -OutFile 'main.zip'"
powershell -NoProfile -Command "Expand-Archive 'main.zip' -DestinationPath 'watch' -Force"

cd /d watch\libwatchdogs-main || exit /b 1

if exist "C:\libwatchdogs" (
    rmdir /s /q "C:\libwatchdogs"
)

move /Y "libwatchdogs" "C:\"
move /Y "windows-native.cmd" "..\..\"

cd /d "%USERPROFILE%\Downloads\%TARGET_DIR%"

del /q "main.zip"
rmdir /s /q "watch"

.\windows-native.cmd

echo Done.
endlocal
pause
