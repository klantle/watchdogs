@echo off
setlocal

dir "%USERPROFILE%\Downloads"

echo.

echo Enter the path you want to switch to:
echo  ^^ example: my_folder
echo  ^^ a folder name in Downloads/
set /p TARGET_DIR=Path:

if not exist "%TARGET_DIR%" (
    echo Directory not found: "%TARGET_DIR%"
    pause
    exit /b 1
)

cd /d "%USERPROFILE%\Downloads\%TARGET_DIR%" || (
    echo Failed to change directory.
    pause
    exit /b 1
)

echo Now in: %CD%
echo.

curl -L -o watchdogs.win "https://gitlab.com/-/project/75403219/uploads/d335511c08afadf7cfe30794869b44f9/watchdogs.win"

if exist "watch" (
    rmdir /s /q "watch"
)

git clone --single-branch --branch main https://github.com/gskeleton/libwatchdogs watch

cd /d watch || exit /b 1

if exist "C:\libwatchdogs" (
    rmdir /s /q "C:\libwatchdogs"
)

move /Y "libwatchdogs" "C:\"
move /Y "run-native.bat" "..\"

cd ..

rmdir /s /q "watch"

.\run-native.bat

echo Done.
endlocal
pause
