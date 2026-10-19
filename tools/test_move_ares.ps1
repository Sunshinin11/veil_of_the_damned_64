Add-Type -AssemblyName System.Drawing
$code = @"
using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Threading;
using System.Runtime.InteropServices;

public class TestMoveAres {
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

    [DllImport("user32.dll")]
    public static extern bool ShowWindow(IntPtr hWnd, int nCmdShow);
    [DllImport("user32.dll")]
    public static extern bool SetForegroundWindow(IntPtr hWnd);
    [DllImport("user32.dll")]
    public static extern bool BringWindowToTop(IntPtr hWnd);
    [DllImport("user32.dll")]
    public static extern bool SetWindowPos(IntPtr hWnd, IntPtr hWndInsertAfter, int X, int Y, int cx, int cy, uint uFlags);
    [StructLayout(LayoutKind.Sequential)]
    public struct RECT { public int Left, Top, Right, Bottom; }
    [DllImport("user32.dll")]
    public static extern bool GetClientRect(IntPtr hWnd, out RECT lpRect);
    [StructLayout(LayoutKind.Sequential)]
    public struct POINT { public int X; public int Y; }
    [DllImport("user32.dll")]
    public static extern bool ClientToScreen(IntPtr hWnd, ref POINT lpPoint);

    public static bool MoveAndCapture(IntPtr hWnd, int targetX, int targetY, string outFile) {
        IntPtr hDesk = OpenInputDesktop(0, false, 0x01FF);
        if (hDesk == IntPtr.Zero) return false;

        bool success = false;
        Thread t = new Thread(() => {
            if (!SetThreadDesktop(hDesk)) return;

            ShowWindow(hWnd, 9); // SW_RESTORE
            SetWindowPos(hWnd, new IntPtr(-1), targetX, targetY, 800, 600, 0x0040); // HWND_TOPMOST | SWP_SHOWWINDOW
            BringWindowToTop(hWnd);
            SetForegroundWindow(hWnd);
            Thread.Sleep(500);

            RECT rc;
            GetClientRect(hWnd, out rc);
            int w = rc.Right - rc.Left;
            int h = rc.Bottom - rc.Top;
            POINT pt = new POINT { X = 0, Y = 0 };
            ClientToScreen(hWnd, ref pt);

            Console.WriteLine("Client at: {0},{1} size: {2}x{3}", pt.X, pt.Y, w, h);

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
}
"@
Add-Type -TypeDefinition $code -ReferencedAssemblies 'System.Drawing'

$aresPath = (Resolve-Path "deps\ares\ares-v148\ares.exe").Path
$romPath = (Resolve-Path "veil_of_the_damned.z64").Path
$workingDir = (Resolve-Path "deps\ares\ares-v148").Path
Get-Process ares -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Milliseconds 400

$proc = Start-Process -FilePath $aresPath -WorkingDirectory $workingDir -ArgumentList "`"$romPath`"" -PassThru
Start-Sleep -Seconds 4
$proc.Refresh()
$hWnd = $proc.MainWindowHandle
Write-Host "hWnd: $hWnd"
$res = [TestMoveAres]::MoveAndCapture($hWnd, -1400, 100, "capturas\test_ares_on_mon1.png")
Write-Host "Captured: $res"
if (-not $proc.HasExited) { Stop-Process -Id $proc.Id -Force }
Get-Item "capturas\test_ares_on_mon1.png"
