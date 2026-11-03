# Real CMake parser regression; no configure/compiler/project/lease/device.
param([string]$Evidence)
$ErrorActionPreference='Stop'
. "$PSScriptRoot/../tools/rig/Cmake-Paths.ps1"
if(-not $Evidence -or (Test-Path $Evidence)){throw 'Fresh explicit evidence folder required'}
[void][IO.Directory]::CreateDirectory($Evidence);$Evidence=[IO.Path]::GetFullPath($Evidence)
$old='E:\Source\_work\fixture\SDL3-3.4.10';$new=Convert-FzeroCmakePath $old
$fail=$false;try{Assert-FzeroCmakePath $old}catch{$fail=$true};if(-not $fail){throw 'Harness admitted the original invalid path'}
Assert-FzeroCmakePath $new
foreach($case in @(@{name='original';path=$old},@{name='normalized';path=$new})){
 $file=Join-Path $Evidence ($case.name+'.cmake')
 [IO.File]::WriteAllText($file,('cmake_minimum_required(VERSION 3.20)'+"`n"+'set(SDL_SOURCE "'+$case.path+'")'+"`n"))
 & cmake -P $file 1> (Join-Path $Evidence ($case.name+'.stdout.txt')) 2> (Join-Path $Evidence ($case.name+'.stderr.txt'))
 if($case.name -eq 'original'){$original=$LASTEXITCODE}else{$normalized=$LASTEXITCODE}
}
if($original -eq 0 -or $normalized -ne 0){throw 'Real CMake parser did not distinguish original and normalized paths'}
@{pass=$true;checks=3;oldRefused=$fail;originalExit=$original;normalizedExit=$normalized;scope='actual CMake -P parser, no build'}|ConvertTo-Json|Tee-Object -FilePath "$Evidence/result.json"
