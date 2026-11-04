#pragma once
#include <windows.h>
#include <string>
#include "imgui/imgui.h"

namespace UiTheme {
    inline void Apply() {
        ImGui::StyleColorsDark();
        auto& s = ImGui::GetStyle();
        s.WindowPadding = {20, 18};
        s.FramePadding = {10, 7};
        s.ItemSpacing = {12, 10};
        s.ItemInnerSpacing = {8, 6};
        s.WindowRounding = 8;
        s.ChildRounding = 4;
        s.FrameRounding = 4;
        s.PopupRounding = 4;
        s.GrabRounding = 3;
        s.WindowBorderSize = 1;
        s.FrameBorderSize = 0;
        s.ScrollbarSize = 12;
        auto* c = s.Colors;
        c[ImGuiCol_Text] = {0.93f, 0.94f, 0.96f, 1};
        c[ImGuiCol_TextDisabled] = {0.60f, 0.64f, 0.70f, 1};
        c[ImGuiCol_WindowBg] = {0.075f, 0.085f, 0.105f, 1};
        c[ImGuiCol_ChildBg] = {0, 0, 0, 0};
        c[ImGuiCol_PopupBg] = {0.11f, 0.125f, 0.15f, 1};
        c[ImGuiCol_Border] = {0.20f, 0.23f, 0.28f, 1};
        c[ImGuiCol_Separator] = c[ImGuiCol_Border];
        c[ImGuiCol_TitleBg] = {0.10f, 0.12f, 0.15f, 1};
        c[ImGuiCol_TitleBgActive] = c[ImGuiCol_TitleBg];
        c[ImGuiCol_FrameBg] = {0.14f, 0.16f, 0.20f, 1};
        c[ImGuiCol_FrameBgHovered] = {0.20f, 0.24f, 0.30f, 1};
        c[ImGuiCol_FrameBgActive] = {0.22f, 0.28f, 0.36f, 1};
        c[ImGuiCol_Button] = {0.17f, 0.31f, 0.54f, 1};
        c[ImGuiCol_ButtonHovered] = {0.20f, 0.39f, 0.68f, 1};
        c[ImGuiCol_ButtonActive] = {0.16f, 0.32f, 0.58f, 1};
        c[ImGuiCol_Header] = {0.16f, 0.24f, 0.36f, 1};
        c[ImGuiCol_HeaderHovered] = {0.20f, 0.29f, 0.42f, 1};
        c[ImGuiCol_HeaderActive] = {0.23f, 0.34f, 0.49f, 1};
        c[ImGuiCol_CheckMark] = {0.45f, 0.70f, 1.0f, 1};
        c[ImGuiCol_SliderGrab] = c[ImGuiCol_CheckMark];
        c[ImGuiCol_SliderGrabActive] = {0.65f, 0.82f, 1.0f, 1};
        c[ImGuiCol_TableHeaderBg] = {0.12f, 0.14f, 0.18f, 1};
        c[ImGuiCol_TableRowBgAlt] = {1, 1, 1, 0.025f};
    }

    inline void Font(float size = 18) {
        wchar_t windows[MAX_PATH]{};
        if (!GetWindowsDirectoryW(windows, MAX_PATH)) return;
        std::wstring path = std::wstring(windows) + L"\\Fonts\\segoeui.ttf";
        int count = WideCharToMultiByte(CP_UTF8, 0, path.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (!count) return;
        std::string utf8(count, '\0');
        WideCharToMultiByte(CP_UTF8, 0, path.c_str(), -1, utf8.data(), count, nullptr, nullptr);
        if (GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES)
            ImGui::GetIO().Fonts->AddFontFromFileTTF(utf8.c_str(), size, nullptr, ImGui::GetIO().Fonts->GetGlyphRangesVietnamese());
    }
}
