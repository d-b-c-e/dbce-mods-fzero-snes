function Convert-FzeroCmakePath([string]$Path) {
    if(-not [IO.Path]::IsPathFullyQualified($Path) -or $Path -match '["\r\n]'){throw 'Absolute plain CMake path required'}
    $Path.Replace('\','/')
}
function Assert-FzeroCmakePath([string]$Path) {
    if($Path.Contains('\')){throw 'CMake generated-source path contains a backslash'}
}
