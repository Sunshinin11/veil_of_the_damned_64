$aresPath = (Resolve-Path "deps\ares\ares-v148\ares.exe").Path
$romPath = (Resolve-Path "veil_of_the_damned.z64").Path
$workingDir = (Resolve-Path "deps\ares\ares-v148").Path
$outFile = (Join-Path (Get-Location) "ares_audio_log.txt")
$errFile = (Join-Path (Get-Location) "ares_audio_err.txt")

$code = @"
using System;
using System.Runtime.InteropServices;

public class Win32Audio {
    [DllImport("user32.dll")]
    public static extern bool PostMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);
    [DllImport("user32.dll")]
    public static extern bool SetForegroundWindow(IntPtr hWnd);
}
"@
Add-Type -TypeDefinition $code

function Send-Key($hWnd, $vk, $durationMs) {
    [Win32Audio]::PostMessage($hWnd, 0x0100, [IntPtr]$vk, [IntPtr]0)
    Start-Sleep -Milliseconds $durationMs
    [Win32Audio]::PostMessage($hWnd, 0x0101, [IntPtr]$vk, [IntPtr]0)
    Start-Sleep -Milliseconds 60
}

function Send-Combo($hWnd, [int[]]$vks, $durationMs) {
    foreach ($vk in $vks) {
        [Win32Audio]::PostMessage($hWnd, 0x0100, [IntPtr]$vk, [IntPtr]0)
    }
    Start-Sleep -Milliseconds $durationMs
    foreach ($vk in $vks) {
        [Win32Audio]::PostMessage($hWnd, 0x0101, [IntPtr]$vk, [IntPtr]0)
    }
    Start-Sleep -Milliseconds 60
}

Write-Host "Iniciando Ares com a ROM de 16.5 MB (4MB RAM base)..."
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
[Win32Audio]::SetForegroundWindow($hWnd)
Start-Sleep -Milliseconds 300

# 1. Ouvir Title Music por 3 segundos
Write-Host "1. Tela de título (bgm_title)..."
Start-Sleep -Seconds 3

# 2. Start para entrar no jogo
Write-Host "2. Pressionando START..."
Send-Key $hWnd 0x0D 100
Start-Sleep -Seconds 2

# 3. Teste das 8 direcoes do Druida (caminhada para a esquerda e direita puras e diagonais)
Write-Host "3. Testando 8 direcoes de caminhada do Druida..."
Write-Host "   - Esquerda pura (LEFT 0x25)..."
Send-Key $hWnd 0x25 900
Write-Host "   - Direita pura (RIGHT 0x27)..."
Send-Key $hWnd 0x27 900
Write-Host "   - Cima (UP 0x26)..."
Send-Key $hWnd 0x26 700
Write-Host "   - Baixo (DOWN 0x28)..."
Send-Key $hWnd 0x28 700
Write-Host "   - Diagonais..."
Send-Combo $hWnd @(0x26, 0x27) 600
Send-Combo $hWnd @(0x26, 0x25) 600
Send-Combo $hWnd @(0x28, 0x27) 600
Send-Combo $hWnd @(0x28, 0x25) 600

# 4. Teste de ataque basico A (todos os quadros e escala ajustada)
Write-Host "4. Testando golpe basico A (Druida)..."
for ($i = 0; $i -lt 4; $i++) {
    Send-Key $hWnd 0x20 120
    Start-Sleep -Milliseconds 350
}

# 5. Magias do Druida e SFX
Write-Host "5. Disparando Spirit Bolt (B)..."
Send-Key $hWnd 0x5A 100
Start-Sleep -Milliseconds 400
Send-Key $hWnd 0x5A 100
Start-Sleep -Milliseconds 400

Write-Host "6. Erupcao de Espinhos (C-Left / J)..."
Send-Key $hWnd 0x4A 100
Start-Sleep -Milliseconds 500

Write-Host "7. Invocando Lobos Espirituais (C-Up / I)..."
Send-Key $hWnd 0x49 100
Start-Sleep -Milliseconds 800

# 6. Esquiva (R)
Write-Host "8. Testando esquiva R..."
Send-Key $hWnd 0x52 100
Start-Sleep -Milliseconds 400

# 7. Transformacao em Lobisomem (C-Right / L)
Write-Host "9. Transformacao em Lobisomem..."
Send-Key $hWnd 0x4C 100
Start-Sleep -Seconds 1

Write-Host "10. Testando caminhada do Lobisomem (8 fases, esquerda e direita puras)..."
Send-Key $hWnd 0x25 800
Send-Key $hWnd 0x27 800
Send-Key $hWnd 0x26 700
Send-Key $hWnd 0x28 700

Write-Host "11. Ataque de garras do Lobisomem (A)..."
Send-Key $hWnd 0x20 120
Start-Sleep -Milliseconds 300
Send-Key $hWnd 0x20 120
Start-Sleep -Milliseconds 300

Write-Host "12. Esquiva do Lobisomem (R)..."
Send-Key $hWnd 0x52 100
Start-Sleep -Milliseconds 400

# 8. Reversao e combate na Area 0
Write-Host "13. Reversao para forma humana..."
Send-Key $hWnd 0x4C 100
Start-Sleep -Seconds 1

Write-Host "14. Combate na Area 0 para liberar portal..."
for ($i = 0; $i -lt 15; $i++) {
    Send-Key $hWnd 0x26 600
    Send-Key $hWnd 0x5A 80
    Send-Key $hWnd 0x20 100
    Start-Sleep -Milliseconds 150
}

# 9. Coletar drops e abrir bau
Write-Host "15. Movimentacao para coletar drops e abrir bau..."
Send-Key $hWnd 0x27 600
Send-Key $hWnd 0x26 800
Send-Key $hWnd 0x25 700

Start-Sleep -Seconds 3

if (-not $proc.HasExited) {
    Write-Host "Encerrando Ares emulator..."
    Stop-Process -Id $proc.Id -Force
}

Write-Host "=== TESTE DE AUDIO E JOGABILIDADE CONCLUIDO ==="
