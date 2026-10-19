Add-Type -AssemblyName System.Drawing
$code = @"
using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Threading;
using System.Runtime.InteropServices;

public class TestAresClean {
    public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);
    [DllImport("user32.dll")]
    public static extern bool EnumWindows(EnumWindowsProc enumProc, IntPtr lParam);
    [DllImport("user32.dll")]
    public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint lpdwProcessId);
    [DllImport("user32.dll")]
    public static extern bool IsWindowVisible(IntPtr hWnd);
    [DllImport("user32.dll", SetLastError = true)]
    public static extern IntPtr FindWindow(string lpClassName, string lpWindowName);

    [DllImport("user32.dll")]
    public static extern bool ShowWindow(IntPtr hWnd, int nCmdShow);
    [DllImport("user32.dll")]
    public static extern bool SetForegroundWindow(IntPtr hWnd);
    [DllImport("user32.dll")]
    public static extern bool BringWindowToTop(IntPtr hWnd);

    [StructLayout(LayoutKind.Sequential)]
    public struct RECT { public int Left, Top, Right, Bottom; }
    [DllImport("user32.dll")]
    public static extern bool GetClientRect(IntPtr hWnd, out RECT lpRect);
    [StructLayout(LayoutKind.Sequential)]
    public struct POINT { public int X; public int Y; }
    [DllImport("user32.dll")]
    public static extern bool ClientToScreen(IntPtr hWnd, ref POINT lpPoint);

    [DllImport("user32.dll", SetLastError = true)]
    public static extern IntPtr OpenInputDesktop(uint dwFlags, bool fInherit, uint dwDesiredAccess);
    [DllImport("user32.dll", SetLastError = true)]
    public static extern bool SetThreadDesktop(IntPtr hDesktop);
    [DllImport("user32.dll", SetLastError = true)]
    public static extern bool CloseDesktop(IntPtr hDesktop);

    [DllImport("user32.dll")]
    public static extern IntPtr GetDC(IntPtr hWnd);
    [DllImport("user32.dll")]
    public static extern int ReleaseDC(IntPtr hWnd, IntPtr hDC);
    [DllImport("gdi32.dll")]
    public static extern IntPtr CreateCompatibleDC(IntPtr hdc);
    [DllImport("gdi32.dll")]
    public static extern IntPtr CreateCompatibleBitmap(IntPtr hdc, int nWidth, int nHeight);
    [DllImport("gdi32.dll")]
    public static extern IntPtr SelectObject(IntPtr hdc, IntPtr hgdiobj);
    [DllImport("gdi32.dll")]
    public static extern bool DeleteDC(IntPtr hdc);
    [DllImport("gdi32.dll")]
    public static extern bool DeleteObject(IntPtr hObject);
    [DllImport("gdi32.dll", SetLastError = true)]
    public static extern bool BitBlt(IntPtr hdcDest, int nXDest, int nYDest, int nWidth, int nHeight, IntPtr hdcSrc, int nXSrc, int nYSrc, uint dwRop);

    public static bool CaptureWindowDirect(IntPtr hWnd, string outFile) {
        IntPtr hDesk = OpenInputDesktop(0, false, 0x01FF);
        if (hDesk == IntPtr.Zero) return false;

        bool success = false;
        Thread t = new Thread(() => {
            if (!SetThreadDesktop(hDesk)) return;

            ShowWindow(hWnd, 9); // SW_RESTORE
            BringWindowToTop(hWnd);
            SetForegroundWindow(hWnd);
            Thread.Sleep(300);

            RECT rc;
            GetClientRect(hWnd, out rc);
            int w = rc.Right - rc.Left;
            int h = rc.Bottom - rc.Top;
            POINT pt = new POINT { X = 0, Y = 0 };
            ClientToScreen(hWnd, ref pt);

            IntPtr hdcSrc = GetDC(IntPtr.Zero);
            IntPtr hdcDest = CreateCompatibleDC(hdcSrc);
            IntPtr hBmp = CreateCompatibleBitmap(hdcSrc, w, h);
            IntPtr hOld = SelectObject(hdcDest, hBmp);

            if (BitBlt(hdcDest, 0, 0, w, h, hdcSrc, pt.X, pt.Y, 0x00CC0020)) {
                using (Bitmap bmp = Image.FromHbitmap(hBmp)) {
                    bmp.Save(outFile, ImageFormat.Png);
                    success = true;
                }
            }

            SelectObject(hdcDest, hOld);
            DeleteDC(hdcDest);
            ReleaseDC(IntPtr.Zero, hdcSrc);
            DeleteObject(hBmp);
        });
        t.Start();
        t.Join();
        CloseDesktop(hDesk);
        return success;
    }

    public static void MinimizeApps(IntPtr[] wnds) {
        IntPtr hDesk = OpenInputDesktop(0, false, 0x01FF);
        Thread t = new Thread(() => {
            if (!SetThreadDesktop(hDesk)) return;
            foreach (var h in wnds) {
                if (h != IntPtr.Zero) ShowWindow(h, 6); // SW_MINIMIZE
            }
        });
        t.Start();
        t.Join();
        if (hDesk != IntPtr.Zero) CloseDesktop(hDesk);
    }

    public static void RestoreApps(IntPtr[] wnds) {
        IntPtr hDesk = OpenInputDesktop(0, false, 0x01FF);
        Thread t = new Thread(() => {
            if (!SetThreadDesktop(hDesk)) return;
            foreach (var h in wnds) {
                if (h != IntPtr.Zero) ShowWindow(h, 9); // SW_RESTORE
            }
        });
        t.Start();
        t.Join();
        if (hDesk != IntPtr.Zero) CloseDesktop(hDesk);
    }
}
"@
Add-Type -TypeDefinition $code -ReferencedAssemblies 'System.Drawing'

# Chrome and Claude window handles
$apps = @([IntPtr]132280, [IntPtr]66902)
[TestAresClean]::MinimizeApps($apps)
Start-Sleep -Milliseconds 400

$aresPath = (Resolve-Path "deps\ares\ares-v148\ares.exe").Path
$romPath = (Resolve-Path "veil_of_the_damned.z64").Path
$workingDir = (Resolve-Path "deps\ares\ares-v148").Path
Get-Process ares -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Milliseconds 400

$proc = Start-Process -FilePath $aresPath -WorkingDirectory $workingDir -ArgumentList "`"$romPath`"" -PassThru
Start-Sleep -Seconds 4

$hWnd = [TestAresClean]::FindWindow($null, "veil_of_the_damned")
Write-Host "Game window: $hWnd"

$res = [TestAresClean]::CaptureWindowDirect($hWnd, "capturas\ares_intro_clean.png")
Write-Host "Captured: $res"
if (-not $proc.HasExited) { Stop-Process -Id $proc.Id -Force }

# Restore apps
[TestAresClean]::RestoreApps($apps)
Get-Item "capturas\ares_intro_clean.png"
