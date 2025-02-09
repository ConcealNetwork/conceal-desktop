@echo off
setlocal enabledelayedexpansion

echo Starting Conceal Desktop Windows Deployment

REM Get Qt path from CMakeLists.txt using PowerShell script
powershell -ExecutionPolicy Bypass -File get_qt_path.ps1 > qt_path.ini
if %ERRORLEVEL% NEQ 0 (
    echo Failed to get Qt path, using default
    set "QTDIR=C:\Qt\5.15.2\msvc2019_64"
) else (
    for /f "tokens=2 delims==" %%a in ('type qt_path.ini ^| find "QtDir"') do set "QTDIR=%%a"
)

echo Using Qt from: %QTDIR%

REM Run windeployqt
echo Running windeployqt...
"%QTDIR%\bin\windeployqt.exe" --release --no-compiler-runtime "..\..\build\Release\conceal-desktop.exe"
if %ERRORLEVEL% NEQ 0 (
    echo Failed to run windeployqt
    exit /b 1
)

REM Copy OpenSSL DLLs
echo Copying OpenSSL DLLs...
REM OpenSSL v3
copy "%QTDIR%\..\..\Tools\OpenSSLv3\Win_x64\bin\libcrypto-3-x64.dll" "..\..\build\Release\"
copy "%QTDIR%\..\..\Tools\OpenSSLv3\Win_x64\bin\libssl-3-x64.dll" "..\..\build\Release\"
REM OpenSSL v1.1
copy "%QTDIR%\..\..\Tools\mingw1120_64\opt\bin\libcrypto-1_1-x64.dll" "..\..\build\Release\"
copy "%QTDIR%\..\..\Tools\mingw1120_64\opt\bin\libssl-1_1-x64.dll" "..\..\build\Release\"
if %ERRORLEVEL% NEQ 0 (
    echo Error: Failed to copy OpenSSL DLLs
    exit /b 1
)

REM Run Inno Setup Compiler
echo Creating installer...
"C:\Program Files (x86)\Inno Setup 6\ISCC.exe" /DQtPath="%QTDIR%" ConcealInstaller.iss
if %ERRORLEVEL% NEQ 0 (
    echo Failed to create installer
    exit /b 1
)

REM Rename the installer to remove spaces
cd bin
set "finalname="
for %%f in ("Conceal Desktop-*.exe") do (
    set "oldname=%%f"
    set "newname=!oldname: =!"
)

for %%f in ("Conceal*Desktop-*.exe") do (
    set "name=%%f"
    set "finalname=!name:.exe=!"
    echo Debug: finalname value is: !finalname!
    
    REM Wait for 3 seconds to ensure the file is released
    timeout /t 3 /nobreak
    echo Creating ZIP archive...
    powershell -Command "Compress-Archive -Path '.\!finalname!.exe' -DestinationPath '.\!finalname!.zip' -Force"
)

if %ERRORLEVEL% NEQ 0 (
    echo Failed to create ZIP archive
    exit /b 1
)
echo Created ZIP archive: !finalname!.zip

REM Generate SHA256 hash for the zip file
certutil -hashfile "!finalname!.zip" SHA256 | findstr /v "hash" | findstr /v "CertUtil" > "!finalname!.zip.sha256"
if %ERRORLEVEL% NEQ 0 (
    echo Failed to generate SHA256 for zip
    exit /b 1
)
echo Generated SHA256 hash file: !finalname!.zip.sha256

echo Deployment completed successfully
echo Installer, ZIP archive and SHA256 hash can be found in the bin directory 