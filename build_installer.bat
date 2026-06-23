@echo off
set PROJECT_DIR=C:\Users\sandy\Develop\Projekte\SonarPractice
set BUILD_DIR=%PROJECT_DIR%\build\Desktop_Qt_6_11_1_MinGW_64_bit_Release
::Desktop_Qt_6_11_1_MinGW_64_bit_RelWithDebInfo
::Desktop_Qt_6_11_1_MinGW_64_bit_Debug\
::Desktop_Qt_6_11_1_MinGW_64_bit_Release
::
set DEPLOY_DIR=%PROJECT_DIR%\deploy

:: 1. Deploy-Ordner leeren
rd /s /q "%DEPLOY_DIR%"
mkdir "%DEPLOY_DIR%"

:: 2. copy RubberBand for SonarPractice
copy "%BUILD_DIR%\libSonarPractice_Rubberband.dll" "%DEPLOY_DIR%\"

:: 2. Eigene EXE kopieren
copy "%BUILD_DIR%\SonarPractice.exe" "%DEPLOY_DIR%\"

:: 3. Qt-Abhängigkeiten hinzufügen
C:\Qt\6.11.1\mingw_64\bin\windeployqt6.exe --qmldir src/ui/ --dir "%DEPLOY_DIR%" "%DEPLOY_DIR%\SonarPractice.exe"

@echo off

REM Setze den Pfad auf das Verzeichnis, in dem dieses Skript liegt
set "DEPLOY_DIR=%~dp0deploy"

REM Erstelle die qt.conf Datei
(
    echo [Paths]
    echo Prefix = .
    echo Plugins = .
    echo Qml2Imports = qml
) > "%DEPLOY_DIR%qt.conf"

echo qt.conf wurde erfolgreich im Verzeichnis %DEPLOY_DIR% erstellt.

echo Deployment fertig!
REM pause
ISCC.exe setup_script.iss
