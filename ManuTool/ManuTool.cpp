// ManuTool.cpp : main source for the application.
//

// Prevent Windows headers from defining min/max macros
#define NOMINMAX

#include "framework.h"
#include "ManuTool.h"
#include "resource.h"

#include <dwmapi.h>
#include <algorithm>
#include <string>
#include <cwchar>   // wcslen, wcscpy_s, wsprintfW
#include <cstdlib>  // malloc, free
#include "CraftingPanel.h"
#include "CenterPanel.h"

#pragma comment(lib, "Dwmapi.lib")

// App-private message used to defer qty changes from the EDIT
#define WM_APP_QTY_CHANGED (WM_APP + 42)

// ---------------- Template globals ----------------
HINSTANCE hInst;                  // current instance
WCHAR szTitle[100];               // title bar text
WCHAR szWindowClass[100];         // main window class

// Fwds
ATOM                MyRegisterClass(HINSTANCE);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

// ---------------- Dark-mode state & helpers ----------------
static bool   g_DarkMode = false;
static HBRUSH g_hbrDark = nullptr;                  // client area dark brush
static HBRUSH g_hbrMenuDark = nullptr;              // popup dark bg
static HFONT  g_hMenuFont = nullptr;                // menu font

static COLORREF kMenuBgDark = RGB(32, 32, 32);
static COLORREF kMenuBgHoverDark = RGB(48, 48, 48);
static COLORREF kMenuTextDark = RGB(240, 240, 240);

// Ask DWM for dark title bar (Win10 1903+/Win11)
static void ApplyDarkModeAttribute(HWND hWnd, bool enable)
{
    const DWORD DWMWA_USE_IMMERSIVE_DARK_MODE = 20;
    BOOL val = enable ? TRUE : FALSE;
    if (FAILED(DwmSetWindowAttribute(hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &val, sizeof(val))))
    {
        const DWORD DWMWA_USE_IMMERSIVE_DARK_MODE_OLD = 19;
        DwmSetWindowAttribute(hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE_OLD, &val, sizeof(val));
    }
}

// Keyboard-cues setting: when false, hide underlines until Alt is pressed
static UINT MenuPrefixDrawFlags()
{
    BOOL show = FALSE;
    SystemParametersInfoW(SPI_GETKEYBOARDCUES, 0, &show, 0);
    return show ? 0u : 0x00100000u /* DT_HIDEPREFIX */;
}

// Duplicate a wide string (malloc'd)
static wchar_t* DupWide(const wchar_t* s)
{
    if (!s) return nullptr;
    const size_t n = wcslen(s) + 1;
    wchar_t* p = static_cast<wchar_t*>(std::malloc(n * sizeof(wchar_t)));
    if (p) wcscpy_s(p, n, s);
    return p;
}

// Cached per-item data for owner-draw
struct MenuItemCache {
    wchar_t* text;     // caption, including '&' mnemonic
    BOOL     topLevel; // true if on the menu bar (not in popup)
};

// Free cached data in a menu tree
static void FreeMenuItemData(HMENU hMenu)
{
    if (!hMenu) return;
    const UINT count = GetMenuItemCount(hMenu);
    for (UINT i = 0; i < count; ++i) {
        MENUITEMINFOW mii{};
        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_DATA | MIIM_SUBMENU | MIIM_FTYPE;
        if (!GetMenuItemInfoW(hMenu, i, TRUE, &mii)) continue;

        if ((mii.fType & MFT_OWNERDRAW) && mii.dwItemData) {
            auto* cache = reinterpret_cast<MenuItemCache*>(mii.dwItemData);
            if (cache) {
                if (cache->text) std::free(cache->text);
                std::free(cache);
            }
            mii.dwItemData = 0;
            SetMenuItemInfoW(hMenu, i, TRUE, &mii);
        }
    }
}

// Apply popup background brush (owner-draw handles bar painting)
static void ApplyMenuBackground(HMENU hMenu, HBRUSH hbr)
{
    if (!hMenu) return;
    MENUINFO mi{};
    mi.cbSize = sizeof(mi);
    mi.fMask = MIM_BACKGROUND;
    mi.hbrBack = hbr; // NULL resets to default
    SetMenuInfo(hMenu, &mi);

    const UINT count = GetMenuItemCount(hMenu);
    for (UINT i = 0; i < count; ++i) {
        HMENU sub = GetSubMenu(hMenu, i);
        if (sub) ApplyMenuBackground(sub, hbr);
    }
}

// Create a font that matches system menu metrics (accurate sizing)
static HFONT CreateMenuFont()
{
    NONCLIENTMETRICSW ncm{};
    ncm.cbSize = sizeof(ncm);
    if (!SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0))
        return (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    return CreateFontIndirectW(&ncm.lfMenuFont);
}

// Owner-draw on/off recursively. We cache text + whether item is top-level.
static void SetMenuOwnerDrawRecursive(HMENU hMenu, bool ownerDraw, BOOL isTopLevel)
{
    if (!hMenu) return;
    const UINT count = GetMenuItemCount(hMenu);

    for (UINT i = 0; i < count; ++i) {
        MENUITEMINFOW mii{};
        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_FTYPE | MIIM_SUBMENU | MIIM_DATA;
        if (!GetMenuItemInfoW(hMenu, i, TRUE, &mii)) continue;

        if (ownerDraw) {
            // Get text by position (works for bar & popups)
            wchar_t text[256] = L"";
            GetMenuStringW(hMenu, i, text, _countof(text), MF_BYPOSITION);

            // Replace any existing cache
            if (mii.dwItemData) {
                auto* old = reinterpret_cast<MenuItemCache*>(mii.dwItemData);
                if (old) { std::free(old->text); std::free(old); }
                mii.dwItemData = 0;
            }
            auto* cache = (MenuItemCache*)std::malloc(sizeof(MenuItemCache));
            cache->text = DupWide(text);
            cache->topLevel = isTopLevel ? TRUE : FALSE;
            mii.dwItemData = reinterpret_cast<ULONG_PTR>(cache);

            mii.fType |= MFT_OWNERDRAW;

            // IMPORTANT: do NOT include MIIM_STRING, or captions get clobbered
            mii.fMask = MIIM_FTYPE | MIIM_DATA | MIIM_SUBMENU;
            SetMenuItemInfoW(hMenu, i, TRUE, &mii);
        }
        else {
            // Turning owner-draw OFF:
            // If owner-draw was set, free any cached data and clear the owner-draw bit
            if ((mii.fType & MFT_OWNERDRAW) || mii.dwItemData) {
                if (mii.dwItemData) {
                    auto* cache = reinterpret_cast<MenuItemCache*>(mii.dwItemData);
                    if (cache) {
                        if (cache->text) std::free(cache->text);
                        std::free(cache);
                    }
                    mii.dwItemData = 0;
                }
                // Clear the owner-draw flag so the system draws the item text again.
                mii.fType &= ~MFT_OWNERDRAW;
                mii.fMask = MIIM_FTYPE | MIIM_DATA | MIIM_SUBMENU;
                SetMenuItemInfoW(hMenu, i, TRUE, &mii);
            }
        }

        if (mii.hSubMenu) SetMenuOwnerDrawRecursive(mii.hSubMenu, ownerDraw, FALSE);
    }
}

static void SetMenuOwnerDraw(HMENU hMenu, bool ownerDraw)
{
    SetMenuOwnerDrawRecursive(hMenu, ownerDraw, TRUE);
}

// ---------------- App entry/registration/init ----------------
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE,
    _In_ LPWSTR,
    _In_ int nCmdShow)
{
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, _countof(szTitle));
    LoadStringW(hInstance, IDC_MANUTOOL, szWindowClass, _countof(szWindowClass));
    MyRegisterClass(hInstance);

    if (!InitInstance(hInstance, nCmdShow))
        return FALSE;

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_MANUTOOL));

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return (int)msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex{};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MANUTOOL));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = GetSysColorBrush(COLOR_WINDOW); // safe stock brush
    wcex.lpszMenuName = nullptr;                        // attach in InitInstance
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance;

    // Register the common control classes we need before creating any controls.
    // This ensures TreeView / UpDown / ListView classes are available.
    INITCOMMONCONTROLSEX icc{};
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_TREEVIEW_CLASSES | ICC_STANDARD_CLASSES | ICC_UPDOWN_CLASS | ICC_LISTVIEW_CLASSES;
    BOOL iccRes = InitCommonControlsEx(&icc);
    {
        wchar_t dbg[128];
        wsprintfW(dbg, L"InitCommonControlsEx() returned %d\n", iccRes ? 1 : 0);
        OutputDebugStringW(dbg);
    }

    HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT, 0, 960, 600, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd)
        return FALSE;

    // Attach main menu...
    HMENU hMenu = LoadMenuW(hInstance, MAKEINTRESOURCEW(IDC_MANUTOOL));
    SetMenu(hWnd, hMenu);

    // Create the panels
    gCraft.create(hWnd, hInstance);
    RecipesDB::RegisterAll();

    RECT rc{}; GetClientRect(hWnd, &rc);
    gCraft.layout(rc.right - rc.left, rc.bottom - rc.top);
    gCenter.create(hWnd, hInstance);
    gCenter.layout(rc.right - rc.left, rc.bottom - rc.top);

    // Assets for dark mode
    if (!g_hbrDark)     g_hbrDark = CreateSolidBrush(RGB(32, 32, 32));
    if (!g_hbrMenuDark) g_hbrMenuDark = CreateSolidBrush(kMenuBgDark);
    if (!g_hMenuFont)   g_hMenuFont = CreateMenuFont();

    // Sync toggle item
    CheckMenuItem(hMenu, IDM_VIEW_DARKMODE,
        MF_BYCOMMAND | (g_DarkMode ? MF_CHECKED : MF_UNCHECKED));
    EnableMenuItem(hMenu, IDM_VIEW_DARKMODE, MF_BYCOMMAND | MF_ENABLED);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
    return TRUE;
}

// Utility: if the cursor is over the tree, do a hit-test and expand/collapse a category node.
// This runs at parent level (fallback) so clicks are handled even when TreeView doesn't get button events.
static void TryExpandTreeAtCursor()
{
    HWND hTree = gCraft.getTree();
    if (!hTree || !IsWindow(hTree)) return;

    POINT pt; GetCursorPos(&pt);
    POINT ptTree = pt;
    ScreenToClient(hTree, &ptTree);

    TVHITTESTINFO ht{};
    ht.pt = ptTree;
    HTREEITEM hit = (HTREEITEM)SendMessageW(hTree, TVM_HITTEST, 0, (LPARAM)&ht);
    if (hit && (ht.flags & (TVHT_ONITEMICON | TVHT_ONITEMLABEL | TVHT_ONITEMSTATEICON | TVHT_ONITEMBUTTON))) {
        TVITEMW it{ 0 }; it.mask = TVIF_CHILDREN; it.hItem = hit;
        if (TreeView_GetItem(hTree, &it) && it.cChildren > 0) {
            UINT st = TreeView_GetItemState(hTree, hit, TVIS_EXPANDED);
            // Use TVM_EXPAND (SendMessage) as the fallback expansion method
            SendMessageW(hTree, TVM_EXPAND, (WPARAM)((st & TVIS_EXPANDED) ? TVE_COLLAPSE : TVE_EXPAND), (LPARAM)hit);
            // Also set selection so TVN_SELCHANGED will fire on normal cases
            SendMessageW(hTree, TVM_SELECTITEM, (WPARAM)TVGN_CARET, (LPARAM)hit);
        }
    }
}

// ---------------- Window procedure ----------------
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_SIZE:
    {
        const int cw = LOWORD(lParam);
        const int ch = HIWORD(lParam);
        gCraft.layout(cw, ch);
        gCenter.layout(cw, ch);
        return 0;
    }

    case WM_COMMAND:
    {
        // Defer qty edits from the buddy EDIT (prevents recursion/beep)
        if ((HWND)lParam == gCenter.getQtyEdit() && HIWORD(wParam) == EN_CHANGE) {
            PostMessageW(hWnd, WM_APP_QTY_CHANGED, 0, 0);
            return 0;
        }

        // Fallback button clicks for qty increment/decrement (these are the small ▲/▼ buttons)
        if ((HWND)lParam == gCenter.getQtyUpBtn() && HIWORD(wParam) == BN_CLICKED) {
            int q = gCenter.qty();
            gCenter.setQty(q + 1);
            gCenter.rebuildNow();
            return 0;
        }
        if ((HWND)lParam == gCenter.getQtyDownBtn() && HIWORD(wParam) == BN_CLICKED) {
            int q = gCenter.qty();
            gCenter.setQty(q - 1);
            gCenter.rebuildNow();
            return 0;
        }

        const int wmId = LOWORD(wParam);
        switch (wmId)
        {
        case IDM_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            return 0;

        case IDM_EXIT:
            DestroyWindow(hWnd);
            return 0;

        case IDM_VIEW_DARKMODE:
            g_DarkMode = !g_DarkMode;
            gCraft.onThemeChanged(g_DarkMode);
            gCenter.setDark(g_DarkMode);

            {
                HMENU hMenu = GetMenu(hWnd);
                CheckMenuItem(hMenu, IDM_VIEW_DARKMODE,
                    MF_BYCOMMAND | (g_DarkMode ? MF_CHECKED : MF_UNCHECKED));

                ApplyDarkModeAttribute(hWnd, g_DarkMode);

                // Owner-draw ON in dark; OFF in light
                SetMenuOwnerDraw(hMenu, g_DarkMode);

                if (!g_DarkMode) {
                    // Revert to stock light menu completely
                    FreeMenuItemData(hMenu);          // drop any cached owner-draw strings
                    ApplyMenuBackground(hMenu, nullptr);
                }
                else {
                    ApplyMenuBackground(hMenu, g_hbrMenuDark);
                }

                DrawMenuBar(hWnd);
                InvalidateRect(hWnd, nullptr, TRUE);
            }
            return 0;
        }

        return 0; // close WM_COMMAND cleanly
    }

    case WM_APP_QTY_CHANGED:
    {
        int v = gCenter.qty();
        SendMessageW(gCenter.getQtyUpDown(), UDM_SETPOS32, 0, v);
        gCenter.rebuildNow();
        return 0;
    }

    // Parent-level fallback: if the user clicks anywhere in the main window and the cursor
    // is over the tree, try to expand/collapse the item. This helps systems where the TreeView
    // isn't getting button messages but the parent can detect the click.
    case WM_LBUTTONUP:
    {
        TryExpandTreeAtCursor();
        break; // allow default processing too
    }

    case WM_NOTIFY:
    {
        auto* hdr = (NMHDR*)lParam;

        // Observe native tree expand notifications for debugging (optional)
        if (hdr->hwndFrom == gCraft.getTree())
        {
            if (hdr->code == TVN_ITEMEXPANDINGW)
            {
                auto* tv = (NMTREEVIEWW*)lParam;
                wchar_t dbg[256];
                wsprintfW(dbg, L"TVN_ITEMEXPANDING: itemNew=%p itemOld=%p action=%d\n",
                    tv->itemNew.hItem, tv->itemOld.hItem, (int)tv->action);
                OutputDebugStringW(dbg);
            }
            else if (hdr->code == TVN_ITEMEXPANDEDW)
            {
                auto* tv = (NMTREEVIEWW*)lParam;
                wchar_t dbg[256];
                wsprintfW(dbg, L"TVN_ITEMEXPANDED: itemNew=%p itemOld=%p action=%d\n",
                    tv->itemNew.hItem, tv->itemOld.hItem, (int)tv->action);
                OutputDebugStringW(dbg);
            }
        }

        // TreeView selection -> set recipe OR expand if category selected
        if (hdr->hwndFrom == gCraft.getTree() && hdr->code == TVN_SELCHANGEDW)
        {
            auto* tv = (NMTREEVIEWW*)lParam;

            // Ignore category nodes (only act on leaves)
            TVITEMW it{}; wchar_t text[256] = L"";
            it.mask = TVIF_TEXT | TVIF_CHILDREN;
            it.hItem = tv->itemNew.hItem;
            it.pszText = text; it.cchTextMax = _countof(text);

            TreeView_GetItem(hdr->hwndFrom, &it);
            if (it.cChildren == 0 && text[0] != L'\0') {
                gCenter.setRecipe(text); // triggers rebuildList()
            }
            else {
                // category selected -> expand (helpful fallback if + isn't clickable)
                UINT st = TreeView_GetItemState(hdr->hwndFrom, it.hItem, TVIS_EXPANDED);
                TreeView_Expand(hdr->hwndFrom, it.hItem, (st & TVIS_EXPANDED) ? TVE_COLLAPSE : TVE_EXPAND);
            }
            return 0; // handled
        }

        break;
    }

    // --- Make static/edit controls match the theme (CenterPanel implements handlers)
    case WM_CTLCOLORSTATIC:
        return (LRESULT)gCenter.onCtlColorStatic((HDC)wParam, (HWND)lParam);

    case WM_CTLCOLOREDIT:
        return (LRESULT)gCenter.onCtlColorEdit((HDC)wParam, (HWND)lParam);

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        if (g_DarkMode && g_hbrDark) {
            FillRect(hdc, &ps.rcPaint, g_hbrDark);
        }
        else {
            FillRect(hdc, &ps.rcPaint, GetSysColorBrush(COLOR_WINDOW));
        }
        EndPaint(hWnd, &ps);
        return 0;
    }

    case WM_DESTROY:
    {
        HMENU hMenu = GetMenu(hWnd);
        FreeMenuItemData(hMenu);

        if (g_hMenuFont) { DeleteObject(g_hMenuFont);   g_hMenuFont = nullptr; }
        if (g_hbrMenuDark) { DeleteObject(g_hbrMenuDark); g_hbrMenuDark = nullptr; }
        if (g_hbrDark) { DeleteObject(g_hbrDark);     g_hbrDark = nullptr; }

        gCraft.destroy();
        PostQuitMessage(0);
        return 0;
    }

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
}

// Stock About dialog
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        SetWindowTextW(hDlg, L"About GoonZu Manufacturing Tool");
        SetDlgItemTextW(hDlg, IDC_ABOUT_TEXT,
            L"GoonZu Manufacturing Tool\r\n"
            L"Version: 0.1\r\n"
            L"Author: itsGir\r\n"
            L"Copyright (c) 2025\r\n"
            L"\r\n"
            L"Please report any bugs and suggestions to me on Discord");
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        // Close on OK/Cancel (modal dialog)
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE; // handled
        }
        return (INT_PTR)FALSE; // let default handle other commands

    case WM_CLOSE:
        EndDialog(hDlg, IDCANCEL);
        return (INT_PTR)TRUE;
    }
    return (INT_PTR)FALSE;
}