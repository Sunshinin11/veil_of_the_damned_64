Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing

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

$aresPath = (Resolve-Path "deps\ares\ares-v148\ares.exe").Path
$romPath = (Resolve-Path "veil_of_the_damned.z64").Path
$workingDir = (Resolve-Path "deps\ares\ares-v148").Path
$captureDir = Join-Path (Get-Location) "capturas"

if (-not (Test-Path $captureDir)) {
    New-Item -ItemType Directory -Path $captureDir | Out-Null
}

$proc = Start-Process -FilePath $aresPath -WorkingDirectory $workingDir -ArgumentList "`"$romPath`"" -PassThru
Start-Sleep -Seconds 4
$proc.Refresh()
$hWnd = $proc.MainWindowHandle

if ($hWnd -ne [IntPtr]::Zero) {
    [Win32]::SetForegroundWindow($hWnd)
    Start-Sleep -Milliseconds 200
    # START button
    [Win32]::PostMessage($hWnd, 0x0100, [IntPtr]0x0D, [IntPtr]0)
    Start-Sleep -Milliseconds 80
    [Win32]::PostMessage($hWnd, 0x0101, [IntPtr]0x0D, [IntPtr]0)
    Start-Sleep -Seconds 3

    # Walk upwards
    [Win32]::PostMessage($hWnd, 0x0100, [IntPtr]0x26, [IntPtr]0)
    Start-Sleep -Seconds 1
    [Win32]::PostMessage($hWnd, 0x0101, [IntPtr]0x26, [IntPtr]0)
    Start-Sleep -Milliseconds 500

    $screen = [System.Windows.Forms.Screen]::PrimaryScreen.Bounds
    $bmp = New-Object System.Drawing.Bitmap $screen.Width, $screen.Height
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.CopyFromScreen($screen.Location, [System.Drawing.Point]::Empty, $screen.Size)
    $outPath = Join-Path $captureDir "screen_gameplay.png"
    $bmp.Save($outPath, [System.Drawing.Imaging.ImageFormat]::Png)
    $g.Dispose()
    $bmp.Dispose()
    Write-Host "Screenshot saved to $outPath"
}

Start-Sleep -Seconds 1
if (-not $proc.HasExited) {
    Stop-Process -Id $proc.Id -Force
}
