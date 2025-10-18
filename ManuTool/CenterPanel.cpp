#include "framework.h"
#include "CenterPanel.h"
#include <commctrl.h>
#include <string>
#include "CraftingPanel.h"  // to get kPanelWidth
#include <algorithm>         // for std::max
#pragma comment(lib, "Comctl32.lib")
#define WM_APP_QTY_CHANGED (WM_APP + 42)

static HFONT createUIFont() {
    NONCLIENTMETRICSW ncm{ sizeof(ncm) };
    if (!SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0))
        return (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    return CreateFontIndirectW(&ncm.lfMessageFont);
}
static HFONT createHeaderFont() {
    NONCLIENTMETRICSW ncm{ sizeof(ncm) };
    SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
    LOGFONTW lf = ncm.lfMessageFont;
    lf.lfWeight = FW_SEMIBOLD; lf.lfHeight = (int)(lf.lfHeight * 1.2);
    return CreateFontIndirectW(&lf);
}

void CenterPanel::recreateBrush() {
    if (_bgBrush) { DeleteObject(_bgBrush); _bgBrush = nullptr; }
    _bgBrush = CreateSolidBrush(_bg);
}

void CenterPanel::setDark(bool darkMode) {
    _dark = darkMode;
    if (_dark) {
        _bg = RGB(32, 33, 36);        // dark surface
        _fg = RGB(232, 234, 237);     // light text
    }
    else {
        _bg = RGB(255, 255, 255);     // light surface
        _fg = RGB(0, 0, 0);           // dark text
    }
    recreateBrush();

    // repaint our area; parent is fine if controls are children
    if (hParent) InvalidateRect(hParent, nullptr, TRUE);
}

// STATICs (labels)
HBRUSH CenterPanel::onCtlColorStatic(HDC hdc, HWND) {
    if (!_dark) return nullptr; // let system paint default in light mode
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, _fg);
    return _bgBrush ? _bgBrush : (HBRUSH)GetStockObject(NULL_BRUSH);
}

// EDITs
HBRUSH CenterPanel::onCtlColorEdit(HDC hdc, HWND) {
    if (!_dark) return nullptr; // system default in light mode
    SetBkMode(hdc, OPAQUE);
    SetBkColor(hdc, _bg);
    SetTextColor(hdc, _fg);
    return _bgBrush ? _bgBrush : (HBRUSH)GetStockObject(WHITE_BRUSH);
}

CenterPanel gCenter;

void CenterPanel::rebuildNow() { rebuildList(); }

void CenterPanel::initFonts() {
    if (!uiFont) uiFont = createUIFont();
    if (!headerFont) headerFont = createHeaderFont();
}

void CenterPanel::create(HWND parent, HINSTANCE hInst)
{
    hParent = parent; initFonts();

    // Safety: ensure the up-down/common controls are registered in case this panel is created
    // before any other init calls; safe to call repeatedly.
    INITCOMMONCONTROLSEX icc{ sizeof(icc), ICC_UPDOWN_CLASS | ICC_STANDARD_CLASSES | ICC_LISTVIEW_CLASSES };
    InitCommonControlsEx(&icc);

    hHeader = CreateWindowExW(0, L"STATIC", L"Recipe",
        WS_CHILD | WS_VISIBLE | SS_LEFT, 0, 0, 100, 24, parent, nullptr, hInst, nullptr);

    hQtyEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"1",
        WS_CHILD | WS_VISIBLE | ES_NUMBER | ES_LEFT, 0, 0, 60, 24, parent, nullptr, hInst, nullptr);

    hQtyLabel = CreateWindowExW(
        0, L"STATIC", L"Quantity:",
        WS_CHILD | WS_VISIBLE | SS_RIGHT,
        0, 0, 60, 20, hParent, nullptr, hInst, nullptr);
    if (uiFont) SendMessageW(hQtyLabel, WM_SETFONT, (WPARAM)uiFont, TRUE);

    // Subclass to detect Enter press in quantity box
    SetWindowSubclass(
        hQtyEdit,
        [](HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
            UINT_PTR, DWORD_PTR refData) -> LRESULT
        {
            CenterPanel* self = reinterpret_cast<CenterPanel*>(refData);
            if (!self) return DefSubclassProc(hwnd, msg, wParam, lParam);

            switch (msg)
            {
            case WM_GETDLGCODE:
                // Tell the parent/dialog that this edit wants to handle keys like Enter
                return DLGC_WANTALLKEYS | DLGC_WANTCHARS;

            case WM_KEYDOWN:
                if (wParam == VK_RETURN) {
                    // Defer the rebuild (don’t write into the edit here)
                    PostMessageW(self->hParent, WM_APP_QTY_CHANGED, 0, 0);
                    return 0; // swallow Enter → no beep
                }
                break;

            case WM_CHAR:
                if (wParam == L'\r') {
                    // Belt-and-suspenders: also swallow Enter at WM_CHAR
                    return 0;
                }
                break;

            case WM_KILLFOCUS:
                // Recalculate on focus loss (optional)
                PostMessageW(self->hParent, WM_APP_QTY_CHANGED, 0, 0);
                break;
            }
            return DefSubclassProc(hwnd, msg, wParam, lParam);
        },
        1, reinterpret_cast<DWORD_PTR>(this));

    // Native UPDOWN (keep for systems where it works)
    hQtyUpDown = CreateWindowExW(0, UPDOWN_CLASSW, L"",
        WS_CHILD | WS_VISIBLE | UDS_SETBUDDYINT | UDS_ARROWKEYS,
        0, 0, 18, 24, parent, nullptr, hInst, nullptr);

    // Fallback small buttons (guarantee increment/decrement works)
    hQtyUpBtn = CreateWindowExW(0, L"BUTTON", L"▲",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_CENTER,
        0, 0, 18, 12, parent, nullptr, hInst, nullptr);
    hQtyDownBtn = CreateWindowExW(0, L"BUTTON", L"▼",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_CENTER,
        0, 0, 18, 12, parent, nullptr, hInst, nullptr);

    // (optional) debug: confirm up-down created. If NULL, we'll see this in DebugView.
    // (also confirm the fallback buddy at creation time - buddy will be set below)
    {
        wchar_t dbg[128];
        if (!hQtyUpDown) {
            wsprintfW(dbg, L"CenterPanel::create: failed to create UPDOWN control (hQtyUpDown == NULL)\n");
            OutputDebugStringW(dbg);
        }
        else {
            wsprintfW(dbg, L"CenterPanel::create: UPDOWN hwnd=%p\n", hQtyUpDown);
            OutputDebugStringW(dbg);
        }
    }

    SendMessageW(hQtyUpDown, UDM_SETBUDDY, (WPARAM)hQtyEdit, 0);
    SendMessageW(hQtyUpDown, UDM_SETRANGE32, 1, 9999);
    SendMessageW(hQtyUpDown, UDM_SETPOS32, 0, 1);

    hList = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_CLIPSIBLINGS |
        LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
        0, 0, 300, 200, parent, nullptr, hInst, nullptr);

    ListView_SetExtendedListViewStyle(hList,
        LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_GRIDLINES);

    LVCOLUMNW col{};
    col.mask = LVCF_TEXT | LVCF_WIDTH;
    col.pszText = const_cast<wchar_t*>(L"Type");   col.cx = 100; ListView_InsertColumn(hList, 0, &col);
    col.pszText = const_cast<wchar_t*>(L"Name");   col.cx = 220; ListView_InsertColumn(hList, 1, &col);
    col.pszText = const_cast<wchar_t*>(L"Qty");    col.cx = 80;  ListView_InsertColumn(hList, 2, &col);

    if (uiFont) {
        SendMessageW(hHeader, WM_SETFONT, (WPARAM)headerFont, TRUE);
        SendMessageW(hQtyLabel, WM_SETFONT, (WPARAM)uiFont, TRUE);
        SendMessageW(hQtyEdit, WM_SETFONT, (WPARAM)uiFont, TRUE);
        SendMessageW(hList, WM_SETFONT, (WPARAM)uiFont, TRUE);
        SendMessageW(hQtyUpBtn, WM_SETFONT, (WPARAM)uiFont, TRUE);
        SendMessageW(hQtyDownBtn, WM_SETFONT, (WPARAM)uiFont, TRUE);
    }
}

int CenterPanel::qty() const {
    wchar_t b[32]{};
    GetWindowTextW(hQtyEdit, b, 32);
    int v = _wtoi(b); if (v < 1) v = 1; if (v > 9999) v = 9999;
    return v;
}
void CenterPanel::setQty(int q) {
    if (q < 1) q = 1; if (q > 9999) q = 9999;
    wchar_t b[32]; wsprintfW(b, L"%d", q);
    SetWindowTextW(hQtyEdit, b);
    SendMessageW(hQtyUpDown, UDM_SETPOS32, 0, q);
}

void CenterPanel::layout(int clientW, int clientH)
{
    const int left = CraftingPanel::kPanelWidth + 2;
    const int width = (clientW > left) ? (clientW - left) : 0;
    const int gap = 6;
    const int qtyW = 64;
    const int qtyH = 24;

    // Header
    MoveWindow(hHeader, left + kMargin, kMargin,
        (std::max)(0, width - 2 * kMargin), 24, TRUE);

    // Quantity EDIT (top-right of center area)
    const int editX = left + (std::max)(0, width - (qtyW + kMargin));
    const int editY = kMargin;
    MoveWindow(hQtyEdit, editX, editY, qtyW, qtyH, TRUE);

    const int labelW = 80, labelH = 16;
    MoveWindow(hQtyLabel, editX - (labelW + 6), editY + (qtyH - labelH) / 2, labelW, labelH, TRUE);

    // Place the fallback up/down buttons to the right of the edit (stacked)
    const int btnW = 18, btnH = 12;
    MoveWindow(hQtyUpBtn, editX + qtyW + 2, editY, btnW, btnH, TRUE);
    MoveWindow(hQtyDownBtn, editX + qtyW + 2, editY + btnH, btnW, btnH, TRUE);

    // Re-anchor native UPDOWN immediately to the right of the edit (if available)
    const int spinW = GetSystemMetrics(SM_CXVSCROLL);
    MoveWindow(hQtyUpDown, editX + qtyW, editY, spinW, qtyH, TRUE);

    SendMessageW(hQtyUpDown, UDM_SETBUDDY, (WPARAM)hQtyEdit, 0);

    // keep qty widgets above the list
    SetWindowPos(hQtyLabel, hList, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    SetWindowPos(hQtyEdit, hList, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    SetWindowPos(hQtyUpDown, hList, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    SetWindowPos(hQtyUpBtn, hList, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    SetWindowPos(hQtyDownBtn, hList, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

    // List view fills the rest
    const int listTop = kMargin + 24 + gap;
    MoveWindow(hList, left + kMargin, listTop,
        (std::max)(0, width - 2 * kMargin),
        (std::max)(0, clientH - listTop - kMargin), TRUE);

    // (optional) belt-and-suspenders: ensure they’re visible
    ShowWindow(hQtyLabel, SW_SHOW);
    ShowWindow(hQtyEdit, SW_SHOW);
    ShowWindow(hQtyUpDown, SW_SHOW);
    ShowWindow(hQtyUpBtn, SW_SHOW);
    ShowWindow(hQtyDownBtn, SW_SHOW);
}

void CenterPanel::setRecipe(const std::wstring& name)
{
    currentRecipe = name;
    std::wstring hdr = L"Recipe: " + name;
    SetWindowTextW(hHeader, hdr.c_str());
    setQty(1);
    rebuildList();
}

void CenterPanel::rebuildList()
{
    ListView_DeleteAllItems(hList);
    if (currentRecipe.empty()) return;

    BOMResult bom{};
    BuildBOM(currentRecipe, qty(), bom);

    auto addRow = [&](const wchar_t* type, const std::wstring& name, int q) {
        LVITEMW it{}; it.mask = LVIF_TEXT; it.iItem = ListView_GetItemCount(hList);
        it.pszText = const_cast<wchar_t*>(type); ListView_InsertItem(hList, &it);
        ListView_SetItemText(hList, it.iItem, 1, const_cast<wchar_t*>(name.c_str()));
        wchar_t nbuf[32]; wsprintfW(nbuf, L"%d", q);
        ListView_SetItemText(hList, it.iItem, 2, nbuf);
        };

    for (auto& kv : bom.materials) addRow(L"Material", kv.first, kv.second);
    for (auto& kv : bom.tools)     addRow(L"Tool", kv.first, kv.second);
}