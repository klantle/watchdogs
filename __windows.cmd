@echo off
setlocal

dir "%USERPROFILE%\Downloads"

echo.

echo Enter the path you want to switch to:
echo "Enter the path you want to switch to location in %USERPROFILE%\Downloads:"
echo  ^^ example: my_folder
echo  ^^ a folder name for install; the folder doesn’t exist?, don’t worry..
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

curl -L -o watchdogs.win "https://github.com/gskeleton/watchdogs/releases/download/WG-251214/watchdogs.win"

if exist "watch" (
    rmdir /s /q "watch"
)

git clone --single-branch --branch main https://github.com/gskeleton/libwatchdogs watch

cd /d watch || exit /b 1

if exist "C:\libwatchdogs" (
    rmdir /s /q "C:\libwatchdogs"
)

move /Y "libwatchdogs" "C:\"
move /Y "windows-native.cmd" "..\"

cd ..

rmdir /s /q "watch"

.\windows-native.cmd

echo Done.
endlocal
pause
