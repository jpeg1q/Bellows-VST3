$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $PSScriptRoot
$Build = Join-Path $Root "build"

cmake -S $Root -B $Build -A x64 -DBELLOWS_BUILD_PLUGIN=ON -DBELLOWS_BUILD_TESTS=ON -DBELLOWS_BUILD_DEMO=ON
cmake --build $Build --config Release --parallel
ctest --test-dir $Build -C Release --output-on-failure

$Vst3 = Join-Path $Build "Bellows_artefacts\Release\VST3\Bellows.vst3"
Write-Host ""
Write-Host "Build complete. VST3 bundle: $Vst3"
Write-Host "Copy it to: C:\Program Files\Common Files\VST3\"
