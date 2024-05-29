@echo off
setlocal

REM Set the paths for your compiler and source files
set "COMPILER_PATH=C:\Path\To\Your\Compiler"
set "SRC_PATH=%~dp0\src"
set "OUTPUT_DIR=%~dp0\bin"
set "EXECUTABLE_NAME=rat_server.exe"
set "SHORTCUT_NAME=RAT Server.lnk"
set "DESKTOP_PATH=%USERPROFILE%\Desktop"

REM Create the output directory if it doesn't exist
if not exist "%OUTPUT_DIR%" (
    mkdir "%OUTPUT_DIR%"
)

REM Compile the project
"%COMPILER_PATH%\cl.exe" /EHsc /Fe:"%OUTPUT_DIR%\%EXECUTABLE_NAME%" "%SRC_PATH%\main.cpp" ws2_32.lib Comctl32.lib

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
