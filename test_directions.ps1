$code = @"
using System;
using System.Runtime.InteropServices;

public class Win32 {
    [DllImport("user32.dll")]
    public static extern bool PostMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);

    [DllImport("user32.dll")]
    public static extern bool SetForegroundWindow(IntPtr hWnd);

    [DllImport("user32.dll")]
    public static extern void keybd_event(byte bVk, byte bScan, uint dwFlags, UIntPtr dwExtraInfo);
}
"@
Add-Type -TypeDefinition $code

$aresPath = (Resolve-Path "deps\ares\ares-v148\ares.exe").Path
$romPath = (Resolve-Path "veil_of_the_damned.z64").Path
$workingDir = (Resolve-Path "deps\ares\ares-v148").Path
$outFile = (Join-Path (Get-Location) "ares_perf_out.txt")
$errFile = (Join-Path (Get-Location) "ares_perf_err.txt")
$captureDir = Join-Path (Get-Location) "capturas"

if (-not (Test-Path $captureDir)) {
    New-Item -ItemType Directory -Path $captureDir | Out-Null
}

function Take-Screenshot($hWnd) {
    [Win32]::SetForegroundWindow($hWnd)
    [Win32]::keybd_event(0x70, 0, 0, [UIntPtr]::Zero) # VK_F1
    [Win32]::PostMessage($hWnd, 0x0100, [IntPtr]0x70, [IntPtr]0)
    Start-Sleep -Milliseconds 80
    [Win32]::keybd_event(0x70, 0, 2, [UIntPtr]::Zero)
    [Win32]::PostMessage($hWnd, 0x0101, [IntPtr]0x70, [IntPtr]0)
    Start-Sleep -Milliseconds 200
}

function Press-Keys($hWnd, [byte[]]$keys, $durationMs, [switch]$Screenshot) {
    [Win32]::SetForegroundWindow($hWnd)
    Start-Sleep -Milliseconds 50

    foreach ($k in $keys) {
        [Win32]::keybd_event($k, 0, 0, [UIntPtr]::Zero)
        [Win32]::PostMessage($hWnd, 0x0100, [IntPtr]$k, [IntPtr]0)
    }

    $sleepMs = [Math]::Max(50, $durationMs - 300)
    Start-Sleep -Milliseconds $sleepMs

    if ($Screenshot) {
        Take-Screenshot $hWnd
    }

    Start-Sleep -Milliseconds 200

    foreach ($k in $keys) {
        [Win32]::keybd_event($k, 0, 2, [UIntPtr]::Zero) # KEYEVENTF_KEYUP = 2
        [Win32]::PostMessage($hWnd, 0x0101, [IntPtr]$k, [IntPtr]0)
    }

    Start-Sleep -Milliseconds 400
}

Write-Host "Launching Ares emulator..."
$proc = Start-Process -FilePath $aresPath -WorkingDirectory $workingDir -ArgumentList "`"$romPath`"" -RedirectStandardOutput $outFile -RedirectStandardError $errFile -PassThru

Start-Sleep -Seconds 4
$proc.Refresh()
$hWnd = $proc.MainWindowHandle
if ($hWnd -eq [IntPtr]::Zero) {
    Start-Sleep -Seconds 2
    $proc.Refresh()
    $hWnd = $proc.MainWindowHandle
}

Write-Host "Window handle: $hWnd"
[Win32]::SetForegroundWindow($hWnd)
Start-Sleep -Milliseconds 300

# START button (VK_RETURN = 0x0D)
Write-Host "Sending START to enter gameplay..."
Press-Keys $hWnd @([byte]0x0D) 200
Start-Sleep -Seconds 2
Take-Screenshot $hWnd

Write-Host "1. UP (Cima)..."
Press-Keys $hWnd @([byte]0x26) 1200 -Screenshot

Write-Host "2. DOWN (Baixo)..."
Press-Keys $hWnd @([byte]0x28) 1200 -Screenshot

Write-Host "3. LEFT (Esquerda)..."
Press-Keys $hWnd @([byte]0x25) 1200 -Screenshot

Write-Host "4. RIGHT (Direita)..."
Press-Keys $hWnd @([byte]0x27) 1200 -Screenshot

Write-Host "5. UP-RIGHT (Cima-Direita)..."
Press-Keys $hWnd @([byte]0x26, [byte]0x27) 1200 -Screenshot

Write-Host "6. UP-LEFT (Cima-Esquerda)..."
Press-Keys $hWnd @([byte]0x26, [byte]0x25) 1200 -Screenshot

Write-Host "7. DOWN-RIGHT (Baixo-Direita)..."
Press-Keys $hWnd @([byte]0x28, [byte]0x27) 1200 -Screenshot

Write-Host "8. DOWN-LEFT (Baixo-Esquerda)..."
Press-Keys $hWnd @([byte]0x28, [byte]0x25) 1200 -Screenshot

Start-Sleep -Seconds 1

if (-not $proc.HasExited) {
    Write-Host "Stopping Ares emulator (PID: $($proc.Id))..."
    Stop-Process -Id $proc.Id -Force
}

Write-Host "=== TEST FINISHED ==="
