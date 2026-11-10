Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

# what this should do:
# 1. if not in stellaris game dir, locate stellaris game-dir ($GAMEDIR)
# 2. locate steam workshop ($STEAM_WORKSHOP)
# 3. locate paradox stellaris mod-dir ($PARADOX_MODDIR)
# 4. if step 1 is executed, self-deploy this script to game-dir
# 5. if already deployed, use the deployed script directly when applicable
# 6. try engage stellaris particle editor
# note:
# editor should support: -mod=<mod> argument
# first, treat <mod> as mod id, and try to find $MOUNT_PATH=$STEAM_WORKSHOP/<mod> dir.
# if not find, try to find $MOUNT_PATH=$PARADOX_MODDIR/<mod> dir
# if still not find, check if $MOUNT_PATH=<mod> dir exists
# if all failed, report an error and exit
# if any dir exists, engage stellaris game with:
# <stellaris-executable> -editor -filterfile=$GAMEDIR/previewer_assets/previewer_filefilter.txt -mount=$MOUNT_PATH

$script:STELLARIS_APP_ID = '281990'
$script:IsWindowsPlatform =
    [System.Environment]::OSVersion.Platform -eq [System.PlatformID]::Win32NT

function Show-Usage {
    $usage = @'
usage: editor.ps1 -mod=<mod>

  <mod> can be a Steam Workshop ID, a mod directory under the Paradox
  Stellaris mod directory, or an explicit mod directory path.
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
    $filterFile = Join-Path (
        Join-Path $Path 'previewer_assets'
    ) 'previewer_filefilter.txt'

    if (-not (Test-ExecutableFile $executable)) {
        return $false
    }
    if (-not (Test-Path -LiteralPath $filterFile -PathType Leaf)) {
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

    if (Test-GameDirectory $script:scriptDirectory) {
        return $script:scriptDirectory
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

function Get-ParadoxModCandidates {
    $candidates = New-Object 'System.Collections.Generic.List[string]'

    if (-not [String]::IsNullOrWhiteSpace($env:PARADOX_MODDIR)) {
        [void]$candidates.Add($env:PARADOX_MODDIR)
    }

    $homePath = [Environment]::GetFolderPath('UserProfile')
    if ([String]::IsNullOrWhiteSpace($homePath)) {
        $homePath = $HOME
    }

    $dataHome = $env:XDG_DATA_HOME
    if ([String]::IsNullOrWhiteSpace($dataHome)) {
        $dataHome = Join-Path $homePath '.local/share'
    }

    [void]$candidates.Add(
        (Join-Path (Join-Path (Join-Path $dataHome 'Paradox Interactive') 'Stellaris') 'mod')
    )
    [void]$candidates.Add(
        (Join-Path (Join-Path (Join-Path $homePath '.paradoxinteractive') 'Stellaris') 'mod')
    )

    $documents = [Environment]::GetFolderPath(
        [Environment+SpecialFolder]::MyDocuments
    )
    if ([String]::IsNullOrWhiteSpace($documents)) {
        $documents = Join-Path $homePath 'Documents'
    }
    [void]$candidates.Add(
        (Join-Path (Join-Path (Join-Path $documents 'Paradox Interactive') 'Stellaris') 'mod')
    )

    return $candidates
}

function Find-ParadoxModDirectory {
    foreach ($candidate in @(Get-ParadoxModCandidates)) {
        $canonical = Get-CanonicalDirectory $candidate
        if ($null -ne $canonical) {
            return $canonical
        }
    }

    return $null
}

function Resolve-MountPath {
    param([Parameter(Mandatory = $true)][string] $ModArgument)

    if (-not [String]::IsNullOrWhiteSpace($script:STEAM_WORKSHOP)) {
        $candidate = Join-Path $script:STEAM_WORKSHOP $ModArgument
        $canonical = Get-CanonicalDirectory $candidate
        if ($null -ne $canonical) {
            return $canonical
        }
    }

    if (-not [String]::IsNullOrWhiteSpace($script:PARADOX_MODDIR)) {
        $candidate = Join-Path $script:PARADOX_MODDIR $ModArgument
        $canonical = Get-CanonicalDirectory $candidate
        if ($null -ne $canonical) {
            return $canonical
        }
    }

    return Get-CanonicalDirectory $ModArgument
}

function Test-FilesEqual {
    param(
        [Parameter(Mandatory = $true)][string] $Left,
        [Parameter(Mandatory = $true)][string] $Right
    )

    if (-not (Test-Path -LiteralPath $Left -PathType Leaf) -or
        -not (Test-Path -LiteralPath $Right -PathType Leaf)) {
        return $false
    }

    try {
        $leftHash = (Get-FileHash -LiteralPath $Left -Algorithm SHA256).Hash
        $rightHash = (Get-FileHash -LiteralPath $Right -Algorithm SHA256).Hash
        return $leftHash -eq $rightHash
    }
    catch {
        return $false
    }
}

$argumentsToParse = @($args | ForEach-Object { [string]$_ })
$modArgument = ''
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
        '-mod' {
            if (($argumentIndex + 1) -ge $argumentsToParse.Count) {
                Exit-WithError 'option -mod requires an argument'
            }
            $modArgument = $argumentsToParse[$argumentIndex + 1]
            $argumentIndex += 2
            continue
        }
        default {
            if ($argument.StartsWith('-mod=', [StringComparison]::Ordinal)) {
                $modArgument = $argument.Substring('-mod='.Length)
            }
            else {
                Exit-WithError "unknown argument: $argument"
            }
            $argumentIndex++
        }
    }
}

if ([String]::IsNullOrEmpty($modArgument)) {
    $usage = @(Show-Usage) -join [Environment]::NewLine
    [Console]::Error.WriteLine($usage)
    exit 2
}

$scriptPath = $MyInvocation.MyCommand.Path
if ([String]::IsNullOrWhiteSpace($scriptPath)) {
    Exit-WithError 'could not determine the script path'
}

$script:scriptDirectory = Get-CanonicalDirectory (
    [System.IO.Path]::GetDirectoryName($scriptPath)
)
$step1Executed = $false
$currentDirectory = Get-CanonicalDirectory (Get-Location).Path

if (Test-GameDirectory $currentDirectory) {
    $GAMEDIR = $currentDirectory
}
elseif (Test-GameDirectory $script:scriptDirectory) {
    $GAMEDIR = $script:scriptDirectory
}
else {
    $GAMEDIR = Find-GameDirectory
    if ($null -eq $GAMEDIR) {
        Exit-WithError 'could not locate the Stellaris game directory'
    }
    $step1Executed = $true
}

$script:STEAM_WORKSHOP = Find-WorkshopDirectory $GAMEDIR
$script:PARADOX_MODDIR = Find-ParadoxModDirectory

$MOUNT_PATH = Resolve-MountPath $modArgument
if ($null -eq $MOUNT_PATH) {
    Exit-WithError "could not locate mod '$modArgument' in Workshop, Paradox mod directory, or as a path"
}

if ($step1Executed) {
    $deployedScript = Join-Path $GAMEDIR 'editor.ps1'
    if (
        $scriptPath -ne $deployedScript -and
        -not (Test-FilesEqual $scriptPath $deployedScript)
    ) {
        try {
            Copy-Item -LiteralPath $scriptPath -Destination $deployedScript -Force
            if (-not $script:IsWindowsPlatform) {
                try {
                    & chmod 755 $deployedScript 2>$null
                }
                catch {
                    # The .ps1 can still be launched through pwsh without chmod.
                }
            }
            Write-Output "deployed editor.ps1 to: $deployedScript"
        }
        catch {
            Exit-WithError "could not deploy editor.ps1 to: $deployedScript"
        }
    }
}

Write-Output "game directory: $GAMEDIR"
if ($null -eq $script:STEAM_WORKSHOP) {
    Write-Output 'Steam Workshop: not found'
}
else {
    Write-Output "Steam Workshop: $script:STEAM_WORKSHOP"
}
if ($null -eq $script:PARADOX_MODDIR) {
    Write-Output 'Paradox mod directory: not found'
}
else {
    Write-Output "Paradox mod directory: $script:PARADOX_MODDIR"
}
Write-Output "mount path: $MOUNT_PATH"

$executable = Join-Path $GAMEDIR (Get-StellarisExecutableName)
$filterFile = Join-Path (
    Join-Path $GAMEDIR 'previewer_assets'
) 'previewer_filefilter.txt'
$launchArguments = @(
    '-editor',
    "-filterfile=$filterFile",
    "-mount=$MOUNT_PATH"
)

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
