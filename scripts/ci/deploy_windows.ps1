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
        "${env:ProgramFiles}\OpenSSL"
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
$cryptoDlls = Get-ChildItem -Path $OpenSslBin -Filter "libcrypto-*.dll" -ErrorAction SilentlyContinue
$sslDlls = Get-ChildItem -Path $OpenSslBin -Filter "libssl-*.dll" -ErrorAction SilentlyContinue
if (-not $cryptoDlls) { throw "OpenSSL DLL was not found: $OpenSslBin\libcrypto-*.dll" }
if (-not $sslDlls) { throw "OpenSSL DLL was not found: $OpenSslBin\libssl-*.dll" }
foreach ($dll in @($cryptoDlls + $sslDlls)) {
    Copy-Item $dll.FullName $DeployDir -Force
    Write-Host "Copied $($dll.Name) from $OpenSslBin"
}

$WinDeployQt = Join-Path $QtBinDir "windeployqt6.exe"
if (-not (Test-Path $WinDeployQt)) {
    $WinDeployQt = Join-Path $QtBinDir "windeployqt.exe"
}

& $WinDeployQt --qmldir (Join-Path $Root "src\ui") --dir $DeployDir (Join-Path $DeployDir "SonarPractice.exe")
if ($LASTEXITCODE -ne 0) {
    throw "windeployqt failed with exit code $LASTEXITCODE"
}

$requiredWebEngine = @(
    "Qt6WebEngineCore.dll"
    "Qt6WebEngineQuick.dll"
    "QtWebEngineProcess.exe"
    "qml\QtWebEngine"
)
$missing = @()
foreach ($rel in $requiredWebEngine) {
    if (-not (Test-Path (Join-Path $DeployDir $rel))) { $missing += $rel }
}
$pak = @(
    (Join-Path $DeployDir "resources\qtwebengine_resources.pak")
    (Join-Path $DeployDir "qtwebengine_resources.pak")
) | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $pak) { $missing += "resources\qtwebengine_resources.pak" }
if ($missing.Count -gt 0) {
    Get-ChildItem $DeployDir | Select-Object -ExpandProperty Name
    throw "Qt WebEngine deploy incomplete (windeployqt): $($missing -join ', ')"
}
Write-Host "Qt WebEngine deploy OK"

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
