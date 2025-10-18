#pragma once
#include <windows.h>
#include <string>

struct SkillsPanel {
    void create(HWND parent, HINSTANCE hInst);
    void layout(int clientW, int clientH);
    void load();
    void save();

    HBRUSH onCtlColorStatic(HDC hdc, bool darkMode);
    HBRUSH onCtlColorEdit(HDC hdc, bool darkMode);

private:
    static constexpr int kNumSkills = 20;
    static constexpr int kPanelWidth = 420; // a bit wider for long names
    static constexpr int kRowHeight = 26;
    static constexpr int kPadY = 6;
    static constexpr int kIconSize = 20;
    static constexpr int kMargin = 10;

    struct Row { HWND hIcon{}, hName{}, hLbl{}, hEdit{}; };
    Row rows[kNumSkills]{};

    HWND      hParent{};
    HINSTANCE hInst{};
    HFONT     uiFont{};
    HFONT     headerFont{};
    HWND      hDivider{};
    HWND      hHeader{};

    std::wstring iniPath() const;
};
extern SkillsPanel gSkills;
