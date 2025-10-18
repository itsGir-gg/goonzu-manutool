#include <dwmapi.h>
#pragma comment(lib, "Dwmapi.lib")

static bool   g_DarkMode = false;
static HBRUSH g_hbrDark = nullptr;

// Try to tell Windows we prefer dark title bar/chrome (Win10 1903+/Win11)
static void ApplyDarkModeAttribute(HWND hWnd, bool enable)
{
    // DWMWA_USE_IMMERSIVE_DARK_MODE is 20 on newer builds (or 19 on older 1903+)
    const DWORD DWMWA_USE_IMMERSIVE_DARK_MODE = 20;
    BOOL val = enable ? TRUE : FALSE;
    // Try 20, then 19 for broader coverage
    if (FAILED(DwmSetWindowAttribute(hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &val, sizeof(val)))) {
        const DWORD DWMWA_USE_IMMERSIVE_DARK_MODE_OLD = 19;
        DwmSetWindowAttribute(hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE_OLD, &val, sizeof(val));
    }
}