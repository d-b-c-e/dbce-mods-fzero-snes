#Runs only against the explicitly supplied private clone, inside parent's job.
param([Parameter(Mandatory)][string]$App,[Parameter(Mandatory)][string]$Profile,[Parameter(Mandatory)][string]$Evidence)
$ErrorActionPreference='Stop'
. "$PSScriptRoot/Readback-Contract.ps1"
function Run([string]$mode){
 $start=[Diagnostics.ProcessStartInfo]::new((Join-Path $App 'controls-setup/Wheelkit.StandaloneSetup.exe'))
 $start.UseShellExecute=$false;$start.CreateNoWindow=$true;$start.WorkingDirectory=$App
 $start.RedirectStandardInput=$true;$start.RedirectStandardOutput=$true;$start.RedirectStandardError=$true
 foreach($a in @($mode,'--game','f-zero','--game-dir',$App,'--profile',$Profile)){[void]$start.ArgumentList.Add($a)}
 $child=[Diagnostics.Process]::new();$child.StartInfo=$start;[void]$child.Start()
 $outTask=$child.StandardOutput.ReadToEndAsync();$errTask=$child.StandardError.ReadToEndAsync()
 if($mode -eq 'apply'){$child.StandardInput.WriteLine('APPLY')};$child.StandardInput.Close()
 if(-not $child.WaitForExit(15000)){throw 'Pinned fixture CLI timeout; parent owns job cleanup'}
 $output=$outTask.GetAwaiter().GetResult();$errorText=$errTask.GetAwaiter().GetResult()
 [IO.File]::WriteAllText("$Evidence/$mode.stdout.txt",$output);[IO.File]::WriteAllText("$Evidence/$mode.stderr.txt",$errorText)
 # Pinned d0f333 writes Native2 cal= endpoints, but its older generic disclosure
 # allowlist omits f-zero. Preserve that contradictory advisory; do not treat it
 # as calibration proof. The real consumer fixture/runtime readback owns proof.
 $gaps=@($output -split '\r?\n'|Where-Object {$_ -like 'Not applied:*'})
 $knownAdvisory="Not applied: calibration: Calibration not transferred: this game's controls writer does not carry it, so the game uses its own range for these controls. (brake, steer, throttle)"
 if($child.ExitCode -or @($gaps|Where-Object {$_ -cne $knownAdvisory}).Count){throw "Fixture CLI refused $mode; exit=$($child.ExitCode)"};$child.Dispose()
}
$before=Get-ReadbackSha "$App/config.ini";Run preview
if((Get-ReadbackSha "$App/config.ini") -cne $before){throw 'Preview changed config'}
Run apply
$after=Get-ReadbackSha "$App/config.ini";if($after -ceq $before){throw 'Apply did not write fixture profile'}
$text=Get-Content "$App/config.ini" -Raw
if($text -notmatch '(?m)^Schema\s*=\s*2\s*$' -or $text -notmatch 'cal=4096\.\.32768\.\.61440' -or $text -notmatch '(?m)^Revision\s*=\s*(\S+)') {throw 'Schema2 fixture calibration/revision missing'}
$revision=$Matches[1]
@{pass=$true;beforeSha256=$before;afterSha256=$after;revision=$revision;synthetic=$true;calibrationDisclosure='Pinned helper has stale generic disclosure; actual cal= bytes and native consumer evidence recorded separately.'}|ConvertTo-Json|Set-Content "$Evidence/apply-result.json"
