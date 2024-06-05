@echo off
setlocal

REM Set the paths for your source files and output directory
set "SRC_PATH=%~dp0\src"
set "OUTPUT_DIR=%ProgramFiles%\RatExample"
set "EXECUTABLE_NAME=rat_server.exe"
set "SHORTCUT_NAME=RatExample.lnk"
set "DESKTOP_PATH=%USERPROFILE%\Desktop"

REM Create the output directory if it doesn't exist
if not exist "%OUTPUT_DIR%" (
    mkdir "%OUTPUT_DIR%"
)

REM Check if a C++ compiler is available
if not exist "%VS140COMNTOOLS%" (
    echo Visual C++ Compiler not found. Installing Visual Studio Build Tools...
    REM You can add commands here to download and install the Visual Studio Build Tools or another compatible compiler.
    REM Once installed, the user can set the environment variables accordingly.
    REM Example: https://aka.ms/vs/16/release/vs_buildtools.exe
    pause
    exit /b 1
)

REM Compile the project using the default compiler
cl.exe /EHsc /Fe:"%OUTPUT_DIR%\%EXECUTABLE_NAME%" "%SRC_PATH%\rat_server.cpp" ws2_32.lib Comctl32.lib

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
echo oLink.Save >> "%TEMP%\CreateShortcut.vbs"

cscript /nologo "%TEMP%\CreateShortcut.vbs"
del "%TEMP%\CreateShortcut.vbs"

echo Installation complete. A shortcut has been created on your desktop.
pause
