. "$PSScriptRoot\test_ares_compile_check.ps1"
$aresPath = (Resolve-Path "deps\ares\ares-v148\ares.exe").Path
$romPath = (Resolve-Path "veil_of_the_damned.z64").Path
$workingDir = (Resolve-Path "deps\ares\ares-v148").Path
Get-Process ares -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Milliseconds 400

$proc = Start-Process -FilePath $aresPath -WorkingDirectory $workingDir -ArgumentList "`"$romPath`"" -PassThru
Start-Sleep -Seconds 4
$hWnd = [AresCaptureSuite]::FindGameWindow([uint32]$proc.Id)
Write-Host "hWnd: $hWnd"
[AresCaptureSuite]::PrepareWindow($hWnd)
Start-Sleep -Milliseconds 600

$res = [AresCaptureSuite]::CaptureClient($hWnd, "capturas\test_topmost_intro.png")
Write-Host "Captured: $res"
if (-not $proc.HasExited) { Stop-Process -Id $proc.Id -Force }
Get-Item "capturas\test_topmost_intro.png"
