Add-Type -AssemblyName System.Drawing
Add-Type -AssemblyName System.Windows.Forms

$code = @"
using System;
using System.Text;
using System.Drawing;
using System.Drawing.Imaging;
using System.Threading;
using System.Runtime.InteropServices;

public class WinInspectorDesktop {
    public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);
    [DllImport("user32.dll")]
    public static extern bool EnumWindows(EnumWindowsProc enumProc, IntPtr lParam);
    [DllImport("user32.dll")]
    public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint lpdwProcessId);
    [DllImport("user32.dll")]
    public static extern bool IsWindowVisible(IntPtr hWnd);
    [DllImport("user32.dll", CharSet = CharSet.Auto)]
    public static extern int GetWindowText(IntPtr hWnd, StringBuilder lpString, int nMaxCount);
    [StructLayout(LayoutKind.Sequential)]
    public struct RECT { public int Left, Top, Right, Bottom; }
    [DllImport("user32.dll")]
    public static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);

    [DllImport("user32.dll", SetLastError = true)]
    public static extern IntPtr OpenInputDesktop(uint dwFlags, bool fInherit, uint dwDesiredAccess);
    [DllImport("user32.dll", SetLastError = true)]
    public static extern bool SetThreadDesktop(IntPtr hDesktop);
    [DllImport("user32.dll", SetLastError = true)]
    public static extern bool CloseDesktop(IntPtr hDesktop);

    public static void ListWindows() {
        IntPtr hDesk = OpenInputDesktop(0, false, 0x01FF);
        if (hDesk == IntPtr.Zero) {
            Console.WriteLine("Failed to open input desktop");
            return;
        }

        Thread t = new Thread(() => {
            if (!SetThreadDesktop(hDesk)) {
                Console.WriteLine("Failed to set thread desktop");
                return;
            }

            EnumWindows((hWnd, lParam) => {
                if (IsWindowVisible(hWnd)) {
                    RECT r;
                    GetWindowRect(hWnd, out r);
                    if (r.Right - r.Left > 100 && r.Bottom - r.Top > 100) {
                        StringBuilder sb = new StringBuilder(256);
                        GetWindowText(hWnd, sb, 256);
                        uint pid;
                        GetWindowThreadProcessId(hWnd, out pid);
                        string title = sb.ToString();
                        if (!string.IsNullOrEmpty(title)) {
                            Console.WriteLine("PID={0}, hWnd={1}, Rect=[{2},{3} to {4},{5}], Title='{6}'",
                                pid, hWnd, r.Left, r.Top, r.Right, r.Bottom, title);
                        }
                    }
                }
                return true;
            }, IntPtr.Zero);
        });
        t.Start();
        t.Join();
        CloseDesktop(hDesk);
    }
}
"@
Add-Type -TypeDefinition $code -ReferencedAssemblies 'System.Drawing', 'System.Windows.Forms'
[WinInspectorDesktop]::ListWindows()
