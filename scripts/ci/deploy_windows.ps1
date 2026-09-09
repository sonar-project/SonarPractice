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

$RubberBandCandidates = @(
    "SonarPractice_Rubberband.dll"
    "libSonarPractice_Rubberband.dll"
)
$RubberBandDll = $null
foreach ($name in $RubberBandCandidates) {
    $candidate = Join-Path $BuildDir $name
    if (Test-Path $candidate) {
        $RubberBandDll = $candidate
        break
    }
}
if (-not $RubberBandDll) {
    Write-Error "Rubber Band DLL was not found in $BuildDir (expected SonarPractice_Rubberband.dll)"
    exit 1
}
Copy-Item $RubberBandDll (Join-Path $DeployDir "SonarPractice_Rubberband.dll") -Force

Copy-Item (Join-Path $BuildDir "SonarPractice.exe") $DeployDir

function Get-OpenSslBinDir {
    $roots = @()
    if ($env:OPENSSL_ROOT_DIR) { $roots += $env:OPENSSL_ROOT_DIR }
    if ($env:OPENSSL_ROOT) { $roots += $env:OPENSSL_ROOT }
    $roots += @(
        "${env:ProgramFiles}\OpenSSL-Win64"
        "${env:ProgramFiles(x86)}\OpenSSL-Win64"
    )
    foreach ($root in $roots) {
        if (-not $root) { continue }
        $bin = Join-Path $root "bin"
        if (Test-Path $bin) { return $bin }
    }
    return $null
}

$OpenSslBin = Get-OpenSslBinDir
if (-not $OpenSslBin) {
    throw "OpenSSL bin directory not found. Set OPENSSL_ROOT_DIR or install OpenSSL-Win64."
}
foreach ($dll in @("libcrypto-3-x64.dll", "libssl-3-x64.dll")) {
    $src = Join-Path $OpenSslBin $dll
    if (-not (Test-Path $src)) {
        throw "OpenSSL DLL was not found: $src"
    }
    Copy-Item $src $DeployDir -Force
    Write-Host "Copied $dll from $OpenSslBin"
}

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
