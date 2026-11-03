# Independent child: expected UTC ticks are numeric strings, so no date parser or restore helper is shared.
param([Parameter(Mandatory)][string]$Expected,[Parameter(Mandatory)][string]$Report)
$ErrorActionPreference='Stop'
$items=Get-Content -LiteralPath $Expected -Raw|ConvertFrom-Json
$errors=@();$count=0
foreach($item in $items){
    $count++
    if(-not [IO.File]::Exists($item.path)){$errors+='missing: '+$item.path;continue}
    if((Get-FileHash -LiteralPath $item.path -Algorithm SHA256).Hash -cne $item.sha256){$errors+='bytes: '+$item.path}
    if([IO.File]::GetLastWriteTimeUtc($item.path).Ticks -ne [long]$item.utcTicks){$errors+='UTC ticks: '+$item.path}
    if([int][IO.File]::GetAttributes($item.path) -ne [int]$item.attributes){$errors+='attributes: '+$item.path}
}
$result=[ordered]@{pass=($errors.Count -eq 0);files=$count;errors=$errors;method='separate process; numeric UTC ticks; no shared restore/date parser'}
$result|ConvertTo-Json -Depth 4|Set-Content -LiteralPath $Report
$result|ConvertTo-Json -Depth 4
if($errors.Count){exit 1}
