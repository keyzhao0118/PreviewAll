param(
    [string]$Python,
    [switch]$DownloadTools
)

$ErrorActionPreference = 'Stop'
$TestRoot = $PSScriptRoot
$ToolsRoot = Join-Path $TestRoot '.tools'
$Password = 'PreviewAll-Test-123!'

function Find-Python {
    if ($Python) { return $Python }

    $command = Get-Command python -ErrorAction SilentlyContinue
    if ($command) { return $command.Source }

    $runtimeRoot = Join-Path $env:USERPROFILE '.cache\codex-runtimes'
    if (Test-Path $runtimeRoot) {
        $candidate = Get-ChildItem $runtimeRoot -Filter python.exe -Recurse -File -ErrorAction SilentlyContinue |
            Where-Object { $_.FullName -match 'dependencies\\python\\python\.exe$' } |
            Select-Object -First 1
        if ($candidate) { return $candidate.FullName }
    }

    $command = Get-Command py -ErrorAction SilentlyContinue
    if ($command) { return $command.Source }

    throw 'Python with Pillow is required. Pass -Python <path>.'
}

function Get-SystemRarPaths {
    @(
        (Join-Path $env:ProgramFiles 'WinRAR\Rar.exe'),
        (Join-Path ${env:ProgramFiles(x86)} 'WinRAR\Rar.exe')
    ) | Where-Object { $_ -and (Test-Path $_) }
}

function Install-TestTools {
    New-Item -ItemType Directory -Force $ToolsRoot | Out-Null
    $sevenZipDir = Join-Path $ToolsRoot '7zip'
    $rarDir = Join-Path $ToolsRoot 'winrar'

    if (-not (Test-Path (Join-Path $sevenZipDir '7z.exe'))) {
        $installer = Join-Path $ToolsRoot '7zip-installer.exe'
        Invoke-WebRequest 'https://github.com/ip7z/7zip/releases/download/26.02/7z2602-x64.exe' -OutFile $installer
        Start-Process $installer -ArgumentList '/S', "/D=$sevenZipDir" -Wait -WindowStyle Hidden
    }

    if (-not (Get-SystemRarPaths) -and -not (Test-Path (Join-Path $rarDir 'Rar.exe'))) {
        $installer = Join-Path $ToolsRoot 'winrar-installer.exe'
        Invoke-WebRequest 'https://www.rarlab.com/rar/winrar-x64-723.exe' -OutFile $installer
        Start-Process $installer -ArgumentList '-s', ("-d" + $rarDir) -Wait -WindowStyle Hidden
    }
}

function Resolve-Tool([string]$Name, [string]$BundledPath, [string[]]$FallbackPaths = @()) {
    if (Test-Path $BundledPath) { return $BundledPath }
    foreach ($path in $FallbackPaths) {
        if ($path -and (Test-Path $path)) { return $path }
    }
    $command = Get-Command $Name -ErrorAction SilentlyContinue
    if ($command) { return $command.Source }
    return $null
}

function Invoke-ArchiveTool([string]$Tool, [string[]]$Arguments) {
    & $Tool @Arguments | Out-Null
    if ($LASTEXITCODE -ne 0) {
        throw "$Tool failed with exit code $LASTEXITCODE"
    }
}

function New-CorruptCopy([string]$Source, [string]$Destination) {
    $bytes = [IO.File]::ReadAllBytes($Source)
    $length = [Math]::Max(16, [Math]::Floor($bytes.Length * 0.6))
    [IO.File]::WriteAllBytes($Destination, $bytes[0..($length - 1)])
}

$PythonExe = Find-Python
& $PythonExe (Join-Path $TestRoot 'generate.py') --root $TestRoot
if ($LASTEXITCODE -ne 0) { throw 'Base fixture generation failed.' }

if ($DownloadTools) { Install-TestTools }

$SevenZip = Resolve-Tool '7z' (Join-Path $ToolsRoot '7zip\7z.exe')
$Rar = Resolve-Tool 'rar' (Join-Path $ToolsRoot 'winrar\Rar.exe') @(Get-SystemRarPaths)
$Payload = Join-Path $TestRoot 'archive-payload'
$ArchiveRoot = Join-Path $TestRoot 'archives'
$extra = [Collections.Generic.List[object]]::new()

if ($SevenZip) {
    $sevenZipCases = @(
        @{ Name='plain.7z'; Args=@('a','-t7z','-mx=5','-y') ; Scenario='Plain 7Z'; Expected='tree preview' },
        @{ Name='password-content.7z'; Args=@('a','-t7z','-mx=5',"-p$Password",'-mhe=off','-y'); Scenario='7Z encrypted content, visible headers'; Expected='password prompt then tree preview' },
        @{ Name='password-header.7z'; Args=@('a','-t7z','-mx=5',"-p$Password",'-mhe=on','-y'); Scenario='7Z encrypted content and headers'; Expected='password prompt before file list' },
        @{ Name='password-aes.zip'; Args=@('a','-tzip','-mx=5',"-p$Password",'-mem=AES256','-y'); Scenario='ZIP AES-256 encryption'; Expected='password prompt then tree preview' },
        @{ Name='password-zipcrypto.zip'; Args=@('a','-tzip','-mx=5',"-p$Password",'-mem=ZipCrypto','-y'); Scenario='ZIP legacy ZipCrypto encryption'; Expected='password prompt then tree preview' }
    )
    foreach ($case in $sevenZipCases) {
        $destination = Join-Path $ArchiveRoot $case.Name
        Remove-Item $destination -Force -ErrorAction SilentlyContinue
        Invoke-ArchiveTool $SevenZip ($case.Args + @($destination, (Join-Path $Payload '*'), '-r'))
        $extra.Add(@{path=$destination; category='archive'; scenario=$case.Scenario; expected=$case.Expected})
    }
    New-CorruptCopy (Join-Path $ArchiveRoot 'plain.7z') (Join-Path $ArchiveRoot 'corrupt-truncated.7z')
    $extra.Add(@{path=(Join-Path $ArchiveRoot 'corrupt-truncated.7z'); category='archive'; scenario='Truncated 7Z'; expected='load failure'})
} else {
    Write-Warning '7z.exe was not found; encrypted ZIP and 7Z fixtures were skipped. Use -DownloadTools.'
}

if ($Rar) {
    $rarCases = @(
        @{ Name='plain-rar5.rar'; Args=@('a','-idq','-r','-ma5'); Scenario='Plain RAR5'; Expected='tree preview' },
        @{ Name='password-content.rar'; Args=@('a','-idq','-r','-ma5',"-p$Password"); Scenario='RAR5 encrypted content, visible headers'; Expected='password prompt then tree preview' },
        @{ Name='password-header.rar'; Args=@('a','-idq','-r','-ma5',"-hp$Password"); Scenario='RAR5 encrypted content and headers'; Expected='password prompt before file list' }
    )
    foreach ($case in $rarCases) {
        $destination = Join-Path $ArchiveRoot $case.Name
        Remove-Item $destination -Force -ErrorAction SilentlyContinue
        Invoke-ArchiveTool $Rar ($case.Args + @($destination, (Join-Path $Payload '*')))
        $extra.Add(@{path=$destination; category='archive'; scenario=$case.Scenario; expected=$case.Expected})
    }
    New-CorruptCopy (Join-Path $ArchiveRoot 'plain-rar5.rar') (Join-Path $ArchiveRoot 'corrupt-truncated.rar')
    $extra.Add(@{path=(Join-Path $ArchiveRoot 'corrupt-truncated.rar'); category='archive'; scenario='Truncated RAR5'; expected='load failure'})
} else {
    Write-Warning 'Rar.exe was not found; RAR fixtures were skipped. Use -DownloadTools.'
}

$extraJson = ConvertTo-Json -InputObject @($extra) -Depth 5
[IO.File]::WriteAllText(
    (Join-Path $TestRoot '.archive-records.json'),
    $extraJson,
    [Text.UTF8Encoding]::new($false))
& $PythonExe (Join-Path $TestRoot 'generate.py') --root $TestRoot --manifest-only
if ($LASTEXITCODE -ne 0) { throw 'Manifest generation failed.' }

Write-Host "Generated PreviewAll fixtures under $TestRoot"
Write-Host "Archive password: $Password"