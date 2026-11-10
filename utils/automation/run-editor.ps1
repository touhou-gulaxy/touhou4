Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$scriptDirectory = $PSScriptRoot
$repoRootOutput = & git -C $scriptDirectory rev-parse --show-toplevel 2>$null
if ($LASTEXITCODE -ne 0 -or $null -eq $repoRootOutput) {
    [Console]::Error.WriteLine('error: could not locate the repository root')
    exit 1
}

$repoRoot = [string]($repoRootOutput | Select-Object -First 1)
$editorScript = Join-Path $scriptDirectory 'editor.ps1'
& $editorScript "-mod=$repoRoot"
$exitCode = $LASTEXITCODE
if ($null -eq $exitCode) {
    $exitCode = 0
}

exit $exitCode
