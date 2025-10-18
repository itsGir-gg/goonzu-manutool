#include "framework.h"
#include "SkillsPanel.h"
#include <shlobj.h>
#include <cwchar>
#include <algorithm>   // add if missing

#pragma comment(lib, "Shell32.lib")

// Real names
static const wchar_t* kSkillNames[20] = {
    L"Prescription", L"Cooking", L"Armor Manufacturing", L"Helmet Manufacturing", L"Sword Manufacturing",
    L"Spear Manufacturing", L"Axe Manufacturing", L"Cane Manufacturing", L"Bow Manufacturing", L"Gun Manufacturing",
    L"Shoes Manufacturing", L"Belt Manufacturing", L"Ironware Manufacturing", L"Tool Manufacturing", L"Crystal Manufacturing",
    L"Clothes Manufacturing", L"Hat Manufacturing", L"Cloak Manufacturing", L"Ring Manufacturing", L"Necklace Manufacturing"
};

static const int kNameW = 200;
static const int kLabelW = 48;   // "Level"

static HICON loadSkillIcon(HINSTANCE hInst, int idx, int size) {
    int resId = 5000 + idx; // optional; OK if missing
    HICON h = (HICON)LoadImageW(hInst, MAKEINTRESOURCEW(resId),
        IMAGE_ICON, size, size, LR_DEFAULTCOLOR);
    if (!h) {
        h = (HICON)LoadImageW(hInst, MAKEINTRESOURCEW(108 /*IDI_SMALL*/),
            IMAGE_ICON, size, size, LR_DEFAULTCOLOR);
    }
    return h;
}

static HFONT createUIFont() {
    NONCLIENTMETRICSW ncm{ sizeof(ncm) };
    if (!SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0))
        return (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    return CreateFontIndirectW(&ncm.lfMessageFont);
}
static HFONT createHeaderFont() {
    NONCLIENTMETRICSW ncm{ sizeof(ncm) };
    if (!SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0))
        return (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    LOGFONTW lf = ncm.lfMessageFont;
    lf.lfWeight = FW_SEMIBOLD;
    // make ~2pt larger
    lf.lfHeight = (int)(lf.lfHeight * 1.3); // negative height keeps logical size
    return CreateFontIndirectW(&lf);
}

SkillsPanel gSkills;

void SkillsPanel::create(HWND parent, HINSTANCE inst)
{
    hParent = parent; hInst = inst;
    if (!uiFont)     uiFont = createUIFont();
    if (!headerFont) headerFont = createHeaderFont();

    // Header: "Enter your skill levels here"
    hHeader = CreateWindowExW(0, L"STATIC", L"Enter your skill levels here",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        0, 0, 200, 24, hParent, nullptr, hInst, nullptr);
    if (headerFont) SendMessageW(hHeader, WM_SETFONT, (WPARAM)headerFont, TRUE);

    for (int i = 0; i < kNumSkills; ++i) {
        rows[i].hIcon = CreateWindowExW(0, L"STATIC", nullptr,
            WS_CHILD | WS_VISIBLE | SS_ICON,
            0, 0, kIconSize, kIconSize, hParent, (HMENU)(INT_PTR)(4000 + i), hInst, nullptr);
        SendMessageW(rows[i].hIcon, STM_SETICON, (WPARAM)loadSkillIcon(hInst, i, kIconSize), 0);

        rows[i].hName = CreateWindowExW(0, L"STATIC", kSkillNames[i],
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            0, 0, kNameW, 20, hParent, (HMENU)(INT_PTR)(4100 + i), hInst, nullptr);

        rows[i].hLbl = CreateWindowExW(0, L"STATIC", L"Level",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            0, 0, kLabelW, 20, hParent, (HMENU)(INT_PTR)(4200 + i), hInst, nullptr);

        rows[i].hEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"0",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_LEFT | ES_AUTOHSCROLL,
            0, 0, 64, 22, hParent, (HMENU)(INT_PTR)(4300 + i), hInst, nullptr);

        if (uiFont) {
            SendMessageW(rows[i].hName, WM_SETFONT, (WPARAM)uiFont, TRUE);
            SendMessageW(rows[i].hLbl, WM_SETFONT, (WPARAM)uiFont, TRUE);
            SendMessageW(rows[i].hEdit, WM_SETFONT, (WPARAM)uiFont, TRUE);
        }
    }

    // divider
    hDivider = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_ETCHEDVERT,
        0, 0, 2, 10, hParent, nullptr, hInst, nullptr);

    load();
}

void SkillsPanel::layout(int clientW, int clientH)
{
    const int x = std::max(0, clientW - kPanelWidth);
    const int w = kPanelWidth;

    MoveWindow(hHeader, x + kMargin, kMargin, w - 2 * kMargin, 24, TRUE);
    const int listTop = kMargin + 24 + 6;
    MoveWindow(hList, x + kMargin, listTop, w - 2 * kMargin, clientH - listTop - kMargin, TRUE);
    MoveWindow(hDivider, x - 1, 0, 2, clientH, TRUE);   // left divider of the right panel
}

// (load/save unchanged)
std::wstring SkillsPanel::iniPath() const { /* … keep your current code … */ }
void SkillsPanel::load() { /* … keep your current code … */ }
void SkillsPanel::save() { /* … keep your current code … */ }

HBRUSH SkillsPanel::onCtlColorStatic(HDC hdc, bool darkMode)
{
    if (!darkMode) return nullptr;
    SetTextColor(hdc, RGB(240, 240, 240));
    SetBkColor(hdc, RGB(32, 32, 32));
    static HBRUSH hbr = nullptr;
    if (!hbr) hbr = CreateSolidBrush(RGB(32, 32, 32));
    return hbr;
}
HBRUSH SkillsPanel::onCtlColorEdit(HDC hdc, bool darkMode)
{
    if (!darkMode) return nullptr;
    SetTextColor(hdc, RGB(240, 240, 240));
    SetBkColor(hdc, RGB(40, 40, 40));
    static HBRUSH hbr = nullptr;
    if (!hbr) hbr = CreateSolidBrush(RGB(40, 40, 40));
    return hbr;
}
