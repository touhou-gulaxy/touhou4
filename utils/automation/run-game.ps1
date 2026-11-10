Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

# must try load mod id: 1623423360
# optional load mod id(if exists): 3387068412, 2841481791, 1595876588
# mod load priority: 1623423360, 1595876588, touhou_mod(project root), 2841481791, 3387068412

$script:STELLARIS_APP_ID = '281990'
$script:MUST_TRY_MOD_ID = '1623423360'
$script:OPTIONAL_MODS_BEFORE_REPO = @('1595876588')
$script:OPTIONAL_MODS_AFTER_REPO = @('2841481791', '3387068412')
$script:IsWindowsPlatform =
    [System.Environment]::OSVersion.Platform -eq [System.PlatformID]::Win32NT

function Show-Usage {
    $usage = @'
usage: run-game.ps1 [-f] [-d] [-c] [--inline_hook=<hook.so>[,<hook.so>...]] [game arguments...]

  -f                    also load optional Workshop mods when installed
  -d                    do not pass -mod; use the game's dlc_load.json
  -c                    continue the most recent save (--continuelastsave)
  --inline_hook=<list>  preload one or more .so files via LD_PRELOAD

The --inline_hook option may be repeated. Remaining arguments are passed
through to the Stellaris executable.
'@
    Write-Output $usage
}

function Exit-WithError {
    param([Parameter(Mandatory = $true)][string] $Message)

    [Console]::Error.WriteLine("error: $Message")
    exit 1
}

function Write-WarningLine {
    param([Parameter(Mandatory = $true)][string] $Message)

    [Console]::Error.WriteLine("warning: $Message")
}

function Get-CanonicalDirectory {
    param([string] $Path)

    if ([String]::IsNullOrWhiteSpace($Path)) {
        return $null
    }

    try {
        if (-not (Test-Path -LiteralPath $Path -PathType Container)) {
            return $null
        }

        if (-not $script:IsWindowsPlatform) {
            try {
                $resolved = & realpath -- $Path 2>$null
                if ($LASTEXITCODE -eq 0 -and $null -ne $resolved) {
                    return [string]($resolved | Select-Object -First 1)
                }
            }
            catch {
                # Fall back to .NET path canonicalization when realpath is unavailable.
            }
        }

        return (Get-Item -LiteralPath $Path -Force).FullName
    }
    catch {
        return $null
    }
}

function Get-CanonicalFile {
    param([string] $Path)

    if ([String]::IsNullOrWhiteSpace($Path)) {
        return $null
    }

    try {
        if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
            return $null
        }

        if (-not $script:IsWindowsPlatform) {
            try {
                $resolved = & realpath -- $Path 2>$null
                if ($LASTEXITCODE -eq 0 -and $null -ne $resolved) {
                    return [string]($resolved | Select-Object -First 1)
                }
            }
            catch {
                # Fall back to .NET path canonicalization when realpath is unavailable.
            }
        }

        return (Get-Item -LiteralPath $Path -Force).FullName
    }
    catch {
        return $null
    }
}

function Test-ElfFile {
    param([string] $Path)

    if ([String]::IsNullOrWhiteSpace($Path) -or
        -not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        return $false
    }

    $stream = $null
    try {
        $stream = [System.IO.File]::OpenRead($Path)
        $magic = New-Object byte[] 4
        if ($stream.Read($magic, 0, $magic.Length) -ne $magic.Length) {
            return $false
        }

        return (
            $magic[0] -eq 0x7f -and
            $magic[1] -eq 0x45 -and
            $magic[2] -eq 0x4c -and
            $magic[3] -eq 0x46
        )
    }
    catch {
        return $false
    }
    finally {
        if ($null -ne $stream) {
            $stream.Dispose()
        }
    }
}

function Test-ExecutableFile {
    param([string] $Path)

    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        return $false
    }

    if ($script:IsWindowsPlatform) {
        return $true
    }

    try {
        & /usr/bin/test -x $Path 2>$null
        return $LASTEXITCODE -eq 0
    }
    catch {
        return $true
    }
}

function Get-StellarisExecutableName {
    if ($script:IsWindowsPlatform) {
        return 'stellaris.exe'
    }

    return 'stellaris'
}

function Test-GameDirectory {
    param([string] $Path)

    if ([String]::IsNullOrWhiteSpace($Path) -or
        -not (Test-Path -LiteralPath $Path -PathType Container)) {
        return $false
    }

    $executable = Join-Path $Path (Get-StellarisExecutableName)
    if (-not (Test-ExecutableFile $executable)) {
        return $false
    }

    if ($script:IsWindowsPlatform) {
        return $true
    }

    return Test-ElfFile $executable
}

function Get-SteamLibraryRoots {
    $roots = New-Object 'System.Collections.Generic.List[string]'
    $homePath = [Environment]::GetFolderPath('UserProfile')
    if ([String]::IsNullOrWhiteSpace($homePath)) {
        $homePath = $HOME
    }

    $environmentRoots = @($env:STEAM_ROOT, $env:STEAM_LIBRARY)
    foreach ($root in $environmentRoots) {
        if (-not [String]::IsNullOrWhiteSpace($root)) {
            [void]$roots.Add($root)
        }
    }

    if ($script:IsWindowsPlatform) {
        $registryEntries = @(
            [PSCustomObject]@{
                RegistryPath = 'HKCU:\Software\Valve\Steam'
                PropertyName = 'SteamPath'
            },
            [PSCustomObject]@{
                RegistryPath = 'HKLM:\Software\WOW6432Node\Valve\Steam'
                PropertyName = 'InstallPath'
            },
            [PSCustomObject]@{
                RegistryPath = 'HKLM:\Software\Valve\Steam'
                PropertyName = 'InstallPath'
            }
        )

        foreach ($entry in $registryEntries) {
            try {
                $values = Get-ItemProperty -LiteralPath $entry.RegistryPath -ErrorAction Stop
                $root = $values.($entry.PropertyName)
                if (-not [String]::IsNullOrWhiteSpace($root)) {
                    [void]$roots.Add([string]$root)
                }
            }
            catch {
                # This registry location is optional.
            }
        }

        if (-not [String]::IsNullOrWhiteSpace($env:ProgramFiles)) {
            [void]$roots.Add((Join-Path $env:ProgramFiles 'Steam'))
        }
        if (-not [String]::IsNullOrWhiteSpace(${env:ProgramFiles(x86)})) {
            [void]$roots.Add((Join-Path ${env:ProgramFiles(x86)} 'Steam'))
        }
    }
    else {
        if (-not [String]::IsNullOrWhiteSpace($homePath)) {
            [void]$roots.Add((Join-Path $homePath '.steam/steam'))
            [void]$roots.Add((Join-Path $homePath '.steam/root'))
            [void]$roots.Add((Join-Path $homePath '.local/share/Steam'))
            [void]$roots.Add((Join-Path $homePath '.var/app/com.valvesoftware.Steam/.local/share/Steam'))
            [void]$roots.Add((Join-Path $homePath 'Library/Application Support/Steam'))
            [void]$roots.Add((Join-Path $homePath 'SteamLibrary'))
        }
    }

    return $roots
}

function Get-SteamLibraries {
    $seen = New-Object -TypeName 'System.Collections.Generic.HashSet[string]' `
        -ArgumentList ([StringComparer]::OrdinalIgnoreCase)

    foreach ($candidate in @(Get-SteamLibraryRoots)) {
        $root = Get-CanonicalDirectory $candidate
        if ($null -eq $root -or -not $seen.Add($root)) {
            continue
        }

        Write-Output $root

        $vdf = Join-Path (Join-Path $root 'steamapps') 'libraryfolders.vdf'
        if (-not (Test-Path -LiteralPath $vdf -PathType Leaf)) {
            continue
        }

        try {
            foreach ($line in [System.IO.File]::ReadLines($vdf)) {
                if ($line -notmatch '^\s*"path"\s+"([^"]*)"') {
                    continue
                }

                $library = $Matches[1] -replace '\\\\', '\'
                $canonical = Get-CanonicalDirectory $library
                if ($null -ne $canonical -and $seen.Add($canonical)) {
                    Write-Output $canonical
                }
            }
        }
        catch {
            Write-WarningLine "could not read Steam library file: $vdf"
        }
    }
}

function Find-GameDirectory {
    if (-not [String]::IsNullOrWhiteSpace($env:GAMEDIR)) {
        if (Test-GameDirectory $env:GAMEDIR) {
            return Get-CanonicalDirectory $env:GAMEDIR
        }

        Write-WarningLine "GAMEDIR is not a valid Stellaris game directory: $env:GAMEDIR"
    }

    $directory = Get-CanonicalDirectory (Get-Location).Path
    while ($null -ne $directory) {
        if (Test-GameDirectory $directory) {
            return $directory
        }

        $parent = [System.IO.Directory]::GetParent($directory)
        if ($null -eq $parent) {
            break
        }
        $directory = $parent.FullName
    }

    foreach ($library in @(Get-SteamLibraries)) {
        $candidate = Join-Path (
            Join-Path (Join-Path $library 'steamapps') 'common'
        ) 'Stellaris'

        if (Test-GameDirectory $candidate) {
            return Get-CanonicalDirectory $candidate
        }
    }

    return $null
}

function Find-WorkshopDirectory {
    param([Parameter(Mandatory = $true)][string] $GameDirectory)

    if (-not [String]::IsNullOrWhiteSpace($env:STEAM_WORKSHOP)) {
        if (Test-Path -LiteralPath $env:STEAM_WORKSHOP -PathType Container) {
            return Get-CanonicalDirectory $env:STEAM_WORKSHOP
        }

        Write-WarningLine "STEAM_WORKSHOP does not exist: $env:STEAM_WORKSHOP"
    }

    $gameLibrary = $GameDirectory
    for ($level = 0; $level -lt 3; $level++) {
        $gameLibrary = [System.IO.Path]::GetDirectoryName($gameLibrary)
        if ([String]::IsNullOrWhiteSpace($gameLibrary)) {
            break
        }
    }

    if (-not [String]::IsNullOrWhiteSpace($gameLibrary)) {
        $candidate = Join-Path (
            Join-Path (Join-Path $gameLibrary 'steamapps') 'workshop/content'
        ) $script:STELLARIS_APP_ID
        if (Test-Path -LiteralPath $candidate -PathType Container) {
            return Get-CanonicalDirectory $candidate
        }
    }

    foreach ($library in @(Get-SteamLibraries)) {
        $candidate = Join-Path (
            Join-Path (Join-Path $library 'steamapps') 'workshop/content'
        ) $script:STELLARIS_APP_ID
        if (Test-Path -LiteralPath $candidate -PathType Container) {
            return Get-CanonicalDirectory $candidate
        }
    }

    return $null
}

function Resolve-InlineHook {
    param([Parameter(Mandatory = $true)][string] $Hook)

    $resolved = Get-CanonicalFile $Hook
    if ($null -ne $resolved) {
        return $resolved
    }

    if (-not [System.IO.Path]::IsPathRooted($Hook)) {
        $projectCandidate = Join-Path $script:repoRoot $Hook
        return Get-CanonicalFile $projectCandidate
    }

    return $null
}

function Add-InlineHooks {
    param([Parameter(Mandatory = $true)][string] $Value)

    if ($Value.Length -eq 0) {
        Exit-WithError 'option --inline_hook requires a non-empty value'
    }

    foreach ($hook in $Value.Split([char]',')) {
        if ($hook.Length -eq 0) {
            Exit-WithError 'empty path in --inline_hook'
        }
        if (-not $hook.EndsWith('.so', [StringComparison]::OrdinalIgnoreCase)) {
            Exit-WithError "inline hook is not a .so file: $hook"
        }

        $resolved = Resolve-InlineHook $hook
        if ($null -eq $resolved) {
            Exit-WithError "cannot find inline hook: $hook"
        }

        [void]$script:inlineHooks.Add($resolved)
    }
}

function Add-WorkshopMod {
    param(
        [Parameter(Mandatory = $true)][string] $ModId,
        [Parameter(Mandatory = $true)][bool] $Required,
        [Parameter(Mandatory = $true)] $ModArguments
    )

    $modPath = Join-Path $script:STEAM_WORKSHOP $ModId
    if (-not (Test-Path -LiteralPath $modPath -PathType Container)) {
        if ($Required) {
            Exit-WithError "required Workshop mod is not installed: $ModId"
        }

        Write-WarningLine "optional Workshop mod is not installed, skipping: $ModId"
        return
    }

    $canonical = Get-CanonicalDirectory $modPath
    [void]$ModArguments.Add("-mod=$canonical")
}

$argumentsToParse = @($args | ForEach-Object { [string]$_ })
$scriptPath = $MyInvocation.MyCommand.Path
if ([String]::IsNullOrWhiteSpace($scriptPath)) {
    Exit-WithError 'could not determine the script path'
}

$scriptDirectory = [System.IO.Path]::GetDirectoryName($scriptPath)
$repoRootOutput = & git -C $scriptDirectory rev-parse --show-toplevel 2>$null
if ($LASTEXITCODE -ne 0 -or $null -eq $repoRootOutput) {
    Exit-WithError 'could not locate the repository root'
}

$script:repoRoot = Get-CanonicalDirectory ([string]($repoRootOutput | Select-Object -First 1))
if ($null -eq $script:repoRoot) {
    Exit-WithError 'could not locate the repository root'
}

$loadOptional = $false
$useDlcLoad = $false
$continueSave = $false
$script:inlineHooks = New-Object 'System.Collections.Generic.List[string]'
$modArguments = New-Object 'System.Collections.Generic.List[string]'
$gameArguments = New-Object 'System.Collections.Generic.List[string]'

$argumentIndex = 0
while ($argumentIndex -lt $argumentsToParse.Count) {
    $argument = $argumentsToParse[$argumentIndex]

    switch -CaseSensitive ($argument) {
        '-h' {
            Show-Usage
            exit 0
        }
        '--help' {
            Show-Usage
            exit 0
        }
        '-f' {
            $loadOptional = $true
            $argumentIndex++
            continue
        }
        '-d' {
            $useDlcLoad = $true
            $argumentIndex++
            continue
        }
        '-c' {
            $continueSave = $true
            $argumentIndex++
            continue
        }
        '--inline_hook' {
            if (($argumentIndex + 1) -ge $argumentsToParse.Count) {
                Exit-WithError 'option --inline_hook requires an argument'
            }
            Add-InlineHooks $argumentsToParse[$argumentIndex + 1]
            $argumentIndex += 2
            continue
        }
        '--' {
            $argumentIndex++
            while ($argumentIndex -lt $argumentsToParse.Count) {
                [void]$gameArguments.Add($argumentsToParse[$argumentIndex])
                $argumentIndex++
            }
            break
        }
        default {
            if ($argument.StartsWith('--inline_hook=', [StringComparison]::Ordinal)) {
                Add-InlineHooks $argument.Substring('--inline_hook='.Length)
            }
            else {
                [void]$gameArguments.Add($argument)
            }
            $argumentIndex++
        }
    }
}

if ($useDlcLoad -and $loadOptional) {
    Write-WarningLine '-f has no effect with -d'
}

$GAMEDIR = Find-GameDirectory
if ($null -eq $GAMEDIR) {
    Exit-WithError 'could not locate the Stellaris game directory'
}

$script:STEAM_WORKSHOP = $null
if (-not $useDlcLoad) {
    $script:STEAM_WORKSHOP = Find-WorkshopDirectory $GAMEDIR
    if ($null -eq $script:STEAM_WORKSHOP) {
        Exit-WithError 'could not locate the Stellaris Steam Workshop directory'
    }

    Add-WorkshopMod $script:MUST_TRY_MOD_ID $true $modArguments

    if ($loadOptional) {
        foreach ($modId in $script:OPTIONAL_MODS_BEFORE_REPO) {
            Add-WorkshopMod $modId $false $modArguments
        }
    }

    $descriptor = Join-Path $script:repoRoot 'descriptor.mod'
    if (-not (Test-Path -LiteralPath $descriptor -PathType Leaf)) {
        Exit-WithError "project root does not contain descriptor.mod: $script:repoRoot"
    }
    [void]$modArguments.Add("-mod=$script:repoRoot")

    if ($loadOptional) {
        foreach ($modId in $script:OPTIONAL_MODS_AFTER_REPO) {
            Add-WorkshopMod $modId $false $modArguments
        }
    }
}

$launchArguments = New-Object 'System.Collections.Generic.List[string]'
foreach ($modArgument in $modArguments) {
    [void]$launchArguments.Add($modArgument)
}

if ($continueSave) {
    [void]$launchArguments.Add('--continuelastsave')
}

foreach ($gameArgument in $gameArguments) {
    [void]$launchArguments.Add($gameArgument)
}

if ($script:inlineHooks.Count -gt 0) {
    if ($script:IsWindowsPlatform) {
        Exit-WithError '--inline_hook requires LD_PRELOAD and is only supported on Unix-like systems'
    }

    $preloadPath = [String]::Join(':', $script:inlineHooks)
    if (-not [String]::IsNullOrEmpty($env:LD_PRELOAD)) {
        $preloadPath = $preloadPath + ':' + $env:LD_PRELOAD
    }
    $env:LD_PRELOAD = $preloadPath
}

Write-Output "game directory: $GAMEDIR"
if ($useDlcLoad) {
    Write-Output 'mod loading: dlc_load.json'
}
else {
    Write-Output 'mod loading: command line'
}
if ($continueSave) {
    Write-Output 'save loading: most recent save'
}

$executable = Join-Path $GAMEDIR (Get-StellarisExecutableName)
$exitCode = 0
Push-Location -LiteralPath $GAMEDIR
try {
    & $executable @launchArguments
    $exitCode = $LASTEXITCODE
}
finally {
    Pop-Location
}

exit $exitCode
