#pragma once
#include <windows.h>
#include <string>
#include <commctrl.h>

// ---------------- CraftingPanel ----------------
struct CraftingPanel {
    // UI constants (tweak if you like)
    static constexpr int kPanelWidth = 360;
    static constexpr int kMargin = 8;
    static constexpr int kIconSize = 24;

    // lifecycle
    void create(HWND parent, HINSTANCE inst);
    void layout(int clientW, int clientH);
    void onThemeChanged(bool darkMode);
    HBRUSH onCtlColorStatic(HDC hdc, bool darkMode);
    void destroy();

    // selection API
    bool getSelection(std::wstring& category, std::wstring& item) const;
    HWND getTree() const { return hTree; }

    // populate the tree from RecipesDB (implemented in .cpp)
    void populate();

private:
    // helpers (implemented in .cpp)
    HTREEITEM addCategory(const wchar_t* title);
    void      addItem(HTREEITEM parent, const std::wstring& name, const wchar_t* categoryName);
    int       addIcon(HICON h);
    int       iconIndexFor(const wchar_t* category, const wchar_t* item) const;

    // state
    HWND       hParent{};
    HINSTANCE  hInst{};
    HWND       hHeader{};
    HWND       hTree{};
    HWND       hDivider{};
    HFONT      uiFont{};
    HFONT      headerFont{};
    bool       dark{ false };
    HIMAGELIST hImgList{};               // image list for tree icons
    std::wstring selCat_{}, selItem_{};
};

extern CraftingPanel gCraft;
