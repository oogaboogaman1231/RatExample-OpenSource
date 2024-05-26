@echo off

rem Step 1: Set Download Paths
set DOWNLOAD_FOLDER=%USERPROFILE%\Downloads
set APPDATA_FOLDER=%APPDATA%\RatExample

rem Step 2: Create AppData Folder
if not exist "%APPDATA_FOLDER%" mkdir "%APPDATA_FOLDER%"

rem Step 3: Download Repository Content
echo Downloading repository content...
git clone https://github.com/oogaboogaman1231/RatExample-OpenSource.git "%APPDATA_FOLDER%" > nul 2>&1

rem Step 4: Copy Files to AppData Folder
echo Copying files to AppData...
xcopy /E /Y "%APPDATA_FOLDER%" "%APPDATA_FOLDER%" > nul

rem Step 5: Download rat.ico to AppData Folder
echo Downloading rat.ico...
curl -sS -o "%APPDATA_FOLDER%\rat.ico" "https://raw.githubusercontent.com/oogaboogaman1231/RatExample-OpenSource/main/rat.ico"

rem Step 6: Create Shortcut for main.bat on Desktop
echo Creating shortcut...
echo Set oWS = WScript.CreateObject("WScript.Shell") > "%TEMP%\CreateShortcut.vbs"
echo sLinkFile = "%USERPROFILE%\Desktop\RatExample.lnk" >> "%TEMP%\CreateShortcut.vbs"
echo Set oLink = oWS.CreateShortcut(sLinkFile) >> "%TEMP%\CreateShortcut.vbs"
echo oLink.TargetPath = "%APPDATA_FOLDER%\main.bat" >> "%TEMP%\CreateShortcut.vbs"
echo oLink.IconLocation = "%APPDATA_FOLDER%\rat.ico" >> "%TEMP%\CreateShortcut.vbs"
echo oLink.Description = "RAT Server" >> "%TEMP%\CreateShortcut.vbs"
echo oLink.Save >> "%TEMP%\CreateShortcut.vbs"
cscript //nologo "%TEMP%\CreateShortcut.vbs" > nul

rem Cleanup
del "%TEMP%\CreateShortcut.vbs" > nul 2>&1

echo Installation complete.
pause
