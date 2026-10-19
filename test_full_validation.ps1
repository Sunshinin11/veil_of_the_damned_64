$aresPath = (Resolve-Path "deps\ares\ares-v148\ares.exe").Path
$romPath = (Resolve-Path "veil_of_the_damned.z64").Path
$workingDir = (Resolve-Path "deps\ares\ares-v148").Path
$outFile = (Join-Path (Get-Location) "ares_full_log.txt")
$errFile = (Join-Path (Get-Location) "ares_full_err.txt")

$code = @"
using System;
using System.Runtime.InteropServices;

public class Win32 {
    [DllImport("user32.dll")]
    public static extern bool PostMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);
    [DllImport("user32.dll")]
    public static extern bool SetForegroundWindow(IntPtr hWnd);
}
"@
Add-Type -TypeDefinition $code

function Send-Key($hWnd, $vk, $durationMs) {
    [Win32]::PostMessage($hWnd, 0x0100, [IntPtr]$vk, [IntPtr]0)
    Start-Sleep -Milliseconds $durationMs
    [Win32]::PostMessage($hWnd, 0x0101, [IntPtr]$vk, [IntPtr]0)
    Start-Sleep -Milliseconds 80
}

function Send-Combo($hWnd, [int[]]$vks, $durationMs) {
    foreach ($vk in $vks) {
        [Win32]::PostMessage($hWnd, 0x0100, [IntPtr]$vk, [IntPtr]0)
    }
    Start-Sleep -Milliseconds $durationMs
    foreach ($vk in $vks) {
        [Win32]::PostMessage($hWnd, 0x0101, [IntPtr]$vk, [IntPtr]0)
    }
    Start-Sleep -Milliseconds 80
}

Write-Host "Iniciando Ares com veil_of_the_damned.z64 (4MB RAM base)..."
$proc = Start-Process -FilePath $aresPath -WorkingDirectory $workingDir -ArgumentList "`"$romPath`"" -RedirectStandardOutput $outFile -RedirectStandardError $errFile -PassThru

Start-Sleep -Seconds 4
$proc.Refresh()
$hWnd = $proc.MainWindowHandle
if ($hWnd -eq [IntPtr]::Zero) {
    Start-Sleep -Seconds 2
    $proc.Refresh()
    $hWnd = $proc.MainWindowHandle
}

Write-Host "Handle da janela Ares: $hWnd"
[Win32]::SetForegroundWindow($hWnd)
Start-Sleep -Milliseconds 300

# 1. Start para sair do Intro
Write-Host "1. Pressionando START..."
Send-Key $hWnd 0x0D 100
Start-Sleep -Seconds 2

# 2. Testando 8 direcoes de caminhada do Druida
Write-Host "2. Testando 8 direcoes do Druida..."
Write-Host "   - Cima (UP)..."
Send-Key $hWnd 0x26 700
Write-Host "   - Baixo (DOWN)..."
Send-Key $hWnd 0x28 700
Write-Host "   - Esquerda (LEFT)..."
Send-Key $hWnd 0x25 700
Write-Host "   - Direita (RIGHT)..."
Send-Key $hWnd 0x27 700
Write-Host "   - Cima-Direita..."
Send-Combo $hWnd @(0x26, 0x27) 600
Write-Host "   - Cima-Esquerda..."
Send-Combo $hWnd @(0x26, 0x25) 600
Write-Host "   - Baixo-Direita..."
Send-Combo $hWnd @(0x28, 0x27) 600
Write-Host "   - Baixo-Esquerda..."
Send-Combo $hWnd @(0x28, 0x25) 600

# 3. Testando Esquiva (R)
Write-Host "3. Testando esquiva com R..."
Send-Key $hWnd 0x52 100
Start-Sleep -Milliseconds 500

# 4. Testando Ataque Basico (A - Golpe)
Write-Host "4. Testando ataque basico A..."
Send-Key $hWnd 0x20 150
Start-Sleep -Milliseconds 400
Send-Key $hWnd 0x20 150
Start-Sleep -Milliseconds 400

# 5. Testando Magias do Druida
Write-Host "5. Testando Spirit Bolt (B)..."
Send-Key $hWnd 0x5A 100
Start-Sleep -Milliseconds 500
Send-Key $hWnd 0x5A 100
Start-Sleep -Milliseconds 500

Write-Host "6. Testando Thorn Burst (C-Left / J)..."
Send-Key $hWnd 0x4A 100
Start-Sleep -Milliseconds 600

Write-Host "7. Testando Spirit Wolves (C-Up / I)..."
Send-Key $hWnd 0x49 100
Start-Sleep -Milliseconds 800

# 6. Testando Transformacao em Lobisomem (C-Right / L)
Write-Host "8. Testando Transformacao em Fera (C-Right / L)..."
Send-Key $hWnd 0x4C 100
Start-Sleep -Seconds 1

Write-Host "9. Testando caminhada do Lobisomem..."
Send-Key $hWnd 0x26 700
Send-Key $hWnd 0x27 700
Send-Key $hWnd 0x28 700
Send-Key $hWnd 0x25 700

Write-Host "10. Testando ataque basico A como Lobisomem (Garras)..."
Send-Key $hWnd 0x20 150
Start-Sleep -Milliseconds 400
Send-Key $hWnd 0x20 150
Start-Sleep -Milliseconds 400

Write-Host "11. Testando esquiva R como Lobisomem..."
Send-Key $hWnd 0x52 100
Start-Sleep -Milliseconds 500

Write-Host "12. Voltando a forma humana..."
Send-Key $hWnd 0x4C 100
Start-Sleep -Seconds 1

# 7. Combate e avanco no cenario (andando e atacando inimigos)
Write-Host "13. Avanco e combate na Area 0..."
for ($i = 0; $i -lt 12; $i++) {
    Send-Key $hWnd 0x26 600
    Send-Key $hWnd 0x5A 80 # Spirit bolt
    Send-Key $hWnd 0x20 100 # Melee
    Start-Sleep -Milliseconds 200
}

Start-Sleep -Seconds 2

if (-not $proc.HasExited) {
    Write-Host "Encerrando Ares emulator..."
    Stop-Process -Id $proc.Id -Force
}

Write-Host "=== TESTE COMPLETO CONCLUIDO ==="
