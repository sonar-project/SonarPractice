@echo off
setlocal EnableExtensions EnableDelayedExpansion

cd /d "%~dp0"
set "PROJECT_DIR=%CD%"

if not defined BUILD_DIR (
    set "BUILD_DIR=%PROJECT_DIR%\build\Desktop_Qt_6_11_1_MinGW_64_bit_Release"
)
:: Alternativen (BUILD_DIR vor dem Aufruf setzen oder Zeile oben anpassen):
:: build\Desktop_Qt_6_11_1_MinGW_64_bit_RelWithDebInfo
:: build\Desktop_Qt_6_11_1_MinGW_64_bit_Debug

set "DEPLOY_DIR=%PROJECT_DIR%\deploy"

if defined QTDIR (
    set "QT_BIN=%QTDIR%\bin"
) else if not defined QT_BIN (
    set "QT_BIN=C:\Qt\6.11.1\mingw_64\bin"
)

set "WINDEPLOYQT=%QT_BIN%\windeployqt6.exe"
if not exist "%WINDEPLOYQT%" set "WINDEPLOYQT=%QT_BIN%\windeployqt.exe"

:: 1. Deploy-Ordner leeren
if exist "%DEPLOY_DIR%" rd /s /q "%DEPLOY_DIR%"
mkdir "%DEPLOY_DIR%"

:: 2. RubberBand, OpenSSL und EXE kopieren
set "RUBBERBAND_DLL="
for %%F in (
    "%BUILD_DIR%\SonarPractice_Rubberband.dll"
    "%BUILD_DIR%\libSonarPractice_Rubberband.dll"
) do (
    if not defined RUBBERBAND_DLL if exist %%~F set "RUBBERBAND_DLL=%%~F"
)
if not defined RUBBERBAND_DLL (
    echo FEHLER: SonarPractice_Rubberband.dll wurde in "%BUILD_DIR%" nicht gefunden.
    exit /b 1
)
copy /Y "%RUBBERBAND_DLL%" "%DEPLOY_DIR%\SonarPractice_Rubberband.dll" >nul
if errorlevel 1 exit /b 1

copy /Y "%BUILD_DIR%\SonarPractice.exe" "%DEPLOY_DIR%\" >nul
if errorlevel 1 exit /b 1

call :CopyOpenSslDlls
if errorlevel 1 exit /b 1

:: 3. Qt-Abhaengigkeiten hinzufuegen
"%WINDEPLOYQT%" --qmldir "%PROJECT_DIR%\src\ui" --dir "%DEPLOY_DIR%" "%DEPLOY_DIR%\SonarPractice.exe"
if errorlevel 1 exit /b 1

:: 3b. WebEngine runtime must be present for the AlphaTab player (CI + local MSVC kits).
set "WE_OK=1"
if not exist "%DEPLOY_DIR%\Qt6WebEngineCore.dll" set "WE_OK=0"
if not exist "%DEPLOY_DIR%\Qt6WebEngineQuick.dll" set "WE_OK=0"
if not exist "%DEPLOY_DIR%\QtWebEngineProcess.exe" set "WE_OK=0"
if not exist "%DEPLOY_DIR%\qml\QtWebEngine" set "WE_OK=0"
if not exist "%DEPLOY_DIR%\resources\qtwebengine_resources.pak" (
  if not exist "%DEPLOY_DIR%\qtwebengine_resources.pak" set "WE_OK=0"
)
if "%WE_OK%"=="0" (
  echo FEHLER: Qt WebEngine wurde von windeployqt nicht vollstaendig deployed.
  echo Erwartet u.a.: Qt6WebEngineCore.dll, QtWebEngineProcess.exe, qml\QtWebEngine
  echo Inhalt von "%DEPLOY_DIR%":
  dir /b "%DEPLOY_DIR%"
  exit /b 1
)
echo Qt WebEngine deploy OK.

:: 4. qt.conf erstellen
(
    echo [Paths]
    echo Prefix = .
    echo Plugins = .
    echo Qml2Imports = qml
) > "%DEPLOY_DIR%\qt.conf"

echo qt.conf wurde erfolgreich im Verzeichnis %DEPLOY_DIR% erstellt.
echo Deployment fertig!

:: 5. Inno Setup (ISCC aus PATH oder Standard-Installation)
:: https://jrsoftware.org/isdl.php
set "ISCC=ISCC.exe"
where ISCC.exe >nul 2>&1
if errorlevel 1 (
    if exist "%ProgramFiles%\Inno Setup 6\ISCC.exe" (
        set "ISCC=%ProgramFiles%\Inno Setup 6\ISCC.exe"
    ) else (
        echo FEHLER: ISCC.exe nicht gefunden. Inno Setup installieren oder PATH setzen.
        exit /b 1
    )
)

if defined APP_VERSION (
    "%ISCC%" /DAppVersion=%APP_VERSION% /DOutputBaseFilename=SonarPractice_%APP_VERSION%_Setup "%PROJECT_DIR%\setup_script.iss"
) else (
    "%ISCC%" "%PROJECT_DIR%\setup_script.iss"
)

endlocal
exit /b 0

:CopyOpenSslDlls
set "OPENSSL_BIN="
if defined OPENSSL_ROOT_DIR if exist "%OPENSSL_ROOT_DIR%\bin" set "OPENSSL_BIN=%OPENSSL_ROOT_DIR%\bin"
if not defined OPENSSL_BIN if defined OPENSSL_ROOT if exist "%OPENSSL_ROOT%\bin" set "OPENSSL_BIN=%OPENSSL_ROOT%\bin"
if not defined OPENSSL_BIN if exist "%ProgramFiles%\OpenSSL-Win64\bin" set "OPENSSL_BIN=%ProgramFiles%\OpenSSL-Win64\bin"
if not defined OPENSSL_BIN if exist "%ProgramFiles%\OpenSSL\bin" set "OPENSSL_BIN=%ProgramFiles%\OpenSSL\bin"
if not defined OPENSSL_BIN if exist "%ProgramFiles(x86)%\OpenSSL-Win64\bin" set "OPENSSL_BIN=%ProgramFiles(x86)%\OpenSSL-Win64\bin"

if not defined OPENSSL_BIN (
    echo FEHLER: OpenSSL bin-Verzeichnis nicht gefunden. OPENSSL_ROOT_DIR setzen oder OpenSSL installieren.
    exit /b 1
)

set "COPIED_CRYPTO="
set "COPIED_SSL="
for %%D in ("%OPENSSL_BIN%\libcrypto-*.dll") do (
    copy /Y "%%~D" "%DEPLOY_DIR%\" >nul
    if errorlevel 1 exit /b 1
    echo Kopiert %%~nxD aus "%OPENSSL_BIN%"
    set "COPIED_CRYPTO=1"
)
for %%D in ("%OPENSSL_BIN%\libssl-*.dll") do (
    copy /Y "%%~D" "%DEPLOY_DIR%\" >nul
    if errorlevel 1 exit /b 1
    echo Kopiert %%~nxD aus "%OPENSSL_BIN%"
    set "COPIED_SSL=1"
)
if not defined COPIED_CRYPTO (
    echo FEHLER: libcrypto-*.dll nicht gefunden in "%OPENSSL_BIN%"
    exit /b 1
)
if not defined COPIED_SSL (
    echo FEHLER: libssl-*.dll nicht gefunden in "%OPENSSL_BIN%"
    exit /b 1
)
exit /b 0
