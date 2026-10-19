$aresPath = (Resolve-Path "deps\ares\ares-v148\ares.exe").Path
$romPath = (Resolve-Path "veil_of_the_damned.z64").Path
$workingDir = (Resolve-Path "deps\ares\ares-v148").Path
$outFile = (Join-Path (Get-Location) "ares_perf_out.txt")
$errFile = (Join-Path (Get-Location) "ares_perf_err.txt")

$code = @"
using System;
using System.Runtime.InteropServices;

public class Win32 {
    [DllImport("user32.dll")]
    public static extern bool PostMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);
}
"@
Add-Type -TypeDefinition $code

Write-Host "Launching Ares emulator..."
$proc = Start-Process -FilePath $aresPath -WorkingDirectory $workingDir -ArgumentList "`"$romPath`"" -RedirectStandardOutput $outFile -RedirectStandardError $errFile -PassThru

Start-Sleep -Seconds 4
$proc.Refresh()
$hWnd = $proc.MainWindowHandle
if ($hWnd -ne [IntPtr]::Zero) {
    Write-Host "Sending START (Return)..."
    [Win32]::PostMessage($hWnd, 0x0100, [IntPtr]0x0D, [IntPtr]0) # VK_RETURN
    Start-Sleep -Milliseconds 80
    [Win32]::PostMessage($hWnd, 0x0101, [IntPtr]0x0D, [IntPtr]0)

    Start-Sleep -Seconds 2

    Write-Host "Walking in directions (VK_UP)..."
    [Win32]::PostMessage($hWnd, 0x0100, [IntPtr]0x26, [IntPtr]0) # VK_UP
    Start-Sleep -Seconds 2
    [Win32]::PostMessage($hWnd, 0x0101, [IntPtr]0x26, [IntPtr]0)

    Write-Host "Walking right (VK_RIGHT)..."
    [Win32]::PostMessage($hWnd, 0x0100, [IntPtr]0x27, [IntPtr]0) # VK_RIGHT
    Start-Sleep -Seconds 2
    [Win32]::PostMessage($hWnd, 0x0101, [IntPtr]0x27, [IntPtr]0)

    Write-Host "Invocando lobos espirituais (C-Up / I)..."
    [Win32]::PostMessage($hWnd, 0x0100, [IntPtr]0x49, [IntPtr]0) # VK_I
    Start-Sleep -Milliseconds 100
    [Win32]::PostMessage($hWnd, 0x0101, [IntPtr]0x49, [IntPtr]0)

    Start-Sleep -Seconds 3

    Write-Host "Transformando em Fera/Werewolf (C-Right / L)..."
    [Win32]::PostMessage($hWnd, 0x0100, [IntPtr]0x4C, [IntPtr]0) # VK_L
    Start-Sleep -Milliseconds 100
    [Win32]::PostMessage($hWnd, 0x0101, [IntPtr]0x4C, [IntPtr]0)

    Start-Sleep -Seconds 2

    Write-Host "Andando como Lobisomem (VK_UP)..."
    [Win32]::PostMessage($hWnd, 0x0100, [IntPtr]0x26, [IntPtr]0) # VK_UP
    Start-Sleep -Seconds 2
    [Win32]::PostMessage($hWnd, 0x0101, [IntPtr]0x26, [IntPtr]0)

    Write-Host "Atacando como Lobisomem (VK_SPACE / A)..."
    [Win32]::PostMessage($hWnd, 0x0100, [IntPtr]0x20, [IntPtr]0) # VK_SPACE
    Start-Sleep -Milliseconds 100
    [Win32]::PostMessage($hWnd, 0x0101, [IntPtr]0x20, [IntPtr]0)
}

Start-Sleep -Seconds 4

if (-not $proc.HasExited) {
    Write-Host "Stopping Ares emulator (PID: $($proc.Id))..."
    Stop-Process -Id $proc.Id -Force
}

Write-Host "=== ARES OUTPUT ==="
if (Test-Path $outFile) {
    Get-Content $outFile
}
Write-Host "=== ARES ERRORS ==="
if (Test-Path $errFile) {
    Get-Content $errFile
}
