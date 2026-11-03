# Pure policy checks: compiles native declarations but invokes no Win32 API.
$ErrorActionPreference='Stop'
Add-Type -Path "$PSScriptRoot/../tools/rig/FzeroLive.cs"
$n=0
function Check($value,$why){if(-not $value){throw $why};$script:n++}
Check ([FzeroLive]::ForegroundAllowed('owner','owner',$null,$false)) 'Prelaunch foreground must remain admissible'
Check ([FzeroLive]::ForegroundAllowed('owner','game',$null,$true)) 'Exact owned natural activation refused'
Check (-not [FzeroLive]::ForegroundAllowed('owner','foreign',$null,$false)) 'Foreign transition admitted'
Check (-not [FzeroLive]::ForegroundAllowed('owner','owner','game',$false)) 'Return to original after activation admitted'
Check ([FzeroLive]::ForegroundAllowed('owner','game','game',$true)) 'Fixed game foreground refused'
foreach($count in 0..2){Check ([FzeroLive]::ExplainReset($count,0,0,90000,$true)) 'Three bounded startup resets must be allowed'}
foreach($args in @(@(3,0,0,1,$true),@(0,1,0,1,$true),@(0,0,1,1,$true),@(0,0,0,-1,$true),@(0,0,0,90001,$true),@(0,0,0,1,$false))){
 Check (-not [FzeroLive]::ExplainReset($args[0],$args[1],$args[2],$args[3],$args[4])) 'Unsafe reset admitted'
}
@{pass=$true;checks=$n;scope='pure policy, no window/hook/input/lease/game'}|ConvertTo-Json
