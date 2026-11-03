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
 if($child.ExitCode -or $output.Contains('Not applied:')){throw "Fixture CLI refused $mode; exit=$($child.ExitCode)"};$child.Dispose()
}
$before=Get-ReadbackSha "$App/config.ini";Run preview
if((Get-ReadbackSha "$App/config.ini") -cne $before){throw 'Preview changed config'}
Run apply
$after=Get-ReadbackSha "$App/config.ini";if($after -ceq $before){throw 'Apply did not write fixture profile'}
$text=Get-Content "$App/config.ini" -Raw
if($text -notmatch '(?m)^Schema\s*=\s*2\s*$' -or $text -notmatch 'cal=4096\.\.32768\.\.61440' -or $text -notmatch '(?m)^Revision\s*=\s*(\S+)') {throw 'Schema2 fixture calibration/revision missing'}
$revision=$Matches[1]
@{pass=$true;beforeSha256=$before;afterSha256=$after;revision=$revision;synthetic=$true}|ConvertTo-Json|Set-Content "$Evidence/apply-result.json"
