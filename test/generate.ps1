param(
    [string]$Python,
    [switch]$DownloadTools,
    [switch]$RegenerateArchives
)

$ErrorActionPreference = 'Stop'
$TestRoot = $PSScriptRoot
$ToolsRoot = Join-Path $TestRoot '.tools'
$WorkRoot = Join-Path (Split-Path $TestRoot -Parent) 'out/test-fixture-work'
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

function Get-SystemSevenZipPaths {
    @(
        (Join-Path $env:ProgramFiles '7-Zip\7z.exe'),
        (Join-Path ${env:ProgramFiles(x86)} '7-Zip\7z.exe')
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
New-Item -ItemType Directory -Force $WorkRoot | Out-Null
$baseArgs = @((Join-Path $TestRoot 'generate.py'), '--root', $TestRoot, '--work-root', $WorkRoot)
if ($RegenerateArchives) { $baseArgs += '--regenerate-archives' }
& $PythonExe @baseArgs
if ($LASTEXITCODE -ne 0) { throw 'Base fixture generation failed.' }

if ($DownloadTools) { Install-TestTools }

$SevenZip = Resolve-Tool '7z' (Join-Path $ToolsRoot '7zip\7z.exe') @(Get-SystemSevenZipPaths)
$Rar = Resolve-Tool 'rar' (Join-Path $ToolsRoot 'winrar\Rar.exe') @(Get-SystemRarPaths)
$Payload = Join-Path $WorkRoot 'archive-payload'
$ArchiveRoot = Join-Path $TestRoot 'archives'
$extra = [Collections.Generic.List[object]]::new()

if ($SevenZip) {
    $sevenZipCases = @(
        @{ Name='plain.7z'; Args=@('a','-t7z','-mx=5','-y') ; Scenario='Plain 7Z'; Expected='tree preview' },
        @{ Name='password-content.7z'; Args=@('a','-t7z','-mx=5',"-p$Password",'-mhe=off','-y'); Scenario='7Z encrypted content, visible headers'; Expected='tree preview without password prompt' },
        @{ Name='password-header.7z'; Args=@('a','-t7z','-mx=5',"-p$Password",'-mhe=on','-y'); Scenario='7Z encrypted content and headers'; Expected='cannot preview without password prompt' },
        @{ Name='password-aes.zip'; Args=@('a','-tzip','-mx=5',"-p$Password",'-mem=AES256','-y'); Scenario='ZIP AES-256 encryption'; Expected='tree preview without password prompt' },
        @{ Name='password-zipcrypto.zip'; Args=@('a','-tzip','-mx=5',"-p$Password",'-mem=ZipCrypto','-y'); Scenario='ZIP legacy ZipCrypto encryption'; Expected='tree preview without password prompt' }
    )
    foreach ($case in $sevenZipCases) {
        $destination = Join-Path $ArchiveRoot $case.Name
        if ($RegenerateArchives -or -not (Test-Path $destination)) {
            Remove-Item $destination -Force -ErrorAction SilentlyContinue
            Invoke-ArchiveTool $SevenZip ($case.Args + @($destination, (Join-Path $Payload '*'), '-r'))
        }
        $extra.Add(@{path=$destination; category='archive'; scenario=$case.Scenario; expected=$case.Expected})
    }
    if ($RegenerateArchives -or -not (Test-Path (Join-Path $ArchiveRoot 'corrupt-truncated.7z'))) {
        New-CorruptCopy (Join-Path $ArchiveRoot 'plain.7z') (Join-Path $ArchiveRoot 'corrupt-truncated.7z')
    }
    $extra.Add(@{path=(Join-Path $ArchiveRoot 'corrupt-truncated.7z'); category='archive'; scenario='Truncated 7Z'; expected='load failure'})
} else {
    if ($RegenerateArchives) { throw '7z.exe is required for -RegenerateArchives.' }
    Write-Warning '7z.exe was not found; checking the committed 7Z and encrypted ZIP fixtures.'
    $existingSevenZipCases = @(
        @{ Name='plain.7z'; Scenario='Plain 7Z'; Expected='tree preview' },
        @{ Name='password-content.7z'; Scenario='7Z encrypted content, visible headers'; Expected='tree preview without password prompt' },
        @{ Name='password-header.7z'; Scenario='7Z encrypted content and headers'; Expected='cannot preview without password prompt' },
        @{ Name='password-aes.zip'; Scenario='ZIP AES-256 encryption'; Expected='tree preview without password prompt' },
        @{ Name='password-zipcrypto.zip'; Scenario='ZIP legacy ZipCrypto encryption'; Expected='tree preview without password prompt' },
        @{ Name='corrupt-truncated.7z'; Scenario='Truncated 7Z'; Expected='load failure' }
    )
    foreach ($case in $existingSevenZipCases) {
        $destination = Join-Path $ArchiveRoot $case.Name
        if (-not (Test-Path $destination)) { throw "Missing fixture $destination. Install 7-Zip or use -DownloadTools." }
        $extra.Add(@{path=$destination; category='archive'; scenario=$case.Scenario; expected=$case.Expected})
    }
}

if ($Rar) {
    $rarCases = @(
        @{ Name='plain-rar5.rar'; Args=@('a','-idq','-r','-ma5'); Scenario='Plain RAR5'; Expected='tree preview' },
        @{ Name='password-content.rar'; Args=@('a','-idq','-r','-ma5',"-p$Password"); Scenario='RAR5 encrypted content, visible headers'; Expected='tree preview without password prompt' },
        @{ Name='password-header.rar'; Args=@('a','-idq','-r','-ma5',"-hp$Password"); Scenario='RAR5 encrypted content and headers'; Expected='cannot preview without password prompt' }
    )
    foreach ($case in $rarCases) {
        $destination = Join-Path $ArchiveRoot $case.Name
        if ($RegenerateArchives -or -not (Test-Path $destination)) {
            Remove-Item $destination -Force -ErrorAction SilentlyContinue
            Invoke-ArchiveTool $Rar ($case.Args + @($destination, (Join-Path $Payload '*')))
        }
        $extra.Add(@{path=$destination; category='archive'; scenario=$case.Scenario; expected=$case.Expected})
    }
    if ($RegenerateArchives -or -not (Test-Path (Join-Path $ArchiveRoot 'corrupt-truncated.rar'))) {
        New-CorruptCopy (Join-Path $ArchiveRoot 'plain-rar5.rar') (Join-Path $ArchiveRoot 'corrupt-truncated.rar')
    }
    $extra.Add(@{path=(Join-Path $ArchiveRoot 'corrupt-truncated.rar'); category='archive'; scenario='Truncated RAR5'; expected='load failure'})
} else {
    if ($RegenerateArchives) { throw 'Rar.exe is required for -RegenerateArchives.' }
    Write-Warning 'Rar.exe was not found; checking the committed RAR fixtures.'
    $existingRarCases = @(
        @{ Name='plain-rar5.rar'; Scenario='Plain RAR5'; Expected='tree preview' },
        @{ Name='password-content.rar'; Scenario='RAR5 encrypted content, visible headers'; Expected='tree preview without password prompt' },
        @{ Name='password-header.rar'; Scenario='RAR5 encrypted content and headers'; Expected='cannot preview without password prompt' },
        @{ Name='corrupt-truncated.rar'; Scenario='Truncated RAR5'; Expected='load failure' }
    )
    foreach ($case in $existingRarCases) {
        $destination = Join-Path $ArchiveRoot $case.Name
        if (-not (Test-Path $destination)) { throw "Missing fixture $destination. Install WinRAR or use -DownloadTools." }
        $extra.Add(@{path=$destination; category='archive'; scenario=$case.Scenario; expected=$case.Expected})
    }
}

$extraJson = ConvertTo-Json -InputObject @($extra) -Depth 5
[IO.File]::WriteAllText(
    (Join-Path $WorkRoot '.archive-records.json'),
    $extraJson,
    [Text.UTF8Encoding]::new($false))
& $PythonExe (Join-Path $TestRoot 'generate.py') --root $TestRoot --work-root $WorkRoot --manifest-only
if ($LASTEXITCODE -ne 0) { throw 'Manifest generation failed.' }

Write-Host "Generated PreviewAll fixtures under $TestRoot"
Write-Host "Archive password: $Password"
