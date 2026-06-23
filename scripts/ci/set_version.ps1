param(
    [Parameter(Mandatory = $true)]
    [string]$Version
)

$Version = $Version -replace '^v', ''

if ($Version -notmatch '^\d+\.\d+\.\d+') {
    Write-Error "Unsupported version format: $Version"
    exit 1
}

$Root = Resolve-Path (Join-Path $PSScriptRoot "..\..")
$CMakeFile = Join-Path $Root "CMakeLists.txt"

$lines = Get-Content -Path $CMakeFile
$lines = $lines -replace '^project\(SonarPractice VERSION .*', "project(SonarPractice VERSION $Version)"
$lines | Set-Content -Path $CMakeFile

Write-Host "Set project version to $Version in CMakeLists.txt"
