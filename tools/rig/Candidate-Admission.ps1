# Snapshot/SHA helpers are provided by Readback-Contract.ps1; this admits files only.
function Get-FzeroCandidate([string]$PackageRoot,[string]$BuildReceipt,[string]$ExpectedSource) {
    $launcher='FZeroSNESRecomp.exe';$build=$null
    if($BuildReceipt){
        Assert-ReadbackPlain $BuildReceipt
        $build=Get-Content -LiteralPath $BuildReceipt -Raw|ConvertFrom-Json -DateKind String
        if($ExpectedSource -cnotmatch '^[0-9a-f]{40}$' -or $build.sourceRevision -cne $ExpectedSource -or $build.dirty -ne $false -or $build.bsDeluxe -ne $true -or $build.privateDeluxe.helperSource -cne 'd0f333c60892dc44beb36e277b19bd6a650507b7' -or $build.privateControlsSetup.sourceCommit -cne $build.privateDeluxe.helperSource){throw 'Private source/helper/Deluxe receipt mismatch'}
        $receiptPath=Join-Path $PackageRoot 'install-receipt.json'
        if((Get-ReadbackSha $receiptPath).ToLowerInvariant() -cne $build.privateDeluxe.installReceiptSha256){throw 'Native2 install receipt hash differs'}
        $installed=Get-Content -LiteralPath $receiptPath -Raw|ConvertFrom-Json -DateKind String
        $launcher=$installed.launcher
        if($launcher -cne 'FZeroSNESRecomp-wheel-launcher.exe' -or $installed.sourceCommit -cne $build.sourceRevision -or $installed.nativeControls.schema -ne 2 -or $installed.nativeControls.adapter -cne 'fzero-raw-wheel-2' -or $installed.nativeControls.consumer -cne $launcher){throw 'Native2 consumer/source mismatch'}
        $files=$build.files
        if($installed.launcherSha256 -ine $files.$launcher -or $installed.privateControlsSetup.manifestSha256 -ine $build.privateControlsSetup.manifestSha256){throw 'Private game/helper linkage differs'}
    }else{
        $manifest=Get-Content -LiteralPath (Join-Path $PackageRoot manifest.json) -Raw|ConvertFrom-Json -DateKind String
        $build=$manifest.build;$files=$manifest.files
        if($build.sourceRevision -cne '9dc40e99f5d3b5bc6d17f44c5ee2fbfd0dba2914' -or $build.bsDeluxe -ne $false -or $build.dirty -ne $false){throw 'Only receipted Oct30 stock candidate admitted on stock route'}
    }
    $known=@{};foreach($m in $files.PSObject.Properties){
        $p=Join-Path $PackageRoot $m.Name
        if(-not(Test-ReadbackWithin $p $PackageRoot)){throw 'Candidate file escaped root'}
        Assert-ReadbackPlain $p
        if((Get-ReadbackSha $p) -ine $m.Value){throw "Candidate bytes differ: $($m.Name)"}
        $known[[IO.Path]::GetFullPath($p)]=$true
    }
    foreach($f in Get-ChildItem -LiteralPath $PackageRoot -File -Recurse -Force){
        if(-not $known.ContainsKey($f.FullName) -and -not(-not $BuildReceipt -and $f.Name -ceq 'manifest.json' -and $f.DirectoryName -ceq [IO.Path]::GetFullPath($PackageRoot))){throw "Unlisted candidate file: $($f.FullName)"}
    }
    @{source=$build.sourceRevision;launcher=$launcher;sha256=$files.$launcher;private=[bool]$BuildReceipt;files=$known.Count}
}
