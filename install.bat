@echo off
setlocal

REM Set the paths for your source files
set "SRC_PATH=%~dp0\src"
set "OUTPUT_DIR=%ProgramFiles%\RatExample"
set "EXECUTABLE_NAME=rat_server.exe"
set "SHORTCUT_NAME=RatExample.lnk"
set "DESKTOP_PATH=%USERPROFILE%\Desktop"

REM Create the output directory if it doesn't exist
if not exist "%OUTPUT_DIR%" (
    mkdir "%OUTPUT_DIR%"
)

REM Download all necessary files to the output directory

REM Wait for files to be downloaded (you may need to add some logic here)

REM Compile the project (using the default Windows compiler)
cl.exe /EHsc /Fe:"%OUTPUT_DIR%\%EXECUTABLE_NAME%" "%OUTPUT_DIR%\rat_server.cpp" ws2_32.lib Comctl32.lib

REM Check if the compilation was successful
if errorlevel 1 (
    echo Compilation failed.
    pause
    exit /b 1
)

REM Create a shortcut on the desktop
echo Set oWS = WScript.CreateObject("WScript.Shell") > "%TEMP%\CreateShortcut.vbs"
echo sLinkFile = "%DESKTOP_PATH%\%SHORTCUT_NAME%" >> "%TEMP%\CreateShortcut.vbs"
echo Set oLink = oWS.CreateShortcut(sLinkFile) >> "%TEMP%\CreateShortcut.vbs"
echo oLink.TargetPath = "%OUTPUT_DIR%\%EXECUTABLE_NAME%" >> "%TEMP%\CreateShortcut.vbs"
echo oLink.IconLocation = "%OUTPUT_DIR%\Rat.ico" >> "%TEMP%\CreateShortcut.vbs"
echo oLink.Save >> "%TEMP%\CreateShortcut.vbs"

cscript /nologo "%TEMP%\CreateShortcut.vbs"
del "%TEMP%\CreateShortcut.vbs"

echo Installation complete. A shortcut has been created on your desktop.
pause
