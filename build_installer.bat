@echo off
setlocal EnableExtensions

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

:: 2. RubberBand und EXE kopieren
copy "%BUILD_DIR%\libSonarPractice_Rubberband.dll" "%DEPLOY_DIR%\"
copy "%BUILD_DIR%\SonarPractice.exe" "%DEPLOY_DIR%\"

:: 3. Qt-Abhaengigkeiten hinzufuegen
"%WINDEPLOYQT%" --qmldir "%PROJECT_DIR%\src\ui" --dir "%DEPLOY_DIR%" "%DEPLOY_DIR%\SonarPractice.exe"

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
