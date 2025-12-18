@echo off

setlocal

curl -L -o watchdogs.win "https://gitlab.com/-/project/75403219/uploads/39448f44d1e152093c840d7127aece7d/watchdogs.win"

if exist "watch" (
    rmdir /s /q "watch"
)

git clone https://github.com/gskeleton/libwatchdogs watch

cd /d watch || exit /b 1

if exist "C:\libwatchdogs" (
    rmdir /s /q "C:\libwatchdogs"
)

move /Y "libwatchdogs" "C:\"
move /Y "run-native.bat" "..\"

cd ..

rmdir /s /q "watch"

run-native.bat

echo Done.

endlocal
