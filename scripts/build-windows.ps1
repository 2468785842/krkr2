Push-Location "$PSScriptRoot/.."
cmake --preset="Windows Debug Config" -DDISABLE_TEST=ON
cmake --build --preset="Windows Debug Build"
Pop-Location