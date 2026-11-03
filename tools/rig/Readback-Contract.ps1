# Pure guards and file transactions. Dot-sourcing does not load native code or touch owner state.
function Get-ReadbackSha([string]$Path) { (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash }
function Convert-ReadbackUtc($Value) {
    # ConvertFrom-Json can materialize ISO dates as DateTime. Never stringify those: it loses ticks and Kind.
    if($Value -is [DateTimeOffset]){return $Value.UtcDateTime}
    $time=if($Value -is [DateTime]){$Value}else{[DateTime]::Parse([string]$Value,[Globalization.CultureInfo]::InvariantCulture,[Globalization.DateTimeStyles]::RoundtripKind)}
    if($time.Kind -eq [DateTimeKind]::Unspecified){throw 'Snapshot timestamp must retain its UTC or offset kind'}
    return $time.ToUniversalTime()
}
function Test-ReadbackWithin([string]$Path,[string]$Root) {
    $p=[IO.Path]::GetFullPath($Path);$r=[IO.Path]::GetFullPath($Root).TrimEnd('\')
    $p.Equals($r,[StringComparison]::OrdinalIgnoreCase) -or $p.StartsWith($r+'\',[StringComparison]::OrdinalIgnoreCase)
}
function Assert-ReadbackPlain([string]$Path) {
    for($p=[IO.Path]::GetFullPath($Path);$p;$p=[IO.Path]::GetDirectoryName($p)) {
        if((Test-Path -LiteralPath $p) -and ((Get-Item -LiteralPath $p -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)){throw "Linked path refused: $p"}
    }
}
function Get-ReadbackAbortReason([int]$Physical=0,[int]$Injected=0,[bool]$TickChanged=$false,[string]$Before,[string]$Now,[bool]$IdentityMatches=$true) {
    if($Physical){return 'physical-input'}
    if($Injected){return 'injected-os-input'} # R0/R1/R2 never need OS injection.
    if($TickChanged){return 'unexplained-last-input-tick'}
    if(-not $Before -or $Before -cne $Now){return 'foreground-changed-or-unknown'}
    if(-not $IdentityMatches){return 'owned-process-identity-changed'}
    return $null
}
function Save-ReadbackSnapshot([object[]]$Roots,[string]$Evidence) {
    $e=[IO.Path]::GetFullPath($Evidence);if(Test-Path -LiteralPath $e){throw 'Fresh snapshot directory required'}
    $seen=@()
    foreach($r in $Roots){
        if($r.kind -notin @('file','directory')){throw 'Unknown snapshot root kind'}
        foreach($prior in $seen){if((Test-ReadbackWithin $r.path $prior) -or (Test-ReadbackWithin $prior $r.path)){throw 'Overlapping snapshot roots'}}
        $seen+=[IO.Path]::GetFullPath($r.path)
        if(Test-Path -LiteralPath $r.path){$item=Get-Item -LiteralPath $r.path -Force;if($item.PSIsContainer -ne ($r.kind -eq 'directory')){throw 'Snapshot root type differs'}}
    }
    foreach($r in $Roots){if((Test-ReadbackWithin $e $r.path) -or (Test-ReadbackWithin $r.path $e)){throw 'Evidence and owner root overlap'};Assert-ReadbackPlain $r.path}
    [void][IO.Directory]::CreateDirectory($e);$entries=@();$i=0
    foreach($r in $Roots){
        $path=[IO.Path]::GetFullPath($r.path);$exists=Test-Path -LiteralPath $path;$files=@();$directories=@()
        if($exists){
            if($r.kind -eq 'file'){$list=@(Get-Item -LiteralPath $path -Force)}else{$list=@(Get-ChildItem -LiteralPath $path -File -Recurse -Force);$directories=@(Get-ChildItem -LiteralPath $path -Directory -Recurse -Force|ForEach-Object {Assert-ReadbackPlain $_.FullName;$_.FullName})}
            foreach($f in $list){Assert-ReadbackPlain $f.FullName;$relative=if($r.kind -eq 'file'){'payload'}else{$f.FullName.Substring($path.Length).TrimStart('\')};$backup=Join-Path (Join-Path $e $i) $relative;[void][IO.Directory]::CreateDirectory((Split-Path $backup));[IO.File]::Copy($f.FullName,$backup,$false);$sha=Get-ReadbackSha $f.FullName;if((Get-ReadbackSha $backup) -cne $sha){throw 'Snapshot copy hash differs'};$files+=@{path=$f.FullName;backup=$backup;sha256=$sha;attributes=[int]$f.Attributes;lastWriteUtc=$f.LastWriteTimeUtc.ToString('o')}}
        }
        $entries+=@{path=$path;kind=$r.kind;existed=$exists;directories=$directories;files=$files};$i++
    }
    $manifest=@{schema='fzero-owner-snapshot@1';roots=$entries};$manifest|ConvertTo-Json -Depth 9|Set-Content -LiteralPath (Join-Path $e manifest.json)
    return $manifest
}
function Restore-ReadbackSnapshot($Snapshot,[string]$AfterEvidence) {
    foreach($r in $Snapshot.roots){if((Test-ReadbackWithin $AfterEvidence $r.path) -or (Test-ReadbackWithin $r.path $AfterEvidence)){throw 'Restore evidence and owner root overlap'}}
    # Validate every backup before any restoration or removal, including roots restored later.
    foreach($r in $Snapshot.roots){foreach($f in $r.files){
        if(-not(Test-ReadbackWithin $f.path $r.path)){throw 'Snapshot file escaped root'}
        Assert-ReadbackPlain $f.backup
        if((Get-ReadbackSha $f.backup) -cne $f.sha256){throw 'Original backup changed; retain custody'}
        [void](Convert-ReadbackUtc $f.lastWriteUtc)
    }}
    [void][IO.Directory]::CreateDirectory($AfterEvidence)
    foreach($r in $Snapshot.roots){
        Assert-ReadbackPlain $r.path;$known=@{};foreach($f in $r.files){$known[$f.path]=$true}
        $current=if(-not(Test-Path -LiteralPath $r.path)){@()}elseif($r.kind -eq 'file'){@(Get-Item -LiteralPath $r.path -Force)}else{@(Get-ChildItem -LiteralPath $r.path -File -Recurse -Force)}
        foreach($f in $current){if(-not(Test-ReadbackWithin $f.FullName $r.path)){throw 'Restore path escaped root'};Assert-ReadbackPlain $f.FullName;if(-not $known.ContainsKey($f.FullName)){$archive=Join-Path $AfterEvidence ([Guid]::NewGuid().ToString('N')+'-'+$f.Name);[IO.File]::Copy($f.FullName,$archive,$false);[IO.File]::SetAttributes($f.FullName,[IO.FileAttributes]::Normal);[IO.File]::Delete($f.FullName)}}
        foreach($f in $r.files){
            Assert-ReadbackPlain $f.path
            [void][IO.Directory]::CreateDirectory((Split-Path $f.path))
            if(Test-Path -LiteralPath $f.path){
                if((Get-ReadbackSha $f.path) -cne $f.sha256){
                    [IO.File]::Copy($f.path,(Join-Path $AfterEvidence ([Guid]::NewGuid().ToString('N')+'-'+[IO.Path]::GetFileName($f.path))),$false)
                    $attributes=[int][IO.File]::GetAttributes($f.path)
                    if($attributes -band 1){[IO.File]::SetAttributes($f.path,[Enum]::ToObject([IO.FileAttributes],($attributes -band (-bnot 1))))}
                    # Overwrite existing bytes without replacing cloud-pin flags, ACL or file identity.
                    [IO.File]::WriteAllBytes($f.path,[IO.File]::ReadAllBytes($f.backup))
                }
            }else{[IO.File]::Copy($f.backup,$f.path,$false)}
            $lastWriteUtc=Convert-ReadbackUtc $f.lastWriteUtc
            if([IO.File]::GetLastWriteTimeUtc($f.path).Ticks -ne $lastWriteUtc.Ticks){[IO.File]::SetLastWriteTimeUtc($f.path,$lastWriteUtc)}
            # Windows exposes legitimate cloud flags not listed in the .NET FileAttributes enum.
            if([int][IO.File]::GetAttributes($f.path) -ne $f.attributes){[IO.File]::SetAttributes($f.path,[Enum]::ToObject([IO.FileAttributes],[int]$f.attributes))}
        }
        if($r.kind -eq 'directory'){
            if($r.existed){[void][IO.Directory]::CreateDirectory($r.path);foreach($d in $r.directories){[void][IO.Directory]::CreateDirectory($d)}}
            if(Test-Path -LiteralPath $r.path){$dirs=@(Get-ChildItem -LiteralPath $r.path -Directory -Recurse -Force|Sort-Object {$_.FullName.Length} -Descending);foreach($d in $dirs){Assert-ReadbackPlain $d.FullName;if($d.FullName -notin $r.directories -and @(Get-ChildItem -LiteralPath $d.FullName -Force).Count -eq 0){[IO.Directory]::Delete($d.FullName,$false)}};if(-not $r.existed -and @(Get-ChildItem -LiteralPath $r.path -Force).Count -eq 0){[IO.Directory]::Delete($r.path,$false)}}
        }
    }
}
function Test-ReadbackRestored($Snapshot) {
    $errors=@();$checked=0
    foreach($r in $Snapshot.roots){
        if((Test-Path -LiteralPath $r.path) -ne [bool]$r.existed){$errors+='root presence: '+$r.path}
        $known=@($r.files|ForEach-Object path)
        foreach($f in $r.files){
            $checked++
            if(-not(Test-Path -LiteralPath $f.path) -or (Get-ReadbackSha $f.path) -cne $f.sha256){$errors+='hash: '+$f.path;continue}
            if([IO.File]::GetLastWriteTimeUtc($f.path).Ticks -ne (Convert-ReadbackUtc $f.lastWriteUtc).Ticks){$errors+='UTC ticks: '+$f.path}
            if([int][IO.File]::GetAttributes($f.path) -ne [int]$f.attributes){$errors+='attributes: '+$f.path}
        }
        if($r.kind -eq 'directory' -and (Test-Path -LiteralPath $r.path)){
            foreach($f in Get-ChildItem -LiteralPath $r.path -File -Recurse -Force){if($f.FullName -notin $known){$errors+='new file: '+$f.FullName}}
            $dirs=@(Get-ChildItem -LiteralPath $r.path -Directory -Recurse -Force|ForEach-Object FullName)
            if(@(Compare-Object @($r.directories) $dirs).Count){$errors+='directory set: '+$r.path}
        }
    }
    return [pscustomobject]@{pass=($errors.Count -eq 0);files=$checked;errors=$errors;method='independent fresh SHA-256, UTC ticks, attributes and presence enumeration'}
}
