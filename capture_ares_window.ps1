Add-Type -AssemblyName System.Drawing
Add-Type -AssemblyName System.Windows.Forms

$code = @"
using System;
using System.Drawing;
using System.Runtime.InteropServices;

public class WinCap {
    [StructLayout(LayoutKind.Sequential)]
    public struct RECT {
        public int Left, Top, Right, Bottom;
    }

    [DllImport("user32.dll")]
    public static extern bool GetClientRect(IntPtr hWnd, out RECT lpRect);

    [DllImport("user32.dll")]
    public static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);

    [DllImport("user32.dll")]
    public static extern bool PrintWindow(IntPtr hWnd, IntPtr hdcBlt, uint nFlags);

    [DllImport("user32.dll")]
    public static extern bool PostMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);

    [DllImport("user32.dll")]
    public static extern bool SetForegroundWindow(IntPtr hWnd);

    public static Bitmap Capture(IntPtr hWnd) {
        RECT rc;
        GetClientRect(hWnd, out rc);
        int w = rc.Right - rc.Left;
        int h = rc.Bottom - rc.Top;
        if (w <= 0 || h <= 0) {
            GetWindowRect(hWnd, out rc);
            w = rc.Right - rc.Left;
            h = rc.Bottom - rc.Top;
        }
        if (w <= 0 || h <= 0) return null;

        Bitmap bmp = new Bitmap(w, h, System.Drawing.Imaging.PixelFormat.Format32bppArgb);
        using (Graphics g = Graphics.FromImage(bmp)) {
            IntPtr hdc = g.GetHdc();
            try {
                // PW_RENDERFULLCONTENT = 2
                PrintWindow(hWnd, hdc, 2);
            } finally {
                g.ReleaseHdc(hdc);
            }
        }
        return bmp;
    }
}
"@
Add-Type -TypeDefinition $code -ReferencedAssemblies 'System.Drawing', 'System.Windows.Forms'

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
    [WinCap]::SetForegroundWindow($hWnd)
    Start-Sleep -Milliseconds 200

    # Capture 1: Intro
    $bmp1 = [WinCap]::Capture($hWnd)
    if ($bmp1) {
        $out1 = Join-Path $captureDir "cap_01_intro.png"
        $bmp1.Save($out1)
        $bmp1.Dispose()
        Write-Host "Saved: $out1"
    }

    # START button
    [WinCap]::PostMessage($hWnd, 0x0100, [IntPtr]0x0D, [IntPtr]0)
    Start-Sleep -Milliseconds 80
    [WinCap]::PostMessage($hWnd, 0x0101, [IntPtr]0x0D, [IntPtr]0)
    Start-Sleep -Seconds 2

    # Capture 2: Gameplay idle
    $bmp2 = [WinCap]::Capture($hWnd)
    if ($bmp2) {
        $out2 = Join-Path $captureDir "cap_02_gameplay.png"
        $bmp2.Save($out2)
        $bmp2.Dispose()
        Write-Host "Saved: $out2"
    }

    # Walk right (direction test)
    [WinCap]::PostMessage($hWnd, 0x0100, [IntPtr]0x27, [IntPtr]0)
    Start-Sleep -Milliseconds 800
    $bmp3 = [WinCap]::Capture($hWnd)
    if ($bmp3) {
        $out3 = Join-Path $captureDir "cap_03_walk_right.png"
        $bmp3.Save($out3)
        $bmp3.Dispose()
        Write-Host "Saved: $out3"
    }
    [WinCap]::PostMessage($hWnd, 0x0101, [IntPtr]0x27, [IntPtr]0)
    Start-Sleep -Milliseconds 300

    # Walk left (direction test)
    [WinCap]::PostMessage($hWnd, 0x0100, [IntPtr]0x25, [IntPtr]0)
    Start-Sleep -Milliseconds 800
    $bmp4 = [WinCap]::Capture($hWnd)
    if ($bmp4) {
        $out4 = Join-Path $captureDir "cap_04_walk_left.png"
        $bmp4.Save($out4)
        $bmp4.Dispose()
        Write-Host "Saved: $out4"
    }
    [WinCap]::PostMessage($hWnd, 0x0101, [IntPtr]0x25, [IntPtr]0)
    Start-Sleep -Milliseconds 300

    # Attack (A)
    [WinCap]::PostMessage($hWnd, 0x0100, [IntPtr]0x20, [IntPtr]0)
    Start-Sleep -Milliseconds 150
    $bmp5 = [WinCap]::Capture($hWnd)
    if ($bmp5) {
        $out5 = Join-Path $captureDir "cap_05_attack.png"
        $bmp5.Save($out5)
        $bmp5.Dispose()
        Write-Host "Saved: $out5"
    }
    [WinCap]::PostMessage($hWnd, 0x0101, [IntPtr]0x20, [IntPtr]0)
    Start-Sleep -Milliseconds 500

    # Transform (C-Right / L)
    [WinCap]::PostMessage($hWnd, 0x0100, [IntPtr]0x4C, [IntPtr]0)
    Start-Sleep -Milliseconds 100
    [WinCap]::PostMessage($hWnd, 0x0101, [IntPtr]0x4C, [IntPtr]0)
    Start-Sleep -Seconds 1

    # Werewolf gameplay
    $bmp6 = [WinCap]::Capture($hWnd)
    if ($bmp6) {
        $out6 = Join-Path $captureDir "cap_06_werewolf.png"
        $bmp6.Save($out6)
        $bmp6.Dispose()
        Write-Host "Saved: $out6"
    }
}

Start-Sleep -Seconds 1
if (-not $proc.HasExited) {
    Stop-Process -Id $proc.Id -Force
}
Write-Host "Captura concluida com sucesso."
