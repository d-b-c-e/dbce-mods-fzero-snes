# Pure ownership helpers plus the existing build-time bundle admission CLI.
function Test-PrivateControlsOwnedPath([string]$Name) {
    if (!$Name -or $Name.Length -gt 512 -or !$Name.StartsWith('controls-setup/', [StringComparison]::Ordinal) -or
        $Name -match '[\\\x00-\x1f<>:"|?*]') { return $false }
    foreach ($part in $Name.Split('/')) {
        if (!$part -or $part -in '.', '..' -or $part.EndsWith('.') -or $part.EndsWith(' ') -or
            $part -match '^(?i:CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])(?:\.|$)') { return $false }
    }
    return $true
}
function Assert-PrivateControlsNoLink([string]$Path) {
    $cursor = [IO.Path]::GetFullPath($Path)
    while ($cursor) {
        if ((Test-Path -LiteralPath $cursor) -and ((Get-Item -LiteralPath $cursor -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)) { throw 'Controls setup path traverses a link.' }
        $cursor = [IO.Path]::GetDirectoryName($cursor)
    }
}
function Assert-PrivateControlsTransition([bool]$Requested, $Previous, [string[]]$Incoming=$null) {
    $prior = @($Previous.files | Where-Object { $_.name -match '^(?i:controls-setup)[\\/]' } | ForEach-Object { $_.name })
    if (@($prior | Where-Object { !(Test-PrivateControlsOwnedPath $_) }).Count) { throw 'Previous controls setup inventory is invalid.' }
    if (!$Requested -and ($prior.Count -or $Previous.privateControlsSetup)) { throw 'Keep -PrivateControlsSetup or roll back its owned install first; helper ownership cannot be withdrawn implicitly.' }
    if ($Requested -and $null -ne $Incoming -and @($prior | Where-Object { $_ -cnotin $Incoming }).Count) { throw 'Incoming controls setup withdraws owned paths; roll back the previous helper install first.' }
}
function Private-ControlsSetupSources([string]$SourceRoot, [string]$BuildRoot, [string]$LinkedText, [string]$PythonExecutable) {
    $bundle = Join-Path $BuildRoot 'controls-setup'
    $python = Get-Command $PythonExecutable -CommandType Application -ErrorAction Stop
    $temporary = Join-Path ([IO.Path]::GetTempPath()) ('fzero-setup-' + [guid]::NewGuid().ToString('N') + '.h')
    if (Test-Path -LiteralPath $temporary) { throw 'Fresh admission output required.' }
    try {
        # Reuse the existing complete manifest/hash/path admission, not a second parser.
        & $python.Source (Join-Path $SourceRoot 'tools/setup_bundle.py') --bundle $bundle --header $temporary
        if ($LASTEXITCODE -ne 0) { throw 'Complete private controls setup admission failed. Nothing installed.' }
        $manifestPath = Join-Path $bundle 'bundle-manifest.json'
        $manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
        $manifestHash = (Get-FileHash -LiteralPath $manifestPath).Hash.ToLowerInvariant()
        $exeHash = (Get-FileHash -LiteralPath (Join-Path $bundle 'DbceModSetup.exe')).Hash.ToLowerInvariant()
        if (!$LinkedText.Contains($manifestHash + [char]0) -or !$LinkedText.Contains($exeHash + [char]0)) { throw 'Built launcher is not bound to this exact controls setup bundle.' }
        $files = @{}; $expected = @{}
        foreach ($entry in @($manifest.files) + @([pscustomobject]@{path='bundle-manifest.json';sha256=$manifestHash})) {
            $name = 'controls-setup/' + $entry.path
            if (!(Test-PrivateControlsOwnedPath $name) -or $files.ContainsKey($name)) { throw 'Invalid controls setup ownership path.' }
            $files[$name] = Join-Path $bundle $entry.path
            $expected[$name] = $entry.sha256.ToUpperInvariant()
        }
        return @{files=$files; expected=$expected; metadata=[ordered]@{sourceCommit=$manifest.sourceCommit;manifestSha256=$manifestHash;exeSha256=$exeHash;folder='controls-setup';game='f-zero';waitForProcessContract=$manifest.waitForProcessContract;waitForProcessTimeoutMs=$manifest.waitForProcessTimeoutMs}}
    } finally { if (Test-Path -LiteralPath $temporary) { Remove-Item -LiteralPath $temporary -Force } }
}
