$ErrorActionPreference = 'Stop'
$project = Split-Path -Parent $MyInvocation.MyCommand.Path
$bundledPython = 'C:\Users\georg\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'

if ($env:SPRITE_PYTHON -and (Test-Path -LiteralPath $env:SPRITE_PYTHON)) {
    $spritePython = $env:SPRITE_PYTHON
} elseif (Test-Path -LiteralPath $bundledPython) {
    $spritePython = $bundledPython
} else {
    $spritePython = (Get-Command python -ErrorAction Stop).Source
}

Push-Location -LiteralPath $project
try {
    & $spritePython 'tools\build_sprite_assets.py'
    if ($LASTEXITCODE -ne 0) { throw 'Falha na conversão dos sprites.' }
    & $spritePython 'tools\build_hawk_assets.py'
    if ($LASTEXITCODE -ne 0) { throw 'Falha na conversão do gavião.' }
    & $spritePython 'tools\densify_anims.py' hunt_idle hunt_run hunt_attack hunt_hurt hunt_death hunt_roll barb_idle barb_run barb_attack barb_attack2 barb_spin barb_shout barb_hurt barb_death barb_roll
    if ($LASTEXITCODE -ne 0) { throw 'Falha nos 12 quadros da Caçadora.' }
    & $spritePython 'tools\gen_sprite_norm.py'
    if ($LASTEXITCODE -ne 0) { throw 'Falha na escala dos sprites.' }
    & $spritePython 'tools\build_scenery_assets.py'
    if ($LASTEXITCODE -ne 0) { throw 'Falha na conversão do cenário.' }
    & $spritePython 'tools\prepare_audio_assets.py'
    if ($LASTEXITCODE -ne 0) { throw 'Falha no preparo do áudio.' }
    & wsl bash -lc 'cd "/mnt/c/Desenvolvimento/Nintendo 64/Veil of the Damned 64" && ./build.sh -j4'
    if ($LASTEXITCODE -ne 0) { throw 'Falha na compilação da ROM.' }
    Write-Host 'ROM criada: veil_of_the_damned.z64'
} finally {
    Pop-Location
}
