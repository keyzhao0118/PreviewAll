param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Debug',
    [switch]$ConfigureOnly,
    [switch]$UpdateTranslations,
    [switch]$UseSystemTools,
    [switch]$Verify
)

$ErrorActionPreference = 'Stop'
$ProjectRoot = Split-Path $PSScriptRoot -Parent
$VcpkgRoot = Join-Path $ProjectRoot 'out/tools/vcpkg'
$Manifest = Get-Content (Join-Path $ProjectRoot 'vcpkg.json') -Raw | ConvertFrom-Json
$Baseline = $Manifest.'builtin-baseline'

function Invoke-Checked([string]$Program, [string[]]$Arguments) {
    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "$Program failed with exit code $LASTEXITCODE"
    }
}

Push-Location $ProjectRoot
try {
    $CMake = (Get-Command cmake -ErrorAction Stop).Source
    $Git = (Get-Command git -ErrorAction Stop).Source
    $VsWhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
    if (-not (Test-Path $VsWhere)) { throw 'Install Visual Studio 2026 with Desktop development with C++.' }
    $VsPath = & $VsWhere -latest -version '[18.0,19.0)' -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (-not $VsPath) { throw 'Visual Studio 2026 C++ tools were not found.' }
    if ($UseSystemTools) {
        $env:VCPKG_FORCE_SYSTEM_BINARIES = '1'
        $env:PATH = (Join-Path $VsPath 'Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja') + ';' +
            (Join-Path $env:ProgramFiles '7-Zip') + ';' + $env:PATH
        $GitRoot = Split-Path (Split-Path $Git -Parent) -Parent
        $GitPerl = Join-Path $GitRoot 'usr/bin'
        if (Test-Path (Join-Path $GitPerl 'perl.exe')) {
            $env:PATH += ';' + $GitPerl
        }
        $env:VCPKG_KEEP_ENV_VARS = (@($env:VCPKG_KEEP_ENV_VARS -split ';') + 'PATH' |
            Where-Object { $_ } | Select-Object -Unique) -join ';'
    }

    if (-not (Test-Path (Join-Path $VcpkgRoot '.git'))) {
        New-Item -ItemType Directory -Force $VcpkgRoot | Out-Null
        Invoke-Checked $Git @('init', $VcpkgRoot)
        Invoke-Checked $Git @('-C', $VcpkgRoot, 'remote', 'add', 'origin', 'https://github.com/microsoft/vcpkg.git')
    }
    $Head = & $Git -C $VcpkgRoot rev-parse --verify --quiet HEAD
    if ($LASTEXITCODE -ne 0 -or $Head -ne $Baseline) {
        Invoke-Checked $Git @('-C', $VcpkgRoot, 'fetch', '--depth', '1', 'origin', $Baseline)
        Invoke-Checked $Git @('-C', $VcpkgRoot, 'checkout', '--detach', $Baseline)
    }
    $ToolMetadata = Get-Content (Join-Path $VcpkgRoot 'scripts/vcpkg-tool-metadata.txt') -Raw | ConvertFrom-StringData
    $ToolVersion = ''
    if (Test-Path (Join-Path $VcpkgRoot 'vcpkg.exe')) {
        try { $ToolVersion = (& (Join-Path $VcpkgRoot 'vcpkg.exe') version --disable-metrics | Out-String) }
        catch { Write-Verbose 'The previous vcpkg download is incomplete; bootstrap will retry.' }
    }
    if ($ToolVersion -notmatch [regex]::Escape($ToolMetadata.VCPKG_TOOL_RELEASE_TAG)) {
        Invoke-Checked (Join-Path $VcpkgRoot 'bootstrap-vcpkg.bat') @('-disableMetrics')
    }

    # Keep downloads and binary caches inside the ignored project output tree.
    $env:VCPKG_ROOT = $VcpkgRoot
    $env:VCPKG_DOWNLOADS = Join-Path $ProjectRoot 'out/downloads'
    $Cache = Join-Path $ProjectRoot 'out/vcpkg-cache'
    New-Item -ItemType Directory -Force $Cache, $env:VCPKG_DOWNLOADS | Out-Null
    $env:VCPKG_BINARY_SOURCES = "clear;files,$Cache,readwrite"
    $env:VCPKG_DISABLE_METRICS = '1'
    $Preset = 'x64-' + $Configuration.ToLowerInvariant()
    Invoke-Checked $CMake @('--preset', $Preset)
    if ($ConfigureOnly) { return }
    if ($UpdateTranslations) {
        Invoke-Checked $CMake @('--build', '--preset', $Preset, '--target', 'PreviewAll_update_translations')
    }
    Invoke-Checked $CMake @('--build', '--preset', $Preset, '--parallel')
    if ($Verify) {
        Invoke-Checked $CMake @('--build', '--preset', $Preset, '--target', 'PreviewAllSmoke', '--parallel')
        Invoke-Checked (Join-Path $ProjectRoot "out/build/$Preset/bin/PreviewAllSmoke.exe") @((Join-Path $ProjectRoot 'test'))
    }
    Write-Host "Build ready: $ProjectRoot/out/build/$Preset/bin"
}
finally {
    Pop-Location
}
