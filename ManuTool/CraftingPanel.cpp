#include "framework.h"
#include "CraftingPanel.h"
#include <commctrl.h>
#include <uxtheme.h>
#include "resource.h"
#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "UxTheme.lib")

#include "ItemRegistry.h"    // for icon tables (optional; fine to keep)
#include "RecipeModel.h"     // for RecipesDB::AllNames()
#include <unordered_map>
#include <vector>
#include <algorithm>

CraftingPanel gCraft; // single global

// ---------------- fonts ----------------
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
    lf.lfHeight = (int)(lf.lfHeight * 1.3);
    return CreateFontIndirectW(&lf);
}

// forward for subclass callback
static LRESULT CALLBACK TreeSubclassProc(
    HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
    UINT_PTR, DWORD_PTR)
{
    // Log mouse messages so we can confirm the tree is getting input directly.
    if (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONDBLCLK || msg == WM_RBUTTONDOWN || msg == WM_MOUSEMOVE) {
        POINT pt{};
        if (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONDBLCLK || msg == WM_RBUTTONDOWN) {
            pt.x = GET_X_LPARAM(lParam); pt.y = GET_Y_LPARAM(lParam);
        }
        else if (msg == WM_MOUSEMOVE) {
            pt.x = GET_X_LPARAM(lParam); pt.y = GET_Y_LPARAM(lParam);
        }
        wchar_t dbg[256];
        wsprintfW(dbg, L"TreeSubclass: msg=%u pt=(%d,%d) hwnd=%p\n", msg, pt.x, pt.y, hwnd);
        OutputDebugStringW(dbg);
    }
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

// ---------------- creation/layout ----------------
void CraftingPanel::create(HWND parent, HINSTANCE inst)
{
    hParent = parent; hInst = inst;
    if (!uiFont)     uiFont = createUIFont();
    if (!headerFont) headerFont = createHeaderFont();

    INITCOMMONCONTROLSEX icc{ sizeof(icc), ICC_TREEVIEW_CLASSES | ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&icc);

    // header
    hHeader = CreateWindowExW(0, L"STATIC", L"Select your recipe",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        0, 0, kPanelWidth - 2 * kMargin, 24, hParent, nullptr, hInst, nullptr);
    if (headerFont) SendMessageW(hHeader, WM_SETFONT, (WPARAM)headerFont, TRUE);

    // tree
    hTree = CreateWindowExW(WS_EX_CLIENTEDGE, WC_TREEVIEWW, L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_CLIPSIBLINGS |
        TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT | TVS_SHOWSELALWAYS,
        0, 0, kPanelWidth - 2 * kMargin, 200, hParent, nullptr, hInst, nullptr);

    // Install a lightweight subclass on the tree to log raw mouse messages.
    // This will tell us whether mouse events are reaching the tree window.
    if (hTree) {
        SetWindowSubclass(hTree, TreeSubclassProc, 1, 0);
        wchar_t dbg[128];
        wsprintfW(dbg, L"CraftingPanel::create: installed tree subclass hwnd=%p\n", hTree);
        OutputDebugStringW(dbg);
    }
    else {
        OutputDebugStringW(L"CraftingPanel::create: FAILED to create tree\n");
    }

    // icons
    hImgList = ImageList_Create(kIconSize, kIconSize, ILC_COLOR32 | ILC_MASK, 32, 32);
    TreeView_SetImageList(hTree, hImgList, TVSIL_NORMAL);
    TreeView_SetItemHeight(hTree, kIconSize + 8);
    TreeView_SetExtendedStyle(hTree, TVS_EX_DOUBLEBUFFER, TVS_EX_DOUBLEBUFFER);
    if (uiFont) SendMessageW(hTree, WM_SETFONT, (WPARAM)uiFont, TRUE);
    SetWindowTheme(hTree, L"Explorer", nullptr);

    onThemeChanged(false);
    populate();

    // divider
    hDivider = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_ETCHEDVERT,
        0, 0, 2, 10, hParent, nullptr, hInst, nullptr);
}

void CraftingPanel::layout(int /*clientW*/, int clientH)
{
    const int x = 0;
    const int w = kPanelWidth;
    MoveWindow(hHeader, x + kMargin, kMargin, w - 2 * kMargin, 24, TRUE);

    const int treeTop = kMargin + 24 + 6;
    MoveWindow(hTree, x + kMargin, treeTop, w - 2 * kMargin, clientH - treeTop - kMargin, TRUE);
    MoveWindow(hDivider, w - 1, 0, 2, clientH, TRUE);
}

// ---------------- theming ----------------
void CraftingPanel::onThemeChanged(bool darkMode)
{
    dark = darkMode;

    if (dark) {
        COLORREF bk = RGB(32, 32, 32);
        COLORREF text = RGB(240, 240, 240);
        COLORREF line = RGB(96, 96, 96);
        TreeView_SetBkColor(hTree, bk);
        TreeView_SetTextColor(hTree, text);
        SendMessageW(hTree, TVM_SETLINECOLOR, 0, (LPARAM)line);
    }
    else {
        COLORREF bk = GetSysColor(COLOR_WINDOW);
        COLORREF text = GetSysColor(COLOR_WINDOWTEXT);
        COLORREF line = GetSysColor(COLOR_3DSHADOW);
        TreeView_SetBkColor(hTree, bk);
        TreeView_SetTextColor(hTree, text);
        SendMessageW(hTree, TVM_SETLINECOLOR, 0, (LPARAM)line);
    }

    // Make sure theme & font reapply cleanly
    SetWindowTheme(hTree, L"Explorer", nullptr);
    if (uiFont) SendMessageW(hTree, WM_SETFONT, (WPARAM)uiFont, TRUE);

    InvalidateRect(hTree, nullptr, TRUE);
}

HBRUSH CraftingPanel::onCtlColorStatic(HDC hdc, bool darkMode)
{
    if (!darkMode) return nullptr;
    SetTextColor(hdc, RGB(240, 240, 240));
    SetBkColor(hdc, RGB(32, 32, 32));
    static HBRUSH hbr = nullptr;
    if (!hbr) hbr = CreateSolidBrush(RGB(32, 32, 32));
    return hbr;
}

// ---------------- selection API ----------------
bool CraftingPanel::getSelection(std::wstring& category, std::wstring& item) const
{
    category = selCat_; item = selItem_;
    return !item.empty();
}

// ---------------- image list helpers ----------------
int CraftingPanel::addIcon(HICON h)
{
    if (!hImgList || !h) return -1;
    return ImageList_AddIcon(hImgList, h);
}

int CraftingPanel::iconIndexFor(const wchar_t* category, const wchar_t* item) const
{
    // Map (category,item) to resource id using ItemRegistry tables
    struct Table { const IconEntry* items; size_t count; };
    static std::unordered_map<std::wstring, Table> sRegistry;
    if (sRegistry.empty()) {
        for (size_t i = 0; i < kAllCategoriesCount; ++i) {
            std::wstring lower = kAllCategories[i].name;
            for (auto& ch : lower) ch = towlower(ch);
            sRegistry.emplace(lower, Table{ kAllCategories[i].items, kAllCategories[i].count });
        }
    }

    if (!item || !*item) return -1;

    std::wstring catKey = category ? category : L"";
    for (auto& ch : catKey) ch = towlower(ch);

    auto it = sRegistry.find(catKey);
    if (it == sRegistry.end()) return -1;

    const IconEntry* arr = it->second.items;
    const size_t n = it->second.count;

    static std::unordered_map<int, int> sResToImg;
    for (size_t i = 0; i < n; ++i) {
        if (_wcsicmp(item, arr[i].name) == 0) {
            auto f = sResToImg.find(arr[i].resId);
            if (f != sResToImg.end()) return f->second;

            HICON h = (HICON)LoadImageW(hInst, MAKEINTRESOURCEW(arr[i].resId),
                IMAGE_ICON, kIconSize, kIconSize, LR_DEFAULTCOLOR);
            if (!h) h = (HICON)LoadImageW(hInst, MAKEINTRESOURCEW(108 /*IDI_SMALL*/),
                IMAGE_ICON, kIconSize, kIconSize, LR_DEFAULTCOLOR);
            int idx = const_cast<CraftingPanel*>(this)->addIcon(h);
            if (h) DestroyIcon(h);
            sResToImg.emplace(arr[i].resId, idx);
            return idx;
        }
    }
    return -1;
}

// ---------------- teardown ----------------
void CraftingPanel::destroy()
{
    if (hImgList) { ImageList_Destroy(hImgList); hImgList = nullptr; }
}

// ---------------- data population (from DB) ----------------
HTREEITEM CraftingPanel::addCategory(const wchar_t* title)
{
    TVINSERTSTRUCTW tvis{};
    tvis.hParent = TVI_ROOT;
    tvis.hInsertAfter = TVI_LAST;

    // Tell the TreeView this node has (or will have) children.
    // This makes the [+] expand handle appear and enables expansion.
    tvis.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN;
    tvis.item.pszText = const_cast<LPWSTR>(title);
    tvis.item.iImage = tvis.item.iSelectedImage = I_IMAGENONE; // no icon for category nodes
    tvis.item.cChildren = 1; // important: mark as expandable

    return (HTREEITEM)SendMessageW(hTree, TVM_INSERTITEM, 0, (LPARAM)&tvis);
}

void CraftingPanel::addItem(HTREEITEM parent, const std::wstring& name, const wchar_t* categoryName)
{
    TVINSERTSTRUCTW t{};
    t.hParent = parent; t.hInsertAfter = TVI_LAST;
    t.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    t.item.pszText = const_cast<LPWSTR>(name.c_str());
    const int idx = iconIndexFor(categoryName, name.c_str());
    t.item.iImage = t.item.iSelectedImage = (idx >= 0 ? idx : I_IMAGENONE);
    SendMessageW(hTree, TVM_INSERTITEM, 0, (LPARAM)&t);

    // Ensure parent shows an expand handle
    TVITEMW parentItem{};
    parentItem.mask = TVIF_CHILDREN;
    parentItem.hItem = parent;
    parentItem.cChildren = 1;
    SendMessageW(hTree, TVM_SETITEMW, 0, (LPARAM)&parentItem);
}

void CraftingPanel::populate()
{
    TreeView_DeleteAllItems(hTree);

    // Pull all recipe names and keep them in a simple vector
    std::vector<std::wstring> remaining = RecipesDB::AllNames();

    // --- Step 1: populate known categories from ItemRegistry.h
    for (size_t c = 0; c < kAllCategoriesCount; ++c)
    {
        const auto& cat = kAllCategories[c];
        HTREEITEM hCat = addCategory(cat.name);
        bool any = false;

        for (size_t i = 0; i < cat.count; ++i)
        {
            const std::wstring itemName = cat.items[i].name;
            auto it = std::find_if(
                remaining.begin(), remaining.end(),
                [&](const std::wstring& n) { return _wcsicmp(n.c_str(), itemName.c_str()) == 0; });

            if (it != remaining.end())
            {
                addItem(hCat, *it, cat.name);
                remaining.erase(it);
                any = true;
            }
        }

        if (any) TreeView_Expand(hTree, hCat, TVE_COLLAPSE);
        else     TreeView_DeleteItem(hTree, hCat);
    }
}