#pragma once
#include <windows.h>
#include <string>
#include "CalcBOM.h"

struct CenterPanel {
    void create(HWND parent, HINSTANCE hInst);
    void layout(int clientW, int clientH);
    void setRecipe(const std::wstring& name); // called when the user selects a leaf
    void setDark(bool darkMode);              // we'll implement theme + brush here
    HWND getQtyEdit()   const { return hQtyEdit; }
    HWND getQtyUpDown() const { return hQtyUpDown; }
    HWND getQtyUpBtn()  const { return hQtyUpBtn; }   // fallback buttons
    HWND getQtyDownBtn()const { return hQtyDownBtn; }
    static constexpr int kMargin = 8;
    int  qty() const;

    // allow parent to set qty + force refresh
    void setQty(int q);
    void rebuildNow();

    // NEW: color handlers the parent WndProc will call
    HBRUSH onCtlColorStatic(HDC hdc, HWND hCtrl);
    HBRUSH onCtlColorEdit(HDC hdc, HWND hCtrl);

private:
    HWND hParent{}, hHeader{}, hQtyEdit{}, hQtyUpDown{}, hList{};
    HWND hQtyLabel{};   // "Quantity:" STATIC
    HWND hQtyUpBtn{};   // fallback up button
    HWND hQtyDownBtn{}; // fallback down button
    HFONT uiFont{}, headerFont{};

    std::wstring currentRecipe;

    // THEME CACHE + BRUSH
    bool     _dark = false;
    COLORREF _bg = RGB(255, 255, 255);
    COLORREF _fg = RGB(0, 0, 0);
    HBRUSH   _bgBrush = nullptr;

    void rebuildList();
    void initFonts();

    // helper to (re)create the brush safely
    void recreateBrush();
};

extern CenterPanel gCenter;