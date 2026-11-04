// C++ / Internal.h / Native / x64 / Windows 10/11 / 34Cheat Internal Core

#pragma once
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <string>
#include <vector>
#include <atomic>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx11.h"
#include "minhook/MinHook.h"
#include "Offsets.h"

struct Vector2 {
    float x, y;
};

struct Vector3 {
    float x, y, z;
};

struct InternalPlayer {
    std::string name;
    RoleType role;
    std::string roleName;
    TeamClassification team;
    bool teamKnown;
    bool isGhost;
    bool isLocal;
    Vector3 worldPos;
    Vector2 screenPos;
    float headScreenY;
    float feetScreenY;
    float boxWidth;
    float boxHeight;
    bool visible;
};

namespace InternalCheat {
    extern std::atomic_bool g_Running;
    extern bool g_ShowMenu;

    // ESP Settings
    extern bool g_EnableESP;
    extern bool g_ShowBox;
    extern bool g_ShowLine;
    extern bool g_ShowName;
    extern bool g_ShowRole;
    extern bool g_ShowGhosts;
    extern bool g_ShowLocalPlayer;
    extern float g_LineThickness;
    extern float g_BoxPadding;

    // Geometry & Calibration
    extern float g_HeadHeight;
    extern float g_FeetDepth;
    extern float g_BoxWidthScale;
    extern int g_SnaplineMode; // 0 = Feet, 1 = Center, 2 = Head

    // Team Colors
    extern ImVec4 g_DuckColor;
    extern ImVec4 g_GooseColor;
    extern ImVec4 g_NeutralColor;
    extern ImVec4 g_GhostColor;

    // Vision
    extern bool g_UnlockSeeGhosts;
    extern bool g_RemoveFog;
    extern bool g_XRayVision;

    void Start(HMODULE hMod);
    void Shutdown();
}
