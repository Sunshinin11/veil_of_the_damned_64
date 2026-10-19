Add-Type -AssemblyName System.Drawing
$code = @"
using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Threading;
using System.Runtime.InteropServices;

public class TestMinInThread {
    [DllImport("user32.dll")]
    public static extern bool ShowWindow(IntPtr hWnd, int nCmdShow);
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

    public static void MinAndCap(IntPtr hTarget, int x, int y, int w, int h, string outFile) {
        IntPtr hDesk = OpenInputDesktop(0, false, 0x01FF);
        Thread t = new Thread(() => {
            if (!SetThreadDesktop(hDesk)) return;

            bool minOk = ShowWindow(hTarget, 6); // SW_MINIMIZE
            Console.WriteLine("ShowWindow minimize: {0}", minOk);
            Thread.Sleep(600);

            IntPtr hdcSrc = GetDC(IntPtr.Zero);
            IntPtr hdcDest = CreateCompatibleDC(hdcSrc);
            IntPtr hBmp = CreateCompatibleBitmap(hdcSrc, w, h);
            IntPtr hOld = SelectObject(hdcDest, hBmp);

            if (BitBlt(hdcDest, 0, 0, w, h, hdcSrc, x, y, 0x00CC0020)) {
                using (Bitmap bmp = Image.FromHbitmap(hBmp)) {
                    bmp.Save(outFile, ImageFormat.Png);
                }
            }

            SelectObject(hdcDest, hOld);
            DeleteDC(hdcDest);
            ReleaseDC(IntPtr.Zero, hdcSrc);
            DeleteObject(hBmp);

            ShowWindow(hTarget, 9); // SW_RESTORE
        });
        t.Start();
        t.Join();
        if (hDesk != IntPtr.Zero) CloseDesktop(hDesk);
    }
}
"@
Add-Type -TypeDefinition $code -ReferencedAssemblies 'System.Drawing'
[TestMinInThread]::MinAndCap([IntPtr]132280, 552, 196, 768, 600, "capturas\test_min_in_thread.png")
Get-Item "capturas\test_min_in_thread.png"
