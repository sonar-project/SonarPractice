param(
    [Parameter(Mandatory = $true)]
    [string]$Version,

    [Parameter(Mandatory = $true)]
    [string]$BuildDir,

    [Parameter(Mandatory = $true)]
    [string]$QtBinDir
)

$ErrorActionPreference = "Stop"
$Version = $Version -replace '^v', ''

$Root = Resolve-Path (Join-Path $PSScriptRoot "..\..")
$DeployDir = Join-Path $Root "deploy"
$IssFile = Join-Path $Root "setup_script.iss"
$OutputBaseFilename = "SonarPractice-$Version`_Setup"

if (Test-Path $DeployDir) {
    Remove-Item -Recurse -Force $DeployDir
}
New-Item -ItemType Directory -Path $DeployDir | Out-Null

$RubberBandDll = Join-Path $BuildDir "SonarPractice_RubberbandLib.dll"
if (-not (Test-Path $RubberBandDll)) {
    Write-Error "DLL was not found: $RubberBandDll"
    exit 1
}
Copy-Item $RubberBandDll $DeployDir

Copy-Item (Join-Path $BuildDir "SonarPractice.exe") $DeployDir

$WinDeployQt = Join-Path $QtBinDir "windeployqt6.exe"
if (-not (Test-Path $WinDeployQt)) {
    $WinDeployQt = Join-Path $QtBinDir "windeployqt.exe"
}

& $WinDeployQt --qmldir (Join-Path $Root "src\ui") --dir $DeployDir (Join-Path $DeployDir "SonarPractice.exe")

@"
[Paths]
Prefix = .
Plugins = .
Qml2Imports = qml
"@ | Set-Content -Path (Join-Path $DeployDir "qt.conf") -Encoding ASCII

$IsccCandidates = @(
    "$env:ProgramFiles\Inno Setup 6\ISCC.exe"
)
$Iscc = $IsccCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $Iscc) {
    throw "Inno Setup compiler (ISCC.exe) not found."
}

& $Iscc `
    "/DAppVersion=$Version" `
    "/DOutputBaseFilename=$OutputBaseFilename" `
    $IssFile

$Installer = Join-Path $Root "$OutputBaseFilename.exe"
if (-not (Test-Path $Installer)) {
    throw "Installer was not created: $Installer"
}

Write-Host "Created $Installer"
