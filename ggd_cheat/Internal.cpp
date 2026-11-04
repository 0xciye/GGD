// C++ / Internal.cpp / Native / x64 / Windows 10/11 / 34Cheat Internal Core

#include "Internal.h"
#include "InternalRoles.h"
#include "UiTheme.h"
#include "SupportedGame.h"
#include <cstdio>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cfloat>
#include <mutex>
#include <unordered_map>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace InternalCheat {
    std::atomic_bool g_Running{true};
    bool g_ShowMenu = true;

    // ESP Settings
    bool g_EnableESP = true;
    bool g_ShowBox = true;
    bool g_ShowLine = true;
    bool g_ShowName = true;
    bool g_ShowRole = true;
    bool g_ShowGhosts = true;
    bool g_ShowLocalPlayer = false;
    static bool g_ShowEventFeed = true;
    static bool g_CornerBoxes = true;
    static float g_LabelScale = 1.0f, g_LineOpacity = 0.35f, g_FeedSeconds = 5.0f;
    static int g_FeedLimit = 5;
    float g_LineThickness = 1.5f;
    float g_BoxPadding = 2.0f;

    // Geometry & Auto-Fit Calibration (Exact model proportions — zero manual tuning needed)
    float g_HeadHeight = 0.82f;        // Top of head / hat world height above pivot
    float g_FeetDepth = 0.90f;         // Bottom of feet world depth below pivot (matches SHADOW_OFFSET_Y = -0.8982)
    float g_BoxWidthScale = 0.92f;     // Total width in world units
    int g_SnaplineMode = 0;            // 0 = Feet, 1 = Center, 2 = Head

    // Team Colors
    ImVec4 g_DuckColor = ImVec4(0.96f, 0.22f, 0.22f, 1.0f);
    ImVec4 g_GooseColor = ImVec4(0.24f, 0.92f, 0.43f, 1.0f);
    ImVec4 g_NeutralColor = ImVec4(1.0f, 0.76f, 0.1f, 1.0f);
    ImVec4 g_GhostColor = ImVec4(0.65f, 0.65f, 0.65f, 0.85f);

    // Exploits, Vision & Camera
    bool g_UnlockSeeGhosts = true;
    bool g_RemoveFog = true;
    bool g_XRayVision = true;
    static bool g_DroneView = false, g_SpeedEnabled = false, g_NoClip = false, g_NoCooldown = false;
    static float g_ViewScale = 2.0f, g_SpeedScale = 1.5f;

    struct BoolSetting { const wchar_t* key; bool* value; };
    struct FloatSetting { const wchar_t* key; float* value; float minimum, maximum; };
    static const BoolSetting kBoolSettings[] = {
        {L"ESP", &g_EnableESP}, {L"Boxes", &g_ShowBox}, {L"Lines", &g_ShowLine},
        {L"Names", &g_ShowName}, {L"Roles", &g_ShowRole}, {L"Ghosts", &g_ShowGhosts},
        {L"LocalPlayer", &g_ShowLocalPlayer}, {L"EventFeed", &g_ShowEventFeed}, {L"CornerBoxes", &g_CornerBoxes},
        {L"SeeGhosts", &g_UnlockSeeGhosts}, {L"RemoveFog", &g_RemoveFog}, {L"XRay", &g_XRayVision},
        {L"Drone", &g_DroneView}, {L"Speed", &g_SpeedEnabled}, {L"NoClip", &g_NoClip}, {L"NoCooldown", &g_NoCooldown}
    };
    static const FloatSetting kFloatSettings[] = {
        {L"LineWidth", &g_LineThickness, 1, 4}, {L"BoxPadding", &g_BoxPadding, 0, 10},
        {L"LabelScale", &g_LabelScale, 0.8f, 1.5f}, {L"LineOpacity", &g_LineOpacity, 0.05f, 1},
        {L"FeedSeconds", &g_FeedSeconds, 3, 15}, {L"ViewScale", &g_ViewScale, 1, 4}, {L"SpeedScale", &g_SpeedScale, 1, 10},
        {L"DuckR", &g_DuckColor.x, 0, 1}, {L"DuckG", &g_DuckColor.y, 0, 1}, {L"DuckB", &g_DuckColor.z, 0, 1}, {L"DuckA", &g_DuckColor.w, 0, 1},
        {L"GooseR", &g_GooseColor.x, 0, 1}, {L"GooseG", &g_GooseColor.y, 0, 1}, {L"GooseB", &g_GooseColor.z, 0, 1}, {L"GooseA", &g_GooseColor.w, 0, 1},
        {L"NeutralR", &g_NeutralColor.x, 0, 1}, {L"NeutralG", &g_NeutralColor.y, 0, 1}, {L"NeutralB", &g_NeutralColor.z, 0, 1}, {L"NeutralA", &g_NeutralColor.w, 0, 1},
        {L"GhostR", &g_GhostColor.x, 0, 1}, {L"GhostG", &g_GhostColor.y, 0, 1}, {L"GhostB", &g_GhostColor.z, 0, 1}, {L"GhostA", &g_GhostColor.w, 0, 1}
    };
    static const char* g_ConfigStatus = "Auto-save when menu closes";
    static std::wstring ConfigPath() {
        wchar_t root[32768]{};
        DWORD n = GetEnvironmentVariableW(L"LOCALAPPDATA", root, 32768);
        if (!n || n >= 32768) return {};
        std::wstring folder = std::wstring(root) + L"\\34Cheat";
        CreateDirectoryW(folder.c_str(), nullptr);
        return folder + L"\\config.ini";
    }
    static bool SaveConfig(const std::wstring& path) {
        if (path.empty()) return false;
        std::wstring content = L"\xFEFF[34Cheat]\r\nVersion=1\r\n";
        for (const auto& setting : kBoolSettings)
            content += std::wstring(setting.key) + L"=" + (*setting.value ? L"1" : L"0") + L"\r\n";
        for (const auto& setting : kFloatSettings)
            content += std::wstring(setting.key) + L"=" + std::to_wstring(*setting.value) + L"\r\n";
        content += L"LineAnchor=" + std::to_wstring(g_SnaplineMode) + L"\r\nFeedLimit=" + std::to_wstring(g_FeedLimit) + L"\r\n";
        const auto temporary = path + L"." + std::to_wstring(GetCurrentProcessId()) + L".tmp";
        HANDLE file = CreateFileW(temporary.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE) return false;
        DWORD written = 0, bytes = static_cast<DWORD>(content.size() * sizeof(wchar_t));
        bool saved = WriteFile(file, content.data(), bytes, &written, nullptr) && written == bytes && FlushFileBuffers(file);
        CloseHandle(file);
        // Replace only after the complete config is durable, preserving the previous file on failure.
        saved = saved && MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
        if (!saved) DeleteFileW(temporary.c_str());
        WritePrivateProfileStringW(nullptr, nullptr, nullptr, path.c_str());
        return saved;
    }
    static bool ConfigNumber(const std::wstring& path, const wchar_t* key, double* number) {
        wchar_t value[96]{};
        GetPrivateProfileStringW(L"34Cheat", key, L"", value, 96, path.c_str());
        if (!value[0]) return false;
        wchar_t* end = nullptr;
        double parsed = wcstod(value, &end);
        if (end == value || *end || !std::isfinite(parsed)) return false;
        *number = parsed; return true;
    }
    static bool LoadConfig(const std::wstring& path) {
        if (path.empty() || GetPrivateProfileIntW(L"34Cheat", L"Version", 0, path.c_str()) != 1) return false;
        double number = 0;
        for (const auto& setting : kBoolSettings)
            if (ConfigNumber(path, setting.key, &number) && (number == 0 || number == 1)) *setting.value = number == 1;
        for (const auto& setting : kFloatSettings)
            if (ConfigNumber(path, setting.key, &number)) *setting.value = static_cast<float>((std::max)(double(setting.minimum), (std::min)(double(setting.maximum), number)));
        if (ConfigNumber(path, L"LineAnchor", &number)) g_SnaplineMode = static_cast<int>((std::max)(0.0, (std::min)(2.0, number)));
        if (ConfigNumber(path, L"FeedLimit", &number)) g_FeedLimit = static_cast<int>((std::max)(3.0, (std::min)(10.0, number)));
        return true;
    }

    static HMODULE g_hModule = nullptr;
    static HANDLE g_ModuleMarker = nullptr;
    static bool g_RenderReported = false;
    static HWND g_GameHwnd = nullptr;
    static WNDPROC g_oWndProc = nullptr;

    static ID3D11Device* g_pDevice = nullptr;
    static ID3D11DeviceContext* g_pContext = nullptr;
    static ID3D11RenderTargetView* g_pRenderTargetView = nullptr;

    typedef HRESULT (__stdcall* PresentFn)(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
    typedef HRESULT (__stdcall* ResizeBuffersFn)(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);

    static PresentFn g_oPresent = nullptr;
    static ResizeBuffersFn g_oResizeBuffers = nullptr;

    static bool g_D3DInitialized = false;
    static bool g_Win32Initialized = false;
    static bool g_DX11Initialized = false;
    static bool g_CameraReady = false;
    static ImGuiContext* g_ImGuiContext = nullptr;
    static IDXGISwapChain* g_SwapChain = nullptr;
    static std::atomic<DWORD> g_GameThread{0};
    static std::atomic<uintptr_t> g_LocalObject{0};
    static bool g_LocalUpdateReported = false, g_CameraReported = false;
    static bool IsGameThread() { return g_GameThread.load() == GetCurrentThreadId(); }

    static void RuntimeLog(const wchar_t* message) {
        wchar_t root[32768]{};
        DWORD length = GetEnvironmentVariableW(L"LOCALAPPDATA", root, 32768);
        if (!length || length >= 32768) return;
        std::wstring directory = std::wstring(root) + L"\\34Cheat";
        CreateDirectoryW(directory.c_str(), nullptr);
        HANDLE file = CreateFileW((directory + L"\\runtime.log").c_str(), FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE) return;
        SYSTEMTIME time{}; GetSystemTime(&time);
        wchar_t line[512]{};
        swprintf_s(line, L"%04u-%02u-%02u %02u:%02u:%02u UTC  pid=%lu  %s\r\n", time.wYear, time.wMonth, time.wDay,
                   time.wHour, time.wMinute, time.wSecond, GetCurrentProcessId(), message);
        char utf8[2048]{};
        int bytes = WideCharToMultiByte(CP_UTF8, 0, line, -1, utf8, sizeof(utf8), nullptr, nullptr);
        DWORD written = 0;
        if (bytes > 1) WriteFile(file, utf8, bytes - 1, &written, nullptr);
        CloseHandle(file);
    }
    static void SignalSession(const wchar_t* state) {
        wchar_t name[96]{};
        swprintf_s(name, L"Local\\34Cheat.%s.%lu", state, GetCurrentProcessId());
        HANDLE event = OpenEventW(EVENT_MODIFY_STATE, FALSE, name);
        if (event) { SetEvent(event); CloseHandle(event); }
    }
    static DWORD RuntimeFailure(const wchar_t* step, long code = 0) {
        wchar_t message[240]{};
        swprintf_s(message, L"Failed: %s (code %ld)", step, code);
        RuntimeLog(message); SignalSession(L"Failed");
        return 1;
    }

    // Signatures match the IntPtr-based Unity bindings in this project's dump.
    struct Bounds { Vector3 center; Vector3 extents; };
    struct UnityBindings {
        void (*worldToScreen)(uintptr_t, const Vector3*, int, Vector3*) = nullptr;
        uintptr_t (*getTransform)(uintptr_t, const void*) = nullptr;
        void (*getPosition)(uintptr_t, Vector3*) = nullptr;
        int (*pixelWidth)(uintptr_t) = nullptr;
        int (*pixelHeight)(uintptr_t) = nullptr;
        void (*getBounds)(uintptr_t, Bounds*) = nullptr;
        bool (*rendererEnabled)(uintptr_t) = nullptr;
        void (*setRendererEnabled)(uintptr_t, bool) = nullptr;
        float (*cameraSize)(uintptr_t) = nullptr;
        void (*setCameraSize)(uintptr_t, float) = nullptr;
        bool (*isTrigger)(uintptr_t) = nullptr;
        void (*setTrigger)(uintptr_t, bool) = nullptr;
    };
    static UnityBindings g_Unity;

    static void ResolveUnityBindings(HMODULE module) {
        if (!IsGameThread()) return;
        using Resolve = void* (*)(const char*);
        auto resolve = reinterpret_cast<Resolve>(GetProcAddress(module, "il2cpp_resolve_icall"));
        if (!resolve) return;
        g_Unity.worldToScreen = reinterpret_cast<decltype(g_Unity.worldToScreen)>(resolve("UnityEngine.Camera::WorldToScreenPoint_Injected"));
        // Unity 6's injected getter returns a marshalled handle, not a native Transform.
        g_Unity.getTransform = reinterpret_cast<decltype(g_Unity.getTransform)>(reinterpret_cast<uintptr_t>(module) + 0x3B878F0);
        g_Unity.getPosition = reinterpret_cast<decltype(g_Unity.getPosition)>(resolve("UnityEngine.Transform::get_position_Injected"));
        g_Unity.pixelWidth = reinterpret_cast<decltype(g_Unity.pixelWidth)>(resolve("UnityEngine.Camera::get_pixelWidth_Injected"));
        g_Unity.pixelHeight = reinterpret_cast<decltype(g_Unity.pixelHeight)>(resolve("UnityEngine.Camera::get_pixelHeight_Injected"));
        g_Unity.getBounds = reinterpret_cast<decltype(g_Unity.getBounds)>(resolve("UnityEngine.Renderer::get_bounds_Injected"));
        g_Unity.rendererEnabled = reinterpret_cast<decltype(g_Unity.rendererEnabled)>(resolve("UnityEngine.Renderer::get_enabled_Injected"));
        g_Unity.setRendererEnabled = reinterpret_cast<decltype(g_Unity.setRendererEnabled)>(resolve("UnityEngine.Renderer::set_enabled_Injected"));
        g_Unity.cameraSize = reinterpret_cast<decltype(g_Unity.cameraSize)>(resolve("UnityEngine.Camera::get_orthographicSize_Injected"));
        g_Unity.setCameraSize = reinterpret_cast<decltype(g_Unity.setCameraSize)>(resolve("UnityEngine.Camera::set_orthographicSize_Injected"));
        g_Unity.isTrigger = reinterpret_cast<decltype(g_Unity.isTrigger)>(resolve("UnityEngine.Collider2D::get_isTrigger_Injected"));
        g_Unity.setTrigger = reinterpret_cast<decltype(g_Unity.setTrigger)>(resolve("UnityEngine.Collider2D::set_isTrigger_Injected"));
    }

    static bool IsFinite(const Vector3& p) {
        return std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z);
    }

    // -------------------------------------------------------------
    // Memory Safe Helper Functions
    // -------------------------------------------------------------
    template <typename T>
    static bool SafeRead(uintptr_t addr, T* outVal) {
        if (!addr || !outVal) return false;
        __try {
            *outVal = *(reinterpret_cast<const T*>(addr));
            return true;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    template <typename T>
    static T SafeReadVal(uintptr_t addr, T defaultVal = T{}) {
        T res = defaultVal;
        SafeRead(addr, &res);
        return res;
    }

    template <typename T>
    static bool SafeWrite(uintptr_t addr, T value) {
        if (!addr) return false;
        __try {
            *reinterpret_cast<T*>(addr) = value;
            return true;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    static bool ReadPosition(uintptr_t entity, Vector3* position) {
        if (!IsGameThread() || !entity) return false;
        uintptr_t native = SafeReadVal<uintptr_t>(entity + Offsets::Entity::m_CachedPtr);
        if (native && g_Unity.getTransform && g_Unity.getPosition) {
            __try {
                uintptr_t transform = g_Unity.getTransform(entity, nullptr);
                uintptr_t transformNative = transform ? SafeReadVal<uintptr_t>(transform + Offsets::Entity::m_CachedPtr) : 0;
                if (transformNative) {
                    g_Unity.getPosition(transformNative, position);
                    if (IsFinite(*position)) return true;
                }
            } __except (EXCEPTION_EXECUTE_HANDLER) {}
        }
        // PlayableEntity also has non-PlayerController subclasses: use its base field.
        return SafeRead(entity + Offsets::Entity::previousPosition, position) && IsFinite(*position);
    }

    static bool Project(uintptr_t camera, const Vector3& world, float w, float h,
                        float cameraW, float cameraH, Vector2* screen) {
        if (!IsGameThread() || !camera || !g_Unity.worldToScreen || cameraW <= 0 || cameraH <= 0 || !IsFinite(world))
            return false;
        Vector3 p{};
        __try {
            g_Unity.worldToScreen(camera, &world, 2, &p); // Mono
        } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
        if (!IsFinite(p) || p.z <= 0.0f) return false;
        screen->x = p.x * w / cameraW;
        screen->y = h - p.y * h / cameraH;
        return std::isfinite(screen->x) && std::isfinite(screen->y);
    }

    static bool ReadCameraSize(uintptr_t camera, float* w, float* h) {
        if (!IsGameThread() || !camera || !g_Unity.pixelWidth || !g_Unity.pixelHeight) return false;
        __try {
            *w = static_cast<float>(g_Unity.pixelWidth(camera));
            *h = static_cast<float>(g_Unity.pixelHeight(camera));
            return *w > 0 && *h > 0;
        } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
    }

    static bool ReadSpriteBounds(uintptr_t player, Bounds* bounds) {
        if (!IsGameThread() || !g_Unity.getBounds || !g_Unity.rendererEnabled) return false;
        constexpr uintptr_t spritesOffset = 0x210; // PlayableEntity.allPlayerSpriteComponentsArray
        uintptr_t array = SafeReadVal<uintptr_t>(player + spritesOffset);
        uintptr_t count = array ? SafeReadVal<uintptr_t>(array + 0x18) : 0;
        if (!count || count > 64) return false;
        Vector3 lo{}, hi{};
        bool found = false;
        for (uintptr_t i = 0; i < count; ++i) {
            uintptr_t renderer = SafeReadVal<uintptr_t>(array + 0x20 + i * sizeof(uintptr_t));
            uintptr_t native = renderer ? SafeReadVal<uintptr_t>(renderer + 0x10) : 0;
            if (!native) continue;
            Bounds b{};
            bool enabled = false;
            __try {
                enabled = g_Unity.rendererEnabled(native);
                if (enabled) g_Unity.getBounds(native, &b);
            } __except (EXCEPTION_EXECUTE_HANDLER) { continue; }
            if (!enabled || !IsFinite(b.center) || !IsFinite(b.extents) ||
                b.extents.x <= 0 || b.extents.y <= 0 || b.extents.z < 0 ||
                b.extents.x > 10 || b.extents.y > 10 || b.extents.z > 10) continue;
            Vector3 a{b.center.x - b.extents.x, b.center.y - b.extents.y, b.center.z - b.extents.z};
            Vector3 z{b.center.x + b.extents.x, b.center.y + b.extents.y, b.center.z + b.extents.z};
            if (!found) { lo = a; hi = z; found = true; }
            else {
                lo = { (std::min)(lo.x,a.x), (std::min)(lo.y,a.y), (std::min)(lo.z,a.z) };
                hi = { (std::max)(hi.x,z.x), (std::max)(hi.y,z.y), (std::max)(hi.z,z.z) };
            }
        }
        if (found) {
            bounds->center = {(lo.x+hi.x)*0.5f, (lo.y+hi.y)*0.5f, (lo.z+hi.z)*0.5f};
            bounds->extents = {(hi.x-lo.x)*0.5f, (hi.y-lo.y)*0.5f, (hi.z-lo.z)*0.5f};
        }
        return found;
    }

    static void CollectPlayers(uintptr_t dict, std::vector<uintptr_t>& players) {
        if (!dict) return;
        uintptr_t entries = SafeReadVal<uintptr_t>(dict + Offsets::Dictionary::Entries);
        int32_t count = SafeReadVal<int32_t>(dict + Offsets::Dictionary::Count);
        uintptr_t capacity = entries ? SafeReadVal<uintptr_t>(entries + Offsets::Dictionary::ArrayLength) : 0;
        if (count <= 0 || count > 4096 || static_cast<uintptr_t>(count) > capacity) return;
        for (int32_t i = 0; i < count; ++i) {
            uintptr_t entry = entries + Offsets::Dictionary::ArrayData + i * Offsets::Dictionary::EntrySize;
            int32_t hash = -1;
            if (!SafeRead(entry, &hash) || hash < 0) continue;
            uintptr_t player = SafeReadVal<uintptr_t>(entry + Offsets::Dictionary::EntryValue);
            if (player && std::find(players.begin(), players.end(), player) == players.end())
                players.push_back(player);
        }
    }

    struct FieldOverride {
        uintptr_t object, klass, native, offset;
        uint32_t original, applied;
        size_t size;
        bool touched;
    };
    static std::vector<FieldOverride> g_Overrides;

    static bool SameObject(const FieldOverride& f) {
        return SafeReadVal<uintptr_t>(f.object) == f.klass &&
               SafeReadVal<uintptr_t>(f.object + 0x10) == f.native;
    }

    template <typename T>
    static void OverrideValue(uintptr_t object, uintptr_t offset, T value) {
        static_assert(sizeof(T) == 1 || sizeof(T) == 4);
        if (!object) return;
        T old{};
        if (!SafeRead(object + offset, &old)) return;
        auto it = std::find_if(g_Overrides.begin(), g_Overrides.end(), [&](const FieldOverride& f) {
            return f.object == object && f.offset == offset;
        });
        if (it != g_Overrides.end() && !SameObject(*it)) {
            g_Overrides.erase(it);
            it = g_Overrides.end();
        }
        if (it == g_Overrides.end()) {
            FieldOverride f{object, SafeReadVal<uintptr_t>(object), SafeReadVal<uintptr_t>(object + 0x10), offset, 0, 0, sizeof(T), true};
            memcpy(&f.original, &old, sizeof(T));
            g_Overrides.push_back(f);
            it = g_Overrides.end() - 1;
        } else if (memcmp(&old, &it->applied, sizeof(T)) != 0) {
            // Retain the latest game-owned value if gameplay changed it between frames.
            memcpy(&it->original, &old, sizeof(T));
        }
        it->touched = true;
        if (SafeWrite(object + offset, value)) memcpy(&it->applied, &value, sizeof(T));
    }

    static void RestoreUntouchedOverrides() {
        for (auto it = g_Overrides.begin(); it != g_Overrides.end();) {
            if (it->touched) { ++it; continue; }
            uint32_t current = 0;
            bool read = it->size == 1 ? SafeRead(it->object + it->offset, reinterpret_cast<uint8_t*>(&current)) : SafeRead(it->object + it->offset, &current);
            if (SameObject(*it) && read && current == it->applied) {
                if (it->size == 1) SafeWrite(it->object + it->offset, static_cast<uint8_t>(it->original));
                else SafeWrite(it->object + it->offset, it->original);
            }
            it = g_Overrides.erase(it);
        }
    }

    static uintptr_t g_FogObject = 0, g_FogNative = 0, g_FogClass = 0;
    static bool g_FogWasDisabled = false;
    static void (*g_DisableFog)(uintptr_t, const void*) = nullptr;
    static void (*g_EnableFog)(uintptr_t, const void*) = nullptr;

    static void ApplyFog(uintptr_t fog) {
        if (!IsGameThread() || !g_DisableFog || !g_EnableFog) return;
        __try {
            if (fog != g_FogObject || (fog && SafeReadVal<uintptr_t>(fog + 0x10) != g_FogNative)) {
                if (g_FogObject && !g_FogWasDisabled &&
                    SafeReadVal<uintptr_t>(g_FogObject) == g_FogClass &&
                    SafeReadVal<uintptr_t>(g_FogObject + 0x10) == g_FogNative)
                    g_EnableFog(g_FogObject, nullptr);
                g_FogObject = g_FogNative = g_FogClass = 0;
            }
            if (!fog || !SafeReadVal<uintptr_t>(fog + 0x10)) return;
            if (!g_FogObject) {
                g_FogObject = fog; g_FogNative = SafeReadVal<uintptr_t>(fog + 0x10);
                g_FogClass = SafeReadVal<uintptr_t>(fog);
                g_FogWasDisabled = SafeReadVal<bool>(fog + Offsets::FogOfWarLayout::disabled);
            }
            // FogOfWar.Disable updates its shader/mesh state; hiding its renderer hides sprites too.
            if (!SafeReadVal<bool>(fog + Offsets::FogOfWarLayout::disabled)) g_DisableFog(fog, nullptr);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            g_FogObject = g_FogNative = g_FogClass = 0;
            RuntimeLog(L"Fog transition failed; override released");
        }
    }

    static bool SafeReadChars(uintptr_t strAddr, wchar_t* outBuf, int maxChars, int* outLen) {
        if (!strAddr || !outBuf || !outLen) return false;
        __try {
            int32_t len = *reinterpret_cast<const int32_t*>(strAddr + 0x10);
            if (len <= 0 || len >= maxChars) return false;
            const wchar_t* chars = reinterpret_cast<const wchar_t*>(strAddr + 0x14);
            for (int i = 0; i < len; ++i) {
                outBuf[i] = chars[i];
            }
            outBuf[len] = L'\0';
            *outLen = len;
            return true;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    static std::string ReadIl2CppString(uintptr_t strAddr) {
        wchar_t wbuf[128] = { 0 };
        int len = 0;
        if (!SafeReadChars(strAddr, wbuf, 128, &len)) return "";
        int utf8Len = WideCharToMultiByte(CP_UTF8, 0, wbuf, len, nullptr, 0, nullptr, nullptr);
        if (utf8Len <= 0) return "";

        std::string s(utf8Len, '\0');
        WideCharToMultiByte(CP_UTF8, 0, wbuf, len, &s[0], utf8Len, nullptr, nullptr);
        return s;
    }

    struct GameRequest {
        bool menuOpen = false;
        bool seeGhosts = true, removeFog = true, xray = true;
        bool drone = false, speed = false, noclip = false, noCooldown = false;
        float viewScale = 2.0f, speedScale = 1.5f;
    };
    struct FrameSnapshot {
        std::vector<InternalPlayer> players;
        bool cameraReady = false;
        ULONGLONG sampledAt = 0;
    };
    static GameRequest g_Request;
    static std::mutex g_RequestMutex, g_FrameMutex;
    static std::recursive_mutex g_UiMutex;
    static FrameSnapshot g_Frame;
    static std::atomic<float> g_FrameWidth{0}, g_FrameHeight{0};
    using CameraCallback = void (*)(uintptr_t, uintptr_t, const void*);
    using LocalUpdate = void (*)(uintptr_t, const void*);
    static CameraCallback g_OriginalCamera = nullptr;
    static LocalUpdate g_OriginalPreCull = nullptr;
    static uintptr_t g_RenderCamera = 0;
    static LocalUpdate g_OriginalUpdate = nullptr;
    struct alignas(4) GameFloat { uint8_t bytes[24]{}; };
    static_assert(sizeof(GameFloat) == 24);
    static LocalUpdate g_OriginalSpeedUpdate = nullptr;
    static float (*g_ReadGameFloat)(GameFloat, const void*) = nullptr;
    static GameFloat (*g_MakeGameFloat)(float, const void*) = nullptr;
    static uintptr_t g_SpeedLocal = 0;
    static std::atomic<float> g_AppliedSpeedScale{1};
    static LocalUpdate g_OriginalCooldownUpdate = nullptr;
    static void (*g_ReduceCooldown)(uintptr_t, float, const void*) = nullptr;
    static std::atomic<bool> g_GameHooksReady{false};
    static void CollectFrame(const GameRequest& request);
    static void RefreshSpeed(uintptr_t local, const GameRequest& request);
    static void RestoreSpeed();

    struct LocalContext { uintptr_t local = 0, fields = 0, player = 0, camera = 0; };
    static bool ReadLocalContext(uintptr_t local, LocalContext* context) {
        *context = {};
        if (!local || !SafeReadVal<uintptr_t>(local + 0x10)) return false;
        uintptr_t klass = SafeReadVal<uintptr_t>(local);
        if (!klass || (klass & 7)) return false;
        uintptr_t fields = SafeReadVal<uintptr_t>(klass + Offsets::Il2CppClass_StaticFields);
        if (!fields || SafeReadVal<uintptr_t>(fields + Offsets::LocalPlayer::Instance) != local) return false;
        uintptr_t player = SafeReadVal<uintptr_t>(fields + Offsets::LocalPlayer::Player);
        uintptr_t camera = SafeReadVal<uintptr_t>(local + Offsets::LocalPlayer::mainCamera);
        if (!player || !SafeReadVal<uintptr_t>(player + 0x10) ||
            !SafeReadVal<bool>(player + Offsets::Entity::isLocal) ||
            !camera || !SafeReadVal<uintptr_t>(camera + 0x10)) return false;
        *context = {local, fields, player, camera};
        return true;
    }

    static GameRequest RequestedFeatures() {
        std::lock_guard<std::mutex> lock(g_RequestMutex);
        return g_Request;
    }

    using SpectatorCallback = void (*)(uintptr_t, uintptr_t, uintptr_t, const void*);
    using RoundStartCallback = void (*)(uintptr_t, bool, const void*);
    using RevealRoleCallback = void (*)(uintptr_t, int16_t, const void*);
    static RevealRoleCallback g_OriginalRevealRole = nullptr;
    using DeathCallback = void (*)(uintptr_t, bool, uintptr_t, uintptr_t, const void*);
    using ReportCallback = void (*)(uintptr_t, uintptr_t, uintptr_t, const void*);
    static DeathCallback g_OriginalDeath = nullptr;
    static ReportCallback g_OriginalReport = nullptr;
    static SpectatorCallback g_OriginalSpectatorInfo = nullptr;
    static RoundStartCallback g_OriginalRoundStart = nullptr;
    static std::mutex g_RoleMutex;
    static std::unordered_map<std::string, RoleType> g_SpectatorRoles;
    static std::unordered_map<std::string, RoleType> g_RevealedRoles;
    static uintptr_t g_RoleOwner = 0;
    static ULONGLONG g_RolesReceivedAt = 0;

    enum class EventKind { Death, Report, Meeting, Player, Round, Sabotage, Role, Vote, Task };
    struct FeedEvent { std::string message; EventKind kind; ULONGLONG at; std::string identity; };
    static std::mutex g_EventMutex;
    static std::vector<FeedEvent> g_Events;
    static void ClearEvents() {
        std::lock_guard<std::mutex> lock(g_EventMutex);
        g_Events.clear();
    }
    static void PushEvent(std::string message, EventKind kind, ULONGLONG now = GetTickCount64(), std::string identity = {}) {
        std::lock_guard<std::mutex> lock(g_EventMutex);
        for (const auto& event : g_Events)
            if (event.kind == kind && event.message == message && event.identity == identity && now - event.at < 300) return;
        if (g_Events.size() == 100) g_Events.erase(g_Events.begin());
        g_Events.push_back({std::move(message), kind, now, std::move(identity)});
    }
    static std::vector<FeedEvent> ActiveEvents(ULONGLONG now = GetTickCount64()) {
        std::lock_guard<std::mutex> lock(g_EventMutex);
        std::vector<FeedEvent> active;
        for (const auto& event : g_Events)
            if (now - event.at < static_cast<ULONGLONG>(g_FeedSeconds * 1000)) active.push_back(event);
        if (active.size() > static_cast<size_t>(g_FeedLimit)) active.erase(active.begin(), active.end() - g_FeedLimit);
        return active;
    }
    static std::vector<FeedEvent> EventHistory() {
        std::lock_guard<std::mutex> lock(g_EventMutex);
        return g_Events;
    }
    static std::string EventPlayerName(uintptr_t player) {
        auto name = player ? ReadIl2CppString(SafeReadVal<uintptr_t>(player + Offsets::Entity::nickname)) : "";
        // Player-controlled line breaks must not create fake feed entries.
        for (char& c : name) if (static_cast<unsigned char>(c) < 32) c = ' ';
        return name.empty() ? "Player" : name;
    }
    static std::string EventNameById(const std::string& id) {
        if (id.empty()) return "Player";
        uintptr_t module = reinterpret_cast<uintptr_t>(GetModuleHandleW(L"GameAssembly.dll"));
        std::vector<uintptr_t> players;
        for (uintptr_t offset : {Offsets::PlayerController_TypeInfo, Offsets::PlayableEntity_TypeInfo}) {
            uintptr_t klass = module ? SafeReadVal<uintptr_t>(module + offset) : 0;
            uintptr_t fields = klass ? SafeReadVal<uintptr_t>(klass + Offsets::Il2CppClass_StaticFields) : 0;
            uintptr_t dictionaryOffset = offset == Offsets::PlayerController_TypeInfo ? 0 : 0x28;
            if (fields) CollectPlayers(SafeReadVal<uintptr_t>(fields + dictionaryOffset), players);
        }
        for (uintptr_t player : players)
            if (ReadIl2CppString(SafeReadVal<uintptr_t>(player + 0x20)) == id) return EventPlayerName(player);
        return "Player";
    }
    static const char* DeathReasonName(uint8_t reason) {
        // EDeathReason is a byte enum in this supported game build.
        static constexpr const char* names[] = {
            "cause unknown", "normal kill", "explosion", "Lover linked death", "Cupid linked death",
            "Carrier/Parasite linked death", "killed inside Pelican", "voted out", "assassination",
            "dream kill", "helium", "Thrall latch", "space exposure", "teleporter",
            "suffocated in shuttle", "crushed by shuttle", "crushed by chandelier", "fell off bridge",
            "crushed by boulder", "mummy", "locust swarm", "demon", "Crow self-destruction",
            "Sin Eater sacrifice", "bled out", "outside safe zone", "hit by bus", "crushed by rollercoaster",
            "Raven self-destruction", "fell from rollercoaster", "furnace", "closet", "lake",
            "tiger", "sinkhole", "Godzilla", "Hawk kill", "Cuckoo kill"
        };
        return reason < sizeof(names) / sizeof(names[0]) ? names[reason] : names[0];
    }
    static std::string DeathMessage(const std::string& victim, const std::string& killer, bool killerKnown, uint8_t reason) {
        const bool linkedOrVote = reason == 3 || reason == 4 || reason == 5 || reason == 7 || reason == 22 || reason == 23 || reason == 28;
        auto message = killerKnown && !killer.empty() && !linkedOrVote ? killer + " killed " + victim : victim + " died";
        return message + " — " + DeathReasonName(reason);
    }
    static void OnPlayerDeath(uintptr_t manager, bool killerKnown, uintptr_t killer, uintptr_t killed, const void* method) {
        auto victim = EventPlayerName(killed);
        // All inspected normal/special death handlers set this before emitting OnPlayerDeathEvent.
        const auto reason = killed ? SafeReadVal<uint8_t>(killed + 0xD8) : uint8_t(0);
        auto identity = killed ? ReadIl2CppString(SafeReadVal<uintptr_t>(killed + 0x20)) : "";
        if (identity.empty()) identity = std::to_string(killed);
        auto message = DeathMessage(victim, killer ? EventPlayerName(killer) : "", killerKnown && killer && killer != killed, reason);
        g_OriginalDeath(manager, killerKnown, killer, killed, method);
        if (g_Running && killed) PushEvent(std::move(message), EventKind::Death, GetTickCount64(), std::move(identity));
    }
    static void OnReport(uintptr_t manager, uintptr_t reporter, uintptr_t body, const void* method) {
        auto reporterId = ReadIl2CppString(reporter);
        auto bodyId = ReadIl2CppString(body);
        auto actor = EventNameById(reporterId);
        auto message = bodyId.empty() ? actor + " called a meeting" : actor + " reported " + EventNameById(bodyId);
        g_OriginalReport(manager, reporter, body, method);
        if (g_Running) PushEvent(std::move(message), bodyId.empty() ? EventKind::Meeting : EventKind::Report);
    }

    using SimpleCallback = void (*)(uintptr_t, const void*);
    using PlayerCallback = void (*)(uintptr_t, uintptr_t, const void*);
    static SimpleCallback g_OriginalSimpleEvents[4]{};
    static PlayerCallback g_OriginalPlayerEvents[3]{}, g_OriginalSabotageEvents[2]{};
    static ReportCallback g_OriginalEaten = nullptr, g_OriginalVote = nullptr;
    template<int Event> static void OnSimpleEvent(uintptr_t manager, const void* method) {
        g_OriginalSimpleEvents[Event](manager, method);
        const char* messages[] = {"Match started", "Match ended", "Meeting started", "Meeting ended"};
        if (g_Running) PushEvent(messages[Event], Event < 2 ? EventKind::Round : EventKind::Meeting);
    }
    template<int Event> static void OnPlayerEvent(uintptr_t manager, uintptr_t player, const void* method) {
        const auto name = EventPlayerName(player);
        g_OriginalPlayerEvents[Event](manager, player, method);
        const char* actions[] = {" joined the room", " left the room", " respawned"};
        if (g_Running && player) PushEvent(name + actions[Event], EventKind::Player);
    }
    template<int Event> static void OnSabotageEvent(uintptr_t manager, uintptr_t id, const void* method) {
        g_OriginalSabotageEvents[Event](manager, id, method);
        if (g_Running) PushEvent(Event == 0 ? "Sabotage started" : "Sabotage ended", EventKind::Sabotage);
    }
    static void OnEaten(uintptr_t manager, uintptr_t eater, uintptr_t eaten, const void* method) {
        const auto message = EventPlayerName(eater) + " swallowed " + EventPlayerName(eaten);
        g_OriginalEaten(manager, eater, eaten, method);
        if (g_Running && eaten) PushEvent(message, EventKind::Player);
    }
    static void OnVote(uintptr_t manager, uintptr_t voter, uintptr_t voted, const void* method) {
        const auto name = EventNameById(ReadIl2CppString(voter));
        g_OriginalVote(manager, voter, voted, method);
        if (g_Running && voter) PushEvent(name + " voted", EventKind::Vote);
    }

    static bool KnownRole(RoleType role) {
        return role != RoleType::None && strcmp(GetInternalRoleName(role), "Unknown") != 0;
    }
    static RoleType EntityRole(uintptr_t player) {
        if (!player || SafeReadVal<uint8_t>(player + 0xA8) != 1) return RoleType::None;
        uintptr_t object = SafeReadVal<uintptr_t>(player + Offsets::Entity::playerRole);
        auto role = object ? static_cast<RoleType>(SafeReadVal<int16_t>(object + Offsets::GGDRole::RoleTypeOffset)) : RoleType::None;
        return KnownRole(role) ? role : RoleType::None;
    }
    static bool ActiveRolePhase(int state) { return state >= 2 && state <= 7; }
    static std::unordered_map<std::string, RoleType> AssignedRoles(uintptr_t data) {
        std::unordered_map<std::string, RoleType> roles;
        uintptr_t keys = data ? SafeReadVal<uintptr_t>(data + 0x10) : 0;
        uintptr_t values = data ? SafeReadVal<uintptr_t>(data + 0x18) : 0;
        uintptr_t count = keys ? SafeReadVal<uintptr_t>(keys + 0x18) : 0;
        if (!values || count > 128 || count != SafeReadVal<uintptr_t>(values + 0x18)) return roles;
        for (uintptr_t i = 0; i < count; ++i) {
            auto role = static_cast<RoleType>(SafeReadVal<int16_t>(values + 0x20 + i * 2));
            auto id = ReadIl2CppString(SafeReadVal<uintptr_t>(keys + 0x20 + i * sizeof(uintptr_t)));
            if (id.empty()) continue;
            if (KnownRole(role)) roles[id] = role;
            else roles.erase(id); // Match the game's last assignment when a packet repeats an ID.
        }
        return roles;
    }
    static uintptr_t CurrentGameManager(uintptr_t module) {
        // Read the already initialized Singleton<MainManager> chain; never initialize it here.
        uintptr_t method = SafeReadVal<uintptr_t>(module + 0x50B6AF0);
        uintptr_t klass = method ? SafeReadVal<uintptr_t>(method + 0x20) : 0;
        uintptr_t context = klass ? SafeReadVal<uintptr_t>(klass + 0xC0) : 0;
        uintptr_t singleton = context ? SafeReadVal<uintptr_t>(context + 0x10) : 0;
        uintptr_t fields = singleton ? SafeReadVal<uintptr_t>(singleton + 0xB8) : 0;
        uintptr_t main = fields ? SafeReadVal<uintptr_t>(fields) : 0;
        uintptr_t manager = main ? SafeReadVal<uintptr_t>(main + 0x148) : 0;
        return manager && SafeReadVal<uintptr_t>(manager + 0x10) ? manager : 0;
    }

    static uintptr_t (*g_GetGameplayMap)(const void*) = nullptr;
    static void (*g_CompleteTask)(uintptr_t, uintptr_t, bool, bool, bool, bool, const void*) = nullptr;
    static void (*g_LocalTaskCompleted)(uintptr_t, uintptr_t, const void*) = nullptr;
    static std::atomic<bool> g_AutoTaskRequested{false}, g_AutoTaskBusy{false}, g_AutoTaskCancel{false};
    struct AutoTaskRun {
        uintptr_t owner = 0, handler = 0, native = 0;
        ULONGLONG started = 0, lastSent = 0;
        int submitted = 0;
        std::unordered_map<std::string, int32_t> stages;
    };
    static AutoTaskRun g_AutoTasks;
    static void ResetAutoTasks() {
        g_AutoTasks = {}; g_AutoTaskRequested = false; g_AutoTaskBusy = false; g_AutoTaskCancel = false;
    }
    static std::vector<uintptr_t> AssignedNormalTasks(uintptr_t handler) {
        std::vector<uintptr_t> tasks, eligible;
        if (!handler || !SafeReadVal<uintptr_t>(handler + 0x10)) return eligible;
        CollectPlayers(SafeReadVal<uintptr_t>(handler + 0x30), tasks);
        for (uintptr_t task : tasks)
            if (!SafeReadVal<bool>(task + 0x53) && !SafeReadVal<bool>(task + 0xD1) && !SafeReadVal<bool>(task + 0x80) &&
                !ReadIl2CppString(SafeReadVal<uintptr_t>(task + 0x10)).empty()) eligible.push_back(task);
        return eligible;
    }
    static uintptr_t TaskHandlerForMap() {
        if (!IsGameThread() || !g_GetGameplayMap) return 0;
        __try {
            uintptr_t map = g_GetGameplayMap(nullptr);
            if (!map || !SafeReadVal<uintptr_t>(map + 0x10)) return 0;
            uintptr_t handler = SafeReadVal<uintptr_t>(map + 0x2D8);
            return handler && SafeReadVal<uintptr_t>(handler + 0x10) && SafeReadVal<uintptr_t>(handler + 0x58) == map ? handler : 0;
        } __except (EXCEPTION_EXECUTE_HANDLER) { return 0; }
    }
    static bool SubmitTask(uintptr_t manager, uintptr_t handler, uintptr_t id) {
        if (!IsGameThread() || !g_CompleteTask || !g_LocalTaskCompleted || !id) return false;
        __try {
            g_CompleteTask(handler, id, false, false, false, true, nullptr);
            g_LocalTaskCompleted(manager, id, nullptr);
            return true;
        } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
    }
    static void ProcessAutoTasks(const LocalContext& context, uintptr_t manager, uintptr_t handler, int state,
                                 ULONGLONG now = GetTickCount64()) {
        if (!IsGameThread()) return;
        if (g_AutoTaskCancel.exchange(false)) {
            ResetAutoTasks(); PushEvent("Auto task stopped", EventKind::Task); return;
        }
        if (g_AutoTaskRequested.exchange(false)) {
            g_AutoTasks = {}; g_AutoTasks.owner = context.local; g_AutoTasks.handler = handler;
            g_AutoTasks.native = handler ? SafeReadVal<uintptr_t>(handler + 0x10) : 0;
            g_AutoTasks.started = now; g_AutoTaskBusy = true;
        }
        if (!g_AutoTaskBusy) return;
        if (state != 2 || !manager || !handler || !g_AutoTasks.native || context.local != g_AutoTasks.owner || handler != g_AutoTasks.handler ||
            SafeReadVal<uintptr_t>(handler + 0x10) != g_AutoTasks.native || SafeReadVal<bool>(context.player + 0x200)) {
            ResetAutoTasks(); PushEvent("Auto task unavailable during a meeting, scene change or spectating", EventKind::Task); return;
        }
        auto tasks = AssignedNormalTasks(handler);
        if (tasks.empty() && (!g_AutoTasks.submitted || now - g_AutoTasks.lastSent >= 750)) {
            const auto message = !g_AutoTasks.submitted ? "Auto task: no normal tasks assigned" :
                "Auto task finished: " + std::to_string(g_AutoTasks.submitted) + " steps, 0 remaining";
            ResetAutoTasks(); PushEvent(message, EventKind::Task); return;
        }
        if (now - g_AutoTasks.started >= 15000 || g_AutoTasks.submitted >= 128) {
            const auto message = "Auto task stopped: " + std::to_string(tasks.size()) + " tasks still assigned";
            ResetAutoTasks(); PushEvent(message, EventKind::Task); return;
        }
        int dispatched = 0;
        for (uintptr_t task : tasks) {
            if (dispatched >= 16 || g_AutoTasks.submitted >= 128) break;
            uintptr_t idObject = SafeReadVal<uintptr_t>(task + 0x10);
            auto id = ReadIl2CppString(idObject);
            int32_t stage = SafeReadVal<int32_t>(task + 0x3C, INT32_MIN);
            if (id.empty() || stage < -1 || stage > 4096) continue;
            auto sent = g_AutoTasks.stages.find(id);
            if (sent != g_AutoTasks.stages.end() && sent->second == stage) continue;
            // Revalidate membership after prior requests: callbacks may replace the assignment dictionary.
            auto current = AssignedNormalTasks(handler);
            if (std::find(current.begin(), current.end(), task) == current.end()) continue;
            if (!SubmitTask(manager, handler, idObject)) {
                ResetAutoTasks(); PushEvent("Auto task stopped: completion failed", EventKind::Task); return;
            }
            g_AutoTasks.stages[id] = stage; ++g_AutoTasks.submitted; ++dispatched; g_AutoTasks.lastSent = now;
        }
    }
    static void ClearSpectatorRoles() {
        std::lock_guard<std::mutex> lock(g_RoleMutex);
        g_SpectatorRoles.clear(); g_RevealedRoles.clear(); g_RoleOwner = 0; g_RolesReceivedAt = 0;
    }
    static void CaptureSpectatorRoles(uintptr_t packet, uintptr_t owner) {
        uintptr_t array = packet ? SafeReadVal<uintptr_t>(packet + 0x18) : 0;
        if (!array) return; // Incremental spectator packets can omit the initial roster.
        uintptr_t count = SafeReadVal<uintptr_t>(array + 0x18);
        if (count > 128) return;
        std::unordered_map<std::string, RoleType> roles;
        for (uintptr_t index = 0; index < count; ++index) {
            uintptr_t info = SafeReadVal<uintptr_t>(array + 0x20 + index * sizeof(uintptr_t));
            if (!info) continue;
            int32_t raw = SafeReadVal<int32_t>(info + 0x20, -1);
            std::string id = ReadIl2CppString(SafeReadVal<uintptr_t>(info + 0x10));
            if (id.empty()) continue;
            if (raw > 0 && raw <= 114) roles[id] = static_cast<RoleType>(raw);
            else roles.erase(id);
        }
        std::lock_guard<std::mutex> lock(g_RoleMutex);
        if (owner && g_RoleOwner && owner != g_RoleOwner) g_RevealedRoles.clear();
        g_SpectatorRoles = std::move(roles);
        g_RoleOwner = owner; g_RolesReceivedAt = GetTickCount64();
    }
    static RoleType SpectatorRole(const std::string& id, uintptr_t owner) {
        std::lock_guard<std::mutex> lock(g_RoleMutex);
        if (!owner || (g_RoleOwner && g_RoleOwner != owner) ||
            (!g_RoleOwner && GetTickCount64() - g_RolesReceivedAt > 5000)) return RoleType::None;
        if (!g_RoleOwner) g_RoleOwner = owner;
        auto revealed = g_RevealedRoles.find(id);
        if (revealed != g_RevealedRoles.end()) return revealed->second;
        auto found = g_SpectatorRoles.find(id);
        return found == g_SpectatorRoles.end() ? RoleType::None : found->second;
    }

    static void CaptureRevealedRole(const std::string& id, int16_t raw, uintptr_t owner) {
        if (id.empty() || !owner || raw <= 0 || raw > 114 || !KnownRole(static_cast<RoleType>(raw))) return;
        std::lock_guard<std::mutex> lock(g_RoleMutex);
        if (g_RoleOwner && g_RoleOwner != owner) {
            g_SpectatorRoles.clear(); g_RevealedRoles.clear();
        }
        g_RoleOwner = owner; g_RolesReceivedAt = GetTickCount64();
        g_RevealedRoles[id] = static_cast<RoleType>(raw);
    }

    static uintptr_t LocalInstance() {
        LocalContext context;
        if (ReadLocalContext(g_LocalObject.load(), &context)) return context.local;
        uintptr_t module = reinterpret_cast<uintptr_t>(GetModuleHandleW(L"GameAssembly.dll"));
        if (!module) return 0;
        uintptr_t klass = SafeReadVal<uintptr_t>(module + Offsets::LocalPlayer_TypeInfo);
        uintptr_t fields = klass ? SafeReadVal<uintptr_t>(klass + Offsets::Il2CppClass_StaticFields) : 0;
        uintptr_t local = fields ? SafeReadVal<uintptr_t>(fields + Offsets::LocalPlayer::Instance) : 0;
        return ReadLocalContext(local, &context) ? local : 0;
    }

    static void OnSpectatorInfo(uintptr_t manager, uintptr_t packet, uintptr_t extra, const void* method) {
        g_OriginalSpectatorInfo(manager, packet, extra, method);
        CaptureSpectatorRoles(packet, LocalInstance());
    }
    static void OnRevealRole(uintptr_t userId, int16_t role, const void* method) {
        const auto id = ReadIl2CppString(userId);
        g_OriginalRevealRole(userId, role, method);
        if (g_Running) {
            CaptureRevealedRole(id, role, LocalInstance());
            if (!id.empty() && KnownRole(static_cast<RoleType>(role)))
                PushEvent(EventNameById(id) + " revealed as " + GetInternalRoleName(static_cast<RoleType>(role)), EventKind::Role);
        }
    }
    static void OnRoundStart(uintptr_t manager, bool firstRound, const void* method) {
        ClearSpectatorRoles();
        g_OriginalRoundStart(manager, firstRound, method);
        if (g_Running) PushEvent(firstRound ? "First round started" : "New round started", EventKind::Round);
    }

    struct NativeToggle { uintptr_t object, native; bool original; };
    static std::vector<NativeToggle> g_Colliders;
    static bool SetColliderTrigger(uintptr_t native, bool value) {
        if (!IsGameThread() || !g_Unity.setTrigger || !native) return false;
        __try { g_Unity.setTrigger(native, value); return true; }
        __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
    }
    static bool GetColliderTrigger(uintptr_t native, bool* value) {
        if (!IsGameThread() || !g_Unity.isTrigger || !native) return false;
        __try { *value = g_Unity.isTrigger(native); return true; }
        __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
    }
    static void RestoreColliders() {
        for (const auto& c : g_Colliders)
            if (SafeReadVal<uintptr_t>(c.object + 0x10) == c.native)
                SetColliderTrigger(c.native, c.original);
        g_Colliders.clear();
    }
    static void ApplyNoClip(uintptr_t player, bool enabled) {
        std::vector<uintptr_t> wanted;
        if (enabled && player) {
            for (uintptr_t offset : {0x2D8, 0x2E0, 0x2E8, 0x2F8}) {
                uintptr_t object = SafeReadVal<uintptr_t>(player + offset);
                if (object) wanted.push_back(object);
            }
        }
        for (auto it = g_Colliders.begin(); it != g_Colliders.end();) {
            if (std::find(wanted.begin(), wanted.end(), it->object) == wanted.end() ||
                SafeReadVal<uintptr_t>(it->object + 0x10) != it->native) {
                if (SafeReadVal<uintptr_t>(it->object + 0x10) == it->native)
                    SetColliderTrigger(it->native, it->original);
                it = g_Colliders.erase(it);
            } else ++it;
        }
        for (uintptr_t object : wanted) {
            uintptr_t native = SafeReadVal<uintptr_t>(object + 0x10);
            if (!native) continue;
            auto it = std::find_if(g_Colliders.begin(), g_Colliders.end(), [&](const NativeToggle& c) { return c.object == object; });
            if (it == g_Colliders.end()) {
                bool original = false;
                if (!GetColliderTrigger(native, &original)) continue;
                g_Colliders.push_back({object, native, original});
            }
            SetColliderTrigger(native, true);
        }
    }

    static uintptr_t g_DroneLocal = 0, g_DroneNative = 0, g_DroneOutput = 0, g_DroneState = 0, g_DroneLens = 0;
    static float g_OriginalView = 0, g_AppliedView = 0;
    static void (*g_OverrideView)(uintptr_t, float, const void*) = nullptr;
    static void (*g_ResetView)(uintptr_t, const void*) = nullptr;
    static bool ReadView(uintptr_t native, float* value) {
        if (!IsGameThread() || !g_Unity.cameraSize || !native) return false;
        __try { *value = g_Unity.cameraSize(native); return std::isfinite(*value) && *value > 0; }
        __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
    }
    static void ApplyDrone(uintptr_t local, bool enabled, float scale) {
        if (!IsGameThread() || !g_OverrideView || !g_ResetView) return;
        uintptr_t camera = local ? SafeReadVal<uintptr_t>(local + Offsets::LocalPlayer::mainCamera) : 0;
        uintptr_t native = camera ? SafeReadVal<uintptr_t>(camera + 0x10) : 0;
        uintptr_t localNative = local ? SafeReadVal<uintptr_t>(local + 0x10) : 0;
        __try {
            if (!enabled || local != g_DroneLocal || native != g_DroneOutput || localNative != g_DroneNative) {
                if (g_DroneLocal && SafeReadVal<uintptr_t>(g_DroneLocal + 0x10) == g_DroneNative)
                    g_ResetView(g_DroneLocal, nullptr);
                g_DroneLocal = g_DroneNative = g_DroneOutput = g_DroneState = g_DroneLens = 0;
            }
            if (!enabled || !native || !localNative || !std::isfinite(scale)) return;
            uintptr_t state = SafeReadVal<uintptr_t>(local + Offsets::LocalPlayer::stateCamera);
            uintptr_t lens = SafeReadVal<uintptr_t>(local + Offsets::LocalPlayer::scriptableState);
            if (!state || !lens || !SafeReadVal<uintptr_t>(state + 0x10) || !SafeReadVal<uintptr_t>(lens + 0x10)) return;
            if (!g_DroneLocal) {
                if (!std::isfinite(g_OriginalView) || g_OriginalView <= 0) {
                    float base = 0;
                    if (!ReadView(native, &base)) return;
                    g_OriginalView = base;
                }
                g_DroneLocal = local; g_DroneNative = localNative; g_DroneOutput = native;
                g_AppliedView = 0;
            }
            if (state != g_DroneState || lens != g_DroneLens) g_AppliedView = 0;
            g_DroneState = state; g_DroneLens = lens;
            float desired = (std::min)(g_OriginalView * std::clamp(scale, 1.0f, 4.0f), 40.0f);
            const float lensSize = SafeReadVal<float>(lens + 0xBC);
            if (desired != g_AppliedView || !std::isfinite(lensSize) || fabsf(lensSize - desired) > 0.001f ||
                SafeReadVal<uintptr_t>(state + 0xB8)) {
                // Set the virtual-camera lens so Cinemachine and name labels share one zoom.
                g_OverrideView(local, desired, nullptr);
                g_AppliedView = desired;
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            g_DroneLocal = g_DroneNative = g_DroneOutput = g_DroneState = g_DroneLens = 0;
            RuntimeLog(L"Camera transition failed; override released");
        }
    }

    static void ResetRoomState() {
        if (!IsGameThread()) return;
        uintptr_t previous = g_LocalObject.load();
        if (previous) ClearEvents();
        {
            std::lock_guard<std::mutex> lock(g_RoleMutex);
            if (previous && g_RoleOwner == previous) {
                g_SpectatorRoles.clear(); g_RevealedRoles.clear(); g_RoleOwner = 0; g_RolesReceivedAt = 0;
            }
        }
        g_LocalObject = 0;
        ResetAutoTasks();
        g_RenderCamera = 0;
        RestoreSpeed();
        RestoreColliders();
        ApplyDrone(0, false, 1.0f);
        for (auto& field : g_Overrides) field.touched = false;
        RestoreUntouchedOverrides();
        ApplyFog(0);
        std::lock_guard<std::mutex> lock(g_FrameMutex);
        g_Frame = {};
    }

    static void OnLocalUpdate(uintptr_t local, const void* method) {
        // Let scene initialization/destruction finish before touching its objects.
        g_OriginalUpdate(local, method);
        if (!g_Running) return;
        DWORD thread = 0;
        g_GameThread.compare_exchange_strong(thread, GetCurrentThreadId());
        if (!IsGameThread()) return;
        if (!g_LocalUpdateReported) { g_LocalUpdateReported = true; RuntimeLog(L"LocalPlayer.Update callback active"); }
        LocalContext context;
        if (!ReadLocalContext(local, &context)) {
            ResetRoomState();
            return;
        }
        if (g_LocalObject.load() != local) {
            ResetRoomState();
            RuntimeLog(L"Room objects ready on Update thread");
        }
        g_LocalObject = local;
        if (!g_Unity.worldToScreen) ResolveUnityBindings(GetModuleHandleW(L"GameAssembly.dll"));
        auto request = RequestedFeatures();
        RefreshSpeed(context.local, request);
        if (g_AutoTaskRequested || g_AutoTaskBusy || g_AutoTaskCancel) {
            auto manager = CurrentGameManager(reinterpret_cast<uintptr_t>(GetModuleHandleW(L"GameAssembly.dll")));
            ProcessAutoTasks(context, manager, TaskHandlerForMap(), manager ? SafeReadVal<int32_t>(manager + 0x148, -1) : -1);
        }
        ApplyNoClip(context.player, request.noclip);
        ApplyDrone(context.local, request.drone, request.viewScale);
        if (g_RenderCamera != context.camera) CollectFrame(request);
    }
    static float AdjustSpeed(float original, bool enabled, float scale) {
        if (!enabled || !std::isfinite(original) || original <= 0 || !std::isfinite(scale)) return original;
        float adjusted = original * std::clamp(scale, 1.0f, 10.0f);
        return std::isfinite(adjusted) ? adjusted : original;
    }
    static bool ScaleMovementSpeed(uintptr_t local, const GameRequest& request) {
        LocalContext context;
        if (!IsGameThread() || !ReadLocalContext(local, &context) || !g_ReadGameFloat || !g_MakeGameFloat) return false;
        const float scale = request.speed && std::isfinite(request.speedScale) ? std::clamp(request.speedScale, 1.0f, 10.0f) : 1.0f;
        GameFloat value{};
        if (!SafeRead(context.fields + 0x28, &value)) return false;
        __try {
            float base = g_ReadGameFloat(value, nullptr);
            if (!std::isfinite(base)) return false;
            if (scale != 1 && !SafeWrite(context.fields + 0x28, g_MakeGameFloat(AdjustSpeed(base, true, scale), nullptr))) return false;
        } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
        g_SpeedLocal = local; g_AppliedSpeedScale = scale;
        return true;
    }
    static void OnMovementSpeed(uintptr_t local, const void* method) {
        g_OriginalSpeedUpdate(local, method);
        if (g_Running) ScaleMovementSpeed(local, RequestedFeatures());
    }
    static void RefreshSpeed(uintptr_t local, const GameRequest& request) {
        const float desired = request.speed && std::isfinite(request.speedScale) ? std::clamp(request.speedScale, 1.0f, 10.0f) : 1.0f;
        if (!g_OriginalSpeedUpdate || (local == g_SpeedLocal && desired == g_AppliedSpeedScale.load())) return;
        // Recompute from game state before scaling; slider changes must never multiply an already scaled value.
        g_OriginalSpeedUpdate(local, nullptr);
        ScaleMovementSpeed(local, request);
    }
    static void RestoreSpeed() {
        LocalContext context;
        if (IsGameThread() && g_SpeedLocal && g_OriginalSpeedUpdate && ReadLocalContext(g_SpeedLocal, &context))
            g_OriginalSpeedUpdate(g_SpeedLocal, nullptr);
        g_SpeedLocal = 0; g_AppliedSpeedScale = 1;
    }
    static bool ButtonRegistered(uintptr_t ui, uintptr_t button) {
        uintptr_t dict = ui ? SafeReadVal<uintptr_t>(ui + 0x3F0) : 0;
        std::vector<uintptr_t> buttons;
        CollectPlayers(dict, buttons);
        return std::find(buttons.begin(), buttons.end(), button) != buttons.end();
    }
    static bool IsLocalButton(uintptr_t button) {
        uintptr_t local = LocalInstance();
        uintptr_t ui = local ? SafeReadVal<uintptr_t>(local + 0x28) : 0;
        return ButtonRegistered(ui, button);
    }
    static void ReduceRegisteredCooldown(uintptr_t ui, uintptr_t button, bool enabled) {
        if (IsGameThread() && enabled && g_ReduceCooldown && ButtonRegistered(ui, button))
            g_ReduceCooldown(button, 86400.0f, nullptr);
    }
    static void OnCooldownUpdate(uintptr_t button, const void* method) {
        uintptr_t local = LocalInstance();
        uintptr_t ui = local ? SafeReadVal<uintptr_t>(local + 0x28) : 0;
        ReduceRegisteredCooldown(ui, button, g_Running && RequestedFeatures().noCooldown);
        g_OriginalCooldownUpdate(button, method);
    }

    static void CollectFrame(const GameRequest& request) {
        if (!IsGameThread()) return;
        LocalContext context;
        if (!ReadLocalContext(g_LocalObject.load(), &context)) {
            ResetRoomState();
            return;
        }
        float screenW = g_FrameWidth.load(), screenH = g_FrameHeight.load();
        if (screenW <= 0 || screenH <= 0) return;
        // -------------------------------------------------------------
        // Read Game Memory In-Process (Direct IL2CPP access)
        // -------------------------------------------------------------
        uintptr_t gameAssembly = (uintptr_t)GetModuleHandleW(L"GameAssembly.dll");
        std::vector<InternalPlayer> playerList;
        bool cameraReadyForFrame = false;

        for (auto& f : g_Overrides) f.touched = false;
        uintptr_t fogObject = 0;

        if (gameAssembly) {
            const auto manager = CurrentGameManager(gameAssembly);
            const bool activeRolePhase = manager && ActiveRolePhase(SafeReadVal<int32_t>(manager + 0x148, -1));
            const auto assignedRoles = AssignedRoles(activeRolePhase ? SafeReadVal<uintptr_t>(manager + 0x150) : 0);
            uintptr_t localPlayerInst = context.local;
            uintptr_t localPlayerEntity = context.player;

            // Project through the real output camera; a virtual camera can lag/blend.
            uintptr_t cameraObject = localPlayerInst ? SafeReadVal<uintptr_t>(localPlayerInst + Offsets::LocalPlayer::mainCamera) : 0;
            uintptr_t camera = cameraObject ? SafeReadVal<uintptr_t>(cameraObject + 0x10) : 0;
            float cameraW = 0, cameraH = 0;
            bool cameraReady = g_Unity.worldToScreen && ReadCameraSize(camera, &cameraW, &cameraH);
            cameraReadyForFrame = cameraReady;

            if (localPlayerInst) {
                if (request.menuOpen)
                    OverrideValue(localPlayerInst, 0x98, true); // LocalPlayer.disableMovement
                if (request.seeGhosts)
                    OverrideValue(localPlayerInst, Offsets::LocalPlayer::canSeeGhosts, true);
                if (request.removeFog) {
                    OverrideValue(localPlayerInst, Offsets::LocalPlayer::isInDark, false);
                    OverrideValue(localPlayerInst, Offsets::LocalPlayer::isInSandstorm, false);
                    uintptr_t fog = SafeReadVal<uintptr_t>(localPlayerInst + Offsets::LocalPlayer::fogOfWar);
                    if (fog) {
                        OverrideValue(fog, Offsets::FogOfWarLayout::isBlind, false);
                        OverrideValue(fog, Offsets::FogOfWarLayout::currentBlindness, 0.0f);
                        OverrideValue(fog, Offsets::FogOfWarLayout::targetBlindness, 0.0f);
                        OverrideValue(fog, Offsets::FogOfWarLayout::mayanLightsSabotaged, false);
                        if (SafeReadVal<bool>(fog + 0x141) &&
                            SafeReadVal<uintptr_t>(fog + 0x108) == localPlayerEntity)
                            fogObject = fog;
                    }
                }
            }
            if (localPlayerEntity && (request.removeFog || request.xray))
                OverrideValue(localPlayerEntity, Offsets::Entity::fogOfWarEnabled, false);
            if (localPlayerEntity && request.xray)
                OverrideValue(localPlayerEntity, Offsets::Entity::hasXRayVision, true);

            // Enumerate ALL Players across entire map (PlayerControllers + PlayableEntities)
            std::vector<uintptr_t> uniquePlayers;
            uniquePlayers.reserve(32);

            uintptr_t pcTypeInfo = SafeReadVal<uintptr_t>(gameAssembly + Offsets::PlayerController_TypeInfo);
            if (pcTypeInfo) {
                uintptr_t pcStatic = SafeReadVal<uintptr_t>(pcTypeInfo + Offsets::Il2CppClass_StaticFields);
                if (pcStatic) {
                    CollectPlayers(SafeReadVal<uintptr_t>(pcStatic + Offsets::PlayerControllerClass::PlayerControllers), uniquePlayers);
                }
            }

            uintptr_t peTypeInfo = SafeReadVal<uintptr_t>(gameAssembly + Offsets::PlayableEntity_TypeInfo);
            if (peTypeInfo) {
                uintptr_t peStatic = SafeReadVal<uintptr_t>(peTypeInfo + Offsets::Il2CppClass_StaticFields);
                if (peStatic) {
                    CollectPlayers(SafeReadVal<uintptr_t>(peStatic + Offsets::PlayableEntityClass::PlayableEntities), uniquePlayers);
                }
            }

            for (uintptr_t player : uniquePlayers) {
                if (!SafeReadVal<uintptr_t>(player + Offsets::Entity::m_CachedPtr)) continue;
                bool isLocal = SafeReadVal<bool>(player + Offsets::Entity::isLocal);
                bool isGhost = SafeReadVal<bool>(player + Offsets::Entity::isGhost);
                Vector3 wPos{};
                bool positionReady = ReadPosition(player, &wPos);

                uintptr_t namePtr = SafeReadVal<uintptr_t>(player + Offsets::Entity::nickname);
                std::string name = ReadIl2CppString(namePtr);
                if (name.empty()) name = "Player";

                RoleType role = EntityRole(player);

                if (!KnownRole(role) && activeRolePhase) {
                    std::string id = ReadIl2CppString(SafeReadVal<uintptr_t>(player + 0x20));
                    role = SpectatorRole(id, context.local);
                    if (!KnownRole(role)) {
                        auto found = assignedRoles.find(id);
                        if (found != assignedRoles.end()) role = found->second;
                    }
                }

                // Match the persistent player ID, never the display nickname.
                if (!KnownRole(role) && activeRolePhase && peTypeInfo) {
                    uintptr_t peStatic = SafeReadVal<uintptr_t>(peTypeInfo + Offsets::Il2CppClass_StaticFields);
                    uintptr_t atStart = peStatic ? SafeReadVal<uintptr_t>(peStatic + Offsets::PlayableEntityClass::PlayableEntitiesAtStart) : 0;
                    std::vector<uintptr_t> infos;
                    CollectPlayers(atStart, infos);
                    std::string playerId = ReadIl2CppString(SafeReadVal<uintptr_t>(player + 0x20)); // EntityControllerBase.id
                    for (uintptr_t info : infos) {
                        std::string id = ReadIl2CppString(SafeReadVal<uintptr_t>(info + Offsets::PlayableEntityInfoLayout::playerId));
                        if (!playerId.empty() && id == playerId) {
                            uintptr_t obj = SafeReadVal<uintptr_t>(info + Offsets::PlayableEntityInfoLayout::playerRole);
                            if (obj) SafeRead(obj + Offsets::GGDRole::RoleTypeOffset, reinterpret_cast<int16_t*>(&role));
                            break;
                        }
                    }
                }

                RoleInfo rInfo = GetInternalRoleInfo(role);
                bool teamKnown = std::string(rInfo.name) != "Unknown";
                rInfo.name = GetInternalRoleName(role);
                if (!teamKnown) {
                    int32_t team = SafeReadVal<int32_t>(player + Offsets::Entity::teamId, -1);
                    if (team == 1 || team == 2) {
                        rInfo.team = team == 1 ? TeamClassification::Crewmate : TeamClassification::Impostor;
                        teamKnown = true;
                    }
                }

                Vector2 screen{};
                float left = 0, right = 0, headY = 0, feetY = 0;
                bool projected = cameraReady && positionReady && Project(camera, wPos, screenW, screenH, cameraW, cameraH, &screen);
                if (projected) {
                    Bounds bounds{};
                    if (!ReadSpriteBounds(player, &bounds)) {
                        // shortcut: fixed proportions only when sprite bounds are unavailable.
                        bounds.center = {wPos.x, wPos.y + (g_HeadHeight - g_FeetDepth) * 0.5f, wPos.z};
                        bounds.extents = {g_BoxWidthScale * 0.5f, (g_HeadHeight + g_FeetDepth) * 0.5f, 0};
                    }
                    left = headY = FLT_MAX;
                    right = feetY = -FLT_MAX;
                    for (int corner = 0; corner < 8; ++corner) {
                        Vector3 world{
                            bounds.center.x + ((corner & 1) ? bounds.extents.x : -bounds.extents.x),
                            bounds.center.y + ((corner & 2) ? bounds.extents.y : -bounds.extents.y),
                            bounds.center.z + ((corner & 4) ? bounds.extents.z : -bounds.extents.z)};
                        Vector2 point{};
                        if (!Project(camera, world, screenW, screenH, cameraW, cameraH, &point)) { projected = false; break; }
                        left = (std::min)(left, point.x); right = (std::max)(right, point.x);
                        headY = (std::min)(headY, point.y); feetY = (std::max)(feetY, point.y);
                    }
                }
                float sx = projected ? (left + right) * 0.5f : 0;
                float sy = projected ? (headY + feetY) * 0.5f : 0;
                float bw = projected ? right - left : 0;
                float bh = projected ? feetY - headY : 0;

                InternalPlayer ip;
                ip.name = name;
                ip.role = role;
                ip.roleName = rInfo.name;
                ip.team = rInfo.team;
                ip.teamKnown = teamKnown;
                ip.isGhost = isGhost;
                ip.isLocal = isLocal;
                ip.worldPos = wPos;
                ip.screenPos = { sx, sy };
                ip.headScreenY = headY;
                ip.feetScreenY = feetY;
                ip.boxWidth = bw;
                ip.boxHeight = bh;
                ip.visible = projected && right >= 0 && left <= screenW && feetY >= 0 && headY <= screenH;

                playerList.push_back(ip);
            }
        }

        ApplyFog(fogObject);
        RestoreUntouchedOverrides();


        FrameSnapshot frame;
        frame.players = std::move(playerList);
        frame.cameraReady = cameraReadyForFrame;
        frame.sampledAt = GetTickCount64();
        std::lock_guard<std::mutex> lock(g_FrameMutex);
        g_Frame = std::move(frame);
    }

    static void OnCameraRendering(uintptr_t context, uintptr_t camera, const void* method) {
        g_OriginalCamera(context, camera, method);
        if (!g_Running || !IsGameThread()) return;
        if (!g_CameraReported) { g_CameraReported = true; RuntimeLog(L"Camera render callback active"); }
        LocalContext room;
        if (!ReadLocalContext(g_LocalObject.load(), &room)) {
            ResetRoomState();
            return;
        }
        if (camera != room.camera) return;
        auto request = RequestedFeatures();
        g_RenderCamera = room.camera;
        CollectFrame(request);
    }

    static void OnCameraPreCull(uintptr_t camera, const void* method) {
        g_OriginalPreCull(camera, method);
        if (!g_Running || !IsGameThread()) return;
        LocalContext room;
        if (!ReadLocalContext(g_LocalObject.load(), &room) || room.camera != camera) return;
        g_RenderCamera = camera;
        CollectFrame(RequestedFeatures());
    }

    static int g_MenuPage = 0;
    static ImU32 WithAlpha(ImU32 color, float alpha) {
        auto rgba = ImGui::ColorConvertU32ToFloat4(color);
        rgba.w *= alpha; return ImGui::ColorConvertFloat4ToU32(rgba);
    }
    static std::string FitLabel(const std::string& text, float size, float maximumWidth) {
        auto* font = ImGui::GetFont();
        if (font->CalcTextSizeA(size, FLT_MAX, 0, text.c_str()).x <= maximumWidth) return text;
        const char* remaining = nullptr;
        const float dots = font->CalcTextSizeA(size, FLT_MAX, 0, "...").x;
        font->CalcTextSizeA(size, (std::max)(1.0f, maximumWidth - dots), 0, text.c_str(), nullptr, &remaining);
        return std::string(text.c_str(), remaining) + "...";
    }
    static void DrawEspLabel(ImDrawList* draw, const std::string& text, float center, float y, ImU32 accent) {
        const auto display = ImGui::GetIO().DisplaySize;
        const float fontSize = ImGui::GetFontSize() * g_LabelScale;
        const auto label = FitLabel(text, fontSize, (std::min)(220.0f * g_LabelScale, display.x - 30));
        auto size = ImGui::GetFont()->CalcTextSizeA(fontSize, FLT_MAX, 0, label.c_str());
        const float x = (std::max)(8.0f, (std::min)(display.x - size.x - 16, center - size.x * 0.5f));
        y = (std::max)(8.0f, (std::min)(display.y - size.y - 12, y));
        draw->AddRectFilled({x - 6, y - 3}, {x + size.x + 7, y + size.y + 3}, IM_COL32(12, 16, 23, 224), 4);
        draw->AddRectFilled({x - 6, y - 1}, {x - 4, y + size.y + 1}, accent, 1);
        draw->AddText(ImGui::GetFont(), fontSize, {x, y}, IM_COL32(242, 245, 250, 255), label.c_str());
    }
    static void DrawESP(const std::vector<InternalPlayer>& players) {
        if (!g_EnableESP) return;
        auto* draw = ImGui::GetBackgroundDrawList();
        const auto display = ImGui::GetIO().DisplaySize;
        if (display.x < 80 || display.y < 80) return;
        for (const auto& p : players) {
            if (!p.visible || (p.isLocal && !g_ShowLocalPlayer) || (p.isGhost && !g_ShowGhosts)) continue;
            if (!std::isfinite(p.screenPos.x) || !std::isfinite(p.headScreenY) || !std::isfinite(p.feetScreenY) ||
                !std::isfinite(p.boxWidth) || p.boxWidth <= 0 || p.feetScreenY <= p.headScreenY) continue;
            ImU32 color = p.isGhost ? ImU32(ImColor(g_GhostColor)) : !p.teamKnown ? IM_COL32(180, 191, 210, 255) :
                p.team == TeamClassification::Impostor ? ImU32(ImColor(g_DuckColor)) :
                p.team == TeamClassification::Neutral ? ImU32(ImColor(g_NeutralColor)) : ImU32(ImColor(g_GooseColor));
            const float x = p.screenPos.x, top = p.headScreenY - g_BoxPadding, bottom = p.feetScreenY + g_BoxPadding;
            const float left = x - p.boxWidth * 0.5f - g_BoxPadding, right = x + p.boxWidth * 0.5f + g_BoxPadding;
            if (g_ShowLine) {
                float targetY = g_SnaplineMode == 1 ? (top + bottom) * 0.5f : g_SnaplineMode == 2 ? top : bottom;
                draw->AddLine({display.x * 0.5f, display.y}, {x, targetY}, WithAlpha(color, g_LineOpacity), g_LineThickness);
            }
            if (g_ShowBox) {
                draw->AddRectFilled({left, top}, {right, bottom}, WithAlpha(color, 0.035f), 3);
                if (g_CornerBoxes) {
                    const float corner = (std::min)(14.0f, (std::min)((right - left) * 0.28f, (bottom - top) * 0.2f));
                    for (int side = 0; side < 4; ++side) {
                        float cx = (side & 1) ? right : left, cy = (side & 2) ? bottom : top;
                        ImVec2 origin{cx, cy}, horizontal{cx + ((side & 1) ? -corner : corner), cy};
                        ImVec2 vertical{cx, cy + ((side & 2) ? -corner : corner)};
                        draw->AddLine(origin, horizontal, IM_COL32(0, 0, 0, 210), 3.5f);
                        draw->AddLine(origin, vertical, IM_COL32(0, 0, 0, 210), 3.5f);
                        draw->AddLine(origin, horizontal, color, 1.5f);
                        draw->AddLine(origin, vertical, color, 1.5f);
                    }
                } else {
                    draw->AddRect({left, top}, {right, bottom}, IM_COL32(0, 0, 0, 210), 3, 0, 3.5f);
                    draw->AddRect({left, top}, {right, bottom}, color, 3, 0, 1.5f);
                }
            }
            if (g_ShowName) DrawEspLabel(draw, p.name + (p.isLocal ? "  / YOU" : p.isGhost ? "  / GHOST" : ""), x,
                                       top - ImGui::GetFontSize() * g_LabelScale - 9, color);
            if (g_ShowRole) DrawEspLabel(draw, p.roleName == "Unknown" ? "Role unknown" : p.roleName, x, bottom + 7, color);
        }
    }

    static void DrawEventFeed() {
        auto events = ActiveEvents();
        if (!g_ShowEventFeed || events.empty()) return;
        const auto display = ImGui::GetIO().DisplaySize;
        if (display.x < 80 || display.y < 80) return;
        auto* draw = ImGui::GetForegroundDrawList();
        const float width = (std::min)(370.0f, display.x - 32), x = display.x - width - 16;
        float y = 20;
        const char* labels[] = {"DEATH", "REPORT", "MEETING", "PLAYER", "ROUND", "SABOTAGE", "ROLE", "VOTE", "TASK"};
        const auto now = GetTickCount64();
        for (auto it = events.rbegin(); it != events.rend(); ++it) {
            const auto& event = *it;
            auto size = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, width - 28, event.message.c_str());
            const float height = size.y + ImGui::GetFontSize() + 21;
            if (y + height > display.y - 16) break;
            const float remaining = g_FeedSeconds * 1000 - static_cast<float>(now - event.at);
            const float alpha = (std::min)(1.0f, remaining / 450.0f);
            ImU32 accent = event.kind == EventKind::Death ? IM_COL32(255, 135, 124, 255) :
                event.kind == EventKind::Sabotage ? IM_COL32(255, 191, 92, 255) :
                event.kind == EventKind::Role ? IM_COL32(145, 211, 255, 255) : IM_COL32(181, 204, 235, 255);
            draw->AddRectFilled({x, y}, {x + width, y + height}, WithAlpha(IM_COL32(13, 18, 26, 236), alpha), 6);
            draw->AddRect({x, y}, {x + width, y + height}, WithAlpha(IM_COL32(60, 73, 94, 190), alpha), 6);
            draw->AddRectFilled({x, y + 8}, {x + 3, y + height - 8}, WithAlpha(accent, alpha), 1);
            draw->AddText({x + 14, y + 8}, WithAlpha(accent, alpha), labels[static_cast<int>(event.kind)]);
            draw->AddText(ImGui::GetFont(), ImGui::GetFontSize(), {x + 14, y + ImGui::GetFontSize() + 13},
                          WithAlpha(IM_COL32(242, 245, 250, 255), alpha), event.message.c_str(), nullptr, width - 28);
            y += height + 7;
        }
    }

    static void DrawMenu(const std::vector<InternalPlayer>& players) {
        if (!g_ShowMenu) return;
        ImGui::SetNextWindowSize({740, 530}, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSizeConstraints({640, 450}, {1100, 850});
        if (ImGui::Begin("34Cheat", &g_ShowMenu, ImGuiWindowFlags_NoCollapse)) {
            const float footer = ImGui::GetFrameHeightWithSpacing();
            ImGui::BeginChild("Navigation", {135, -footer});
            const char* pages[] = {"Visuals", "Movement", "Camera", "Players", "Events"};
            for (int i = 0; i < 5; ++i)
                if (ImGui::Selectable(pages[i], g_MenuPage == i, 0, {0, 35})) g_MenuPage = i;
            ImGui::SetCursorPosY((std::max)(ImGui::GetCursorPosY(), ImGui::GetWindowHeight() - 150));
            ImGui::Separator();
            if (ImGui::Button("Save config", {-1, 0})) g_ConfigStatus = SaveConfig(ConfigPath()) ? "Config saved" : "Save failed";
            if (ImGui::Button("Load config", {-1, 0})) g_ConfigStatus = LoadConfig(ConfigPath()) ? "Config loaded" : "No valid config";
            ImGui::TextWrapped("%s", g_ConfigStatus);
            ImGui::EndChild();
            ImGui::SameLine();
            ImGui::BeginChild("Content", {0, -footer});
            ImGui::SeparatorText(pages[g_MenuPage]);
            if (g_MenuPage == 0) {
                ImGui::Checkbox("ESP", &g_EnableESP);
                ImGui::Checkbox("Kill feed (always on screen)", &g_ShowEventFeed);
                ImGui::BeginDisabled(!g_EnableESP);
                if (ImGui::BeginTable("Display", 2, ImGuiTableFlags_SizingStretchSame)) {
                    ImGui::TableNextColumn(); ImGui::Checkbox("Boxes", &g_ShowBox);
                    ImGui::TableNextColumn(); ImGui::Checkbox("Lines", &g_ShowLine);
                    ImGui::TableNextColumn(); ImGui::Checkbox("Names", &g_ShowName);
                    ImGui::TableNextColumn(); ImGui::Checkbox("Roles", &g_ShowRole);
                    ImGui::TableNextColumn(); ImGui::Checkbox("Ghosts", &g_ShowGhosts);
                    ImGui::TableNextColumn(); ImGui::Checkbox("Local player", &g_ShowLocalPlayer);
                    ImGui::EndTable();
                }
                ImGui::EndDisabled();
                ImGui::Spacing(); ImGui::SeparatorText("Vision");
                ImGui::Checkbox("Remove fog", &g_RemoveFog);
                ImGui::Checkbox("X-ray", &g_XRayVision);
                ImGui::Checkbox("See ghosts", &g_UnlockSeeGhosts);
                ImGui::Spacing();
                if (ImGui::CollapsingHeader("Appearance")) {
                    ImGui::Checkbox("Corner boxes", &g_CornerBoxes);
                    ImGui::SetNextItemWidth(180); ImGui::SliderFloat("Label size", &g_LabelScale, 0.8f, 1.5f, "%.1fx");
                    ImGui::SetNextItemWidth(180); ImGui::SliderFloat("Line width", &g_LineThickness, 1, 4, "%.1f px");
                    ImGui::SetNextItemWidth(180); ImGui::SliderFloat("Line opacity", &g_LineOpacity, 0.05f, 1, "%.2f");
                    ImGui::SetNextItemWidth(180); ImGui::SliderFloat("Box padding", &g_BoxPadding, 0, 10, "%.0f px");
                    const char* anchors[] = {"Feet", "Center", "Head"};
                    ImGui::SetNextItemWidth(180); ImGui::Combo("Line anchor", &g_SnaplineMode, anchors, 3);
                    ImGui::ColorEdit4("Duck", &g_DuckColor.x, ImGuiColorEditFlags_NoInputs);
                    ImGui::ColorEdit4("Goose", &g_GooseColor.x, ImGuiColorEditFlags_NoInputs);
                    ImGui::ColorEdit4("Neutral", &g_NeutralColor.x, ImGuiColorEditFlags_NoInputs);
                    ImGui::ColorEdit4("Ghost", &g_GhostColor.x, ImGuiColorEditFlags_NoInputs);
                }
                if (ImGui::CollapsingHeader("Notifications")) {
                    ImGui::SetNextItemWidth(180); ImGui::SliderFloat("Visible for", &g_FeedSeconds, 3, 15, "%.0f seconds");
                    ImGui::SetNextItemWidth(180); ImGui::SliderInt("Visible events", &g_FeedLimit, 3, 10);
                }
            } else if (g_MenuPage == 1) {
                ImGui::BeginDisabled(!g_GameHooksReady);
                ImGui::Checkbox("Speed", &g_SpeedEnabled);
                ImGui::BeginDisabled(!g_SpeedEnabled);
                ImGui::SetNextItemWidth(210); ImGui::SliderFloat("Multiplier", &g_SpeedScale, 1, 10, "%.1fx");
                ImGui::TextDisabled("Applied: %.1fx", g_AppliedSpeedScale.load());
                ImGui::EndDisabled();
                ImGui::Spacing();
                ImGui::Checkbox("No clip", &g_NoClip);
                ImGui::Checkbox("No cooldown", &g_NoCooldown);
                ImGui::Spacing();
                ImGui::BeginDisabled(!g_CameraReady);
                if (ImGui::Button(g_AutoTaskBusy ? "Stop auto task" : "Auto task (instant)", {210, 0})) {
                    if (g_AutoTaskBusy) g_AutoTaskCancel = true;
                    else { g_AutoTaskRequested = true; g_AutoTaskBusy = true; }
                }
                ImGui::EndDisabled();
                ImGui::TextDisabled("Complete your assigned normal tasks.");
                ImGui::EndDisabled();
                ImGui::Spacing();
                ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
                if (ImGui::Button("Reset movement")) { g_SpeedEnabled = g_NoClip = g_NoCooldown = false; g_SpeedScale = 1.5f; }
                ImGui::PopStyleColor();
            } else if (g_MenuPage == 2) {
                ImGui::BeginDisabled(!g_GameHooksReady);
                ImGui::Checkbox("Drone view", &g_DroneView);
                ImGui::BeginDisabled(!g_DroneView);
                ImGui::SetNextItemWidth(210); ImGui::SliderFloat("View range", &g_ViewScale, 1, 4, "%.1fx");
                ImGui::EndDisabled();
                ImGui::EndDisabled();
            } else if (g_MenuPage == 3) {
                static char filter[96]{};
                ImGui::SetNextItemWidth(-1);
                ImGui::InputTextWithHint("##search", "Find player", filter, sizeof(filter));
                ImGui::TextDisabled("Unknown = role data has not been received.");
                if (players.empty()) ImGui::TextDisabled("Join a room to see players.");
                else if (ImGui::BeginTable("Players", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable, {0, -1})) {
                    ImGui::TableSetupColumn("Name"); ImGui::TableSetupColumn("Role");
                    ImGui::TableSetupColumn("Team"); ImGui::TableSetupColumn("State");
                    ImGui::TableSetupScrollFreeze(0, 1); ImGui::TableHeadersRow();
                    for (const auto& p : players) {
                        if (filter[0] && p.name.find(filter) == std::string::npos) continue;
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn(); ImGui::TextUnformatted(p.name.c_str());
                        ImGui::TableNextColumn(); ImGui::TextUnformatted(p.roleName.c_str());
                        ImGui::TableNextColumn();
                        const char* team = !p.teamKnown ? "Unknown" : p.team == TeamClassification::Impostor ? "Duck" : p.team == TeamClassification::Neutral ? "Neutral" : "Goose";
                        ImGui::TextUnformatted(team);
                        ImGui::TableNextColumn(); ImGui::TextUnformatted(p.isLocal ? "You" : p.isGhost ? "Ghost" : "Alive");
                    }
                    ImGui::EndTable();
                }
            } else {
                auto history = EventHistory();
                if (ImGui::Button("Clear history")) { ClearEvents(); history.clear(); }
                ImGui::SameLine(); ImGui::TextDisabled("Last 100 events in this room");
                ImGui::BeginChild("Event history", {0, 0});
                const auto now = GetTickCount64();
                ImGui::PushTextWrapPos(0);
                for (auto it = history.rbegin(); it != history.rend(); ++it) {
                    ImGui::TextDisabled("%llus ago", (now - it->at) / 1000);
                    ImGui::SameLine(); ImGui::TextUnformatted(it->message.c_str());
                    ImGui::Separator();
                }
                if (history.empty()) ImGui::TextDisabled("Waiting for game events.");
                ImGui::PopTextWrapPos();
                ImGui::EndChild();
            }
            ImGui::EndChild();
            ImGui::Separator();
            ImGui::TextDisabled("%s", !g_GameHooksReady ? "Game data unavailable" : g_CameraReady ? "Connected" : "Waiting for room");
            ImGui::SameLine(ImGui::GetWindowWidth() - 170);
            ImGui::TextDisabled("Insert  /  menu");
        }
        ImGui::End();
    }

    // -------------------------------------------------------------
    // Window Procedure Hook
    // -------------------------------------------------------------
    static bool ShouldToggleMenu(UINT msg, WPARAM key, LPARAM flags, bool focused) {
        return focused && msg == WM_KEYDOWN && key == VK_INSERT && !(flags & (1LL << 30));
    }

    static void ConfigureMenuCursor(bool active) {
        auto& io = ImGui::GetIO();
        // The Windows cursor moves independently of the game's rendering rate.
        io.MouseDrawCursor = false;
        if (active) io.ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange;
        else io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
    }

    static LRESULT CALLBACK hkWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        std::unique_lock<std::recursive_mutex> uiLock(g_UiMutex);
        if (msg == WM_KILLFOCUS) {
            g_ShowMenu = false;
            std::lock_guard<std::mutex> lock(g_RequestMutex);
            g_Request.menuOpen = false;
        }
        LRESULT handled = 0;
        if (g_D3DInitialized) {
            ImGui::SetCurrentContext(g_ImGuiContext);
            ConfigureMenuCursor(g_ShowMenu && GetForegroundWindow() == hWnd);
            // A click can arrive before WM_MOUSEMOVE after focus/capture changes.
            if ((msg >= WM_LBUTTONDOWN && msg <= WM_MBUTTONDBLCLK) ||
                (msg >= WM_XBUTTONDOWN && msg <= WM_XBUTTONDBLCLK))
                ImGui::GetIO().AddMousePosEvent(static_cast<short>(LOWORD(lParam)),
                                              static_cast<short>(HIWORD(lParam)));
            handled = ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
        }
        if (g_ShowMenu && msg == WM_MOUSEACTIVATE) {
            std::lock_guard<std::mutex> lock(g_RequestMutex);
            g_Request.menuOpen = true;
            return MA_ACTIVATE;
        }

        if ((msg == WM_KEYDOWN || msg == WM_KEYUP) && wParam == VK_INSERT &&
            GetForegroundWindow() == hWnd) {
            if (ShouldToggleMenu(msg, wParam, lParam, true)) {
                g_ShowMenu = !g_ShowMenu;
                std::lock_guard<std::mutex> lock(g_RequestMutex);
                g_Request.menuOpen = g_ShowMenu;
            }
            return 0;
        }

        if (g_D3DInitialized && g_ShowMenu && GetForegroundWindow() == hWnd) {
            if (handled) return handled;
            // Keep focus/lifecycle and system shortcuts flowing to Windows.
            if ((msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST) ||
                (msg >= WM_KEYFIRST && msg <= WM_KEYLAST &&
                 msg != WM_SYSKEYDOWN && msg != WM_SYSKEYUP && msg != WM_SYSCHAR))
                return 0;
            if (msg == WM_INPUT)
                return DefWindowProc(hWnd, msg, wParam, lParam);
        }

        auto original = g_oWndProc;
        uiLock.unlock();
        return CallWindowProc(original, hWnd, msg, wParam, lParam);
    }

    // -------------------------------------------------------------
    // DirectX 11 Hook Implementation
    // -------------------------------------------------------------
    static void CreateRenderTarget(IDXGISwapChain* pSwapChain) {
        ID3D11Texture2D* pBackBuffer = nullptr;
        if (SUCCEEDED(pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer)))) {
            g_pDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_pRenderTargetView);
            pBackBuffer->Release();
        }
    }

    static void CleanupRenderTarget() {
        if (g_pRenderTargetView) {
            g_pRenderTargetView->Release();
            g_pRenderTargetView = nullptr;
        }
    }

    static HRESULT __stdcall hkResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags) {
        if (!g_Running || pSwapChain != g_SwapChain)
            return g_oResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
        g_pContext->OMSetRenderTargets(0, nullptr, nullptr);
        CleanupRenderTarget();
        HRESULT hr = g_oResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
        if (SUCCEEDED(hr)) CreateRenderTarget(pSwapChain);
        return hr;
    }

    static HRESULT __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {
        std::unique_lock<std::recursive_mutex> uiLock(g_UiMutex);
        // Present can wait for the window thread, which also needs the UI lock.
        const auto present = [&]() {
            uiLock.unlock();
            return g_oPresent(pSwapChain, SyncInterval, Flags);
        };
        if (!g_Running || (Flags & DXGI_PRESENT_TEST) || (g_SwapChain && g_SwapChain != pSwapChain))
            return present();
        if (!g_D3DInitialized) {
            if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&g_pDevice))) {
                g_pDevice->GetImmediateContext(&g_pContext);

                DXGI_SWAP_CHAIN_DESC desc;
                if (FAILED(pSwapChain->GetDesc(&desc))) {
                    g_pContext->Release(); g_pContext = nullptr;
                    g_pDevice->Release(); g_pDevice = nullptr;
                    return present();
                }
                g_GameHwnd = desc.OutputWindow;

                CreateRenderTarget(pSwapChain);

                // Initialize Dear ImGui
                IMGUI_CHECKVERSION();
                g_ImGuiContext = ImGui::CreateContext();
                ImGuiIO& io = ImGui::GetIO();
                io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
                io.ConfigDebugHighlightIdConflicts = false;

                UiTheme::Apply();
                UiTheme::Font();

                g_Win32Initialized = ImGui_ImplWin32_Init(g_GameHwnd);
                g_DX11Initialized = g_Win32Initialized && ImGui_ImplDX11_Init(g_pDevice, g_pContext);
                if (g_DX11Initialized && g_pRenderTargetView)
                    g_oWndProc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(g_GameHwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(hkWndProc)));
                if (!g_DX11Initialized || !g_pRenderTargetView || !g_oWndProc) {
                    if (g_DX11Initialized) ImGui_ImplDX11_Shutdown();
                    if (g_Win32Initialized) ImGui_ImplWin32_Shutdown();
                    ImGui::DestroyContext(g_ImGuiContext); g_ImGuiContext = nullptr;
                    g_DX11Initialized = g_Win32Initialized = false;
                    CleanupRenderTarget();
                    g_pContext->Release(); g_pContext = nullptr;
                    g_pDevice->Release(); g_pDevice = nullptr;
                    return present();
                }
                g_SwapChain = pSwapChain;
                g_D3DInitialized = true;
            } else {
                return present();
            }
        }

        ImGui::SetCurrentContext(g_ImGuiContext);
        if (!g_pRenderTargetView) CreateRenderTarget(pSwapChain);
        if (!g_pRenderTargetView) return present();

        // Start new ImGui frame
        ConfigureMenuCursor(g_ShowMenu && GetForegroundWindow() == g_GameHwnd);
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGuiIO& io = ImGui::GetIO();
        float screenW = io.DisplaySize.x;
        float screenH = io.DisplaySize.y;
        g_FrameWidth = screenW; g_FrameHeight = screenH;

        std::vector<InternalPlayer> playerList;
        {
            std::lock_guard<std::mutex> lock(g_FrameMutex);
            playerList = g_Frame.players;
            g_CameraReady = g_Frame.cameraReady && GetTickCount64() - g_Frame.sampledAt < 1500;
        }
        if (!g_CameraReady) playerList.clear();
        // -------------------------------------------------------------
        DrawESP(playerList);

        DrawEventFeed();
        DrawMenu(playerList);
        static bool menuWasOpen = true;
        if (menuWasOpen && !g_ShowMenu) g_ConfigStatus = SaveConfig(ConfigPath()) ? "Config saved" : "Save failed";
        menuWasOpen = g_ShowMenu;

        {
            std::lock_guard<std::mutex> lock(g_RequestMutex);
            g_Request.menuOpen = g_ShowMenu && GetForegroundWindow() == g_GameHwnd;
            g_Request.seeGhosts = g_UnlockSeeGhosts;
            g_Request.removeFog = g_RemoveFog;
            g_Request.xray = g_XRayVision;
            g_Request.drone = g_DroneView;
            g_Request.viewScale = g_ViewScale;
            g_Request.speed = g_SpeedEnabled;
            g_Request.speedScale = g_SpeedScale;
            g_Request.noclip = g_NoClip;
            g_Request.noCooldown = g_NoCooldown;
        }
        ImGui::Render();
        ID3D11RenderTargetView* previousTarget = nullptr;
        ID3D11DepthStencilView* previousDepth = nullptr;
        g_pContext->OMGetRenderTargets(1, &previousTarget, &previousDepth);
        g_pContext->OMSetRenderTargets(1, &g_pRenderTargetView, nullptr);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_pContext->OMSetRenderTargets(1, &previousTarget, previousDepth);
        if (previousTarget) previousTarget->Release();
        if (previousDepth) previousDepth->Release();
        if (!g_RenderReported) {
            g_RenderReported = true;
            RuntimeLog(L"First interface frame rendered");
            SignalSession(L"Ready");
        }

        return present();
    }

    // -------------------------------------------------------------
    // Dummy Device SwapChain VTable Extractor
    // -------------------------------------------------------------
    static bool GetSwapChainVTable(uintptr_t* pVTable) {
        WNDCLASSEXW wc = { sizeof(WNDCLASSEXW), CS_CLASSDC, DefWindowProcW, 0L, 0L, GetModuleHandleW(nullptr), nullptr, nullptr, nullptr, nullptr, L"DummyD3D11Class", nullptr };
        RegisterClassExW(&wc);
        HWND hWnd = CreateWindowW(wc.lpszClassName, L"", WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, nullptr, nullptr, wc.hInstance, nullptr);

        DXGI_SWAP_CHAIN_DESC sd;
        ZeroMemory(&sd, sizeof(sd));
        sd.BufferCount = 1;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = hWnd;
        sd.SampleDesc.Count = 1;
        sd.Windowed = TRUE;
        sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

        D3D_FEATURE_LEVEL featureLevel;
        ID3D11Device* pDev = nullptr;
        ID3D11DeviceContext* pCtx = nullptr;
        IDXGISwapChain* pChain = nullptr;

        HRESULT hr = D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0,
            D3D11_SDK_VERSION, &sd, &pChain, &pDev, &featureLevel, &pCtx);

        if (FAILED(hr)) {
            DestroyWindow(hWnd);
            UnregisterClassW(wc.lpszClassName, wc.hInstance);
            return false;
        }

        uintptr_t* vtbl = *reinterpret_cast<uintptr_t**>(pChain);
        memcpy(pVTable, vtbl, 18 * sizeof(uintptr_t));

        pChain->Release();
        pCtx->Release();
        pDev->Release();
        DestroyWindow(hWnd);
        UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return true;
    }

    // -------------------------------------------------------------
    // Main Initialization Thread
    // -------------------------------------------------------------
    static bool SupportedImage(HMODULE module) {
        if (!module) return false;
        __try {
            auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(module);
            if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew <= 0 || dos->e_lfanew > 4096) return false;
            auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(reinterpret_cast<uintptr_t>(module) + dos->e_lfanew);
            return nt->Signature == IMAGE_NT_SIGNATURE && nt->FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64 &&
                   nt->FileHeader.TimeDateStamp == SupportedGame::Timestamp && nt->OptionalHeader.SizeOfImage == SupportedGame::ImageSize;
        } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
    }
    static DWORD WINAPI InitThread(LPVOID lpParam) {
        if (LoadConfig(ConfigPath())) g_ConfigStatus = "Config loaded";
        RuntimeLog(L"Module attached; waiting for game engine");
        ULONGLONG start = GetTickCount64();
        HMODULE module = nullptr;
        while (g_Running && !(module = GetModuleHandleW(L"GameAssembly.dll")) && GetTickCount64() - start < 45000)
            Sleep(100);
        if (!module) return RuntimeFailure(L"game engine timeout");
        if (!SupportedImage(module)) return RuntimeFailure(L"unsupported game image");
        RuntimeLog(L"Game engine available"); SignalSession(L"Engine");
        // Game callbacks own Unity calls; no pattern-based guard patches.

        // Step 2: Initialize MinHook
        MH_STATUS status = MH_Initialize();
        if (status != MH_OK) return RuntimeFailure(L"MinHook initialization", status);

        // Step 3: Find SwapChain VTable via dummy device
        uintptr_t vtable[18] = { 0 };
        start = GetTickCount64();
        while (g_Running && !GetSwapChainVTable(vtable)) {
            if (GetTickCount64() - start > 10000) {
                MH_Uninitialize(); return RuntimeFailure(L"graphics device timeout");
            }
            Sleep(100);
        }
        if (!g_Running) return 1;

        // Hook Present (Index 8)
        void* pPresentTarget = (void*)vtable[8];
        status = MH_CreateHook(pPresentTarget, (LPVOID)hkPresent, (LPVOID*)&g_oPresent);
        if (status != MH_OK) {
            MH_Uninitialize();
            return RuntimeFailure(L"Present hook", status);
        }

        auto game = reinterpret_cast<uintptr_t>(GetModuleHandleW(L"GameAssembly.dll"));
        void* cameraTarget = reinterpret_cast<void*>(game + 0x3BD5A80); // RenderPipelineManager.BeginCameraRendering
        g_GameHooksReady = MH_CreateHook(cameraTarget, reinterpret_cast<void*>(OnCameraRendering),
                                        reinterpret_cast<void**>(&g_OriginalCamera)) == MH_OK;
        const bool updateReady = MH_CreateHook(reinterpret_cast<void*>(game + 0xDB8610), reinterpret_cast<void*>(OnLocalUpdate), reinterpret_cast<void**>(&g_OriginalUpdate)) == MH_OK;
        const bool preCullReady = MH_CreateHook(reinterpret_cast<void*>(game + 0x3B32B50), reinterpret_cast<void*>(OnCameraPreCull), reinterpret_cast<void**>(&g_OriginalPreCull)) == MH_OK;
        const bool spectatorReady = MH_CreateHook(reinterpret_cast<void*>(game + 0x9F1DF0), reinterpret_cast<void*>(OnSpectatorInfo), reinterpret_cast<void**>(&g_OriginalSpectatorInfo)) == MH_OK;
        const bool roundReady = MH_CreateHook(reinterpret_cast<void*>(game + 0x9F1D70), reinterpret_cast<void*>(OnRoundStart), reinterpret_cast<void**>(&g_OriginalRoundStart)) == MH_OK;
        const bool revealReady = MH_CreateHook(reinterpret_cast<void*>(game + 0xDD19D0), reinterpret_cast<void*>(OnRevealRole), reinterpret_cast<void**>(&g_OriginalRevealRole)) == MH_OK;
        const bool deathReady = MH_CreateHook(reinterpret_cast<void*>(game + 0x9F1C30), reinterpret_cast<void*>(OnPlayerDeath), reinterpret_cast<void**>(&g_OriginalDeath)) == MH_OK;
        const bool reportReady = MH_CreateHook(reinterpret_cast<void*>(game + 0xA13E40), reinterpret_cast<void*>(OnReport), reinterpret_cast<void**>(&g_OriginalReport)) == MH_OK;
        struct EventHook { uintptr_t offset; void* callback; void** original; };
        const EventHook eventHooks[] = {
            {0x9F1A30, reinterpret_cast<void*>(OnSimpleEvent<0>), reinterpret_cast<void**>(&g_OriginalSimpleEvents[0])},
            {0x9F19F0, reinterpret_cast<void*>(OnSimpleEvent<1>), reinterpret_cast<void**>(&g_OriginalSimpleEvents[1])},
            {0x9F1BD0, reinterpret_cast<void*>(OnSimpleEvent<2>), reinterpret_cast<void**>(&g_OriginalSimpleEvents[2])},
            {0x9F1BF0, reinterpret_cast<void*>(OnSimpleEvent<3>), reinterpret_cast<void**>(&g_OriginalSimpleEvents[3])},
            {0x9F1C70, reinterpret_cast<void*>(OnPlayerEvent<0>), reinterpret_cast<void**>(&g_OriginalPlayerEvents[0])},
            {0x9F1CB0, reinterpret_cast<void*>(OnPlayerEvent<1>), reinterpret_cast<void**>(&g_OriginalPlayerEvents[1])},
            {0x9F1CD0, reinterpret_cast<void*>(OnPlayerEvent<2>), reinterpret_cast<void**>(&g_OriginalPlayerEvents[2])},
            {0x9F1DB0, reinterpret_cast<void*>(OnSabotageEvent<0>), reinterpret_cast<void**>(&g_OriginalSabotageEvents[0])},
            {0x9F1DD0, reinterpret_cast<void*>(OnSabotageEvent<1>), reinterpret_cast<void**>(&g_OriginalSabotageEvents[1])},
            {0x9F1C50, reinterpret_cast<void*>(OnEaten), reinterpret_cast<void**>(&g_OriginalEaten)},
            {0x9F1CF0, reinterpret_cast<void*>(OnVote), reinterpret_cast<void**>(&g_OriginalVote)},
        };
        bool eventsReady = true;
        for (const auto& hook : eventHooks)
            eventsReady = (MH_CreateHook(reinterpret_cast<void*>(game + hook.offset), hook.callback, hook.original) == MH_OK) && eventsReady;
        const bool speedReady = MH_CreateHook(reinterpret_cast<void*>(game + 0xDB6450), reinterpret_cast<void*>(OnMovementSpeed), reinterpret_cast<void**>(&g_OriginalSpeedUpdate)) == MH_OK;
        g_ReadGameFloat = reinterpret_cast<decltype(g_ReadGameFloat)>(game + 0x544420);
        g_MakeGameFloat = reinterpret_cast<decltype(g_MakeGameFloat)>(game + 0x544430);
        g_GetGameplayMap = reinterpret_cast<decltype(g_GetGameplayMap)>(game + 0xA19A50);
        g_CompleteTask = reinterpret_cast<decltype(g_CompleteTask)>(game + 0xDC08D0);
        g_LocalTaskCompleted = reinterpret_cast<decltype(g_LocalTaskCompleted)>(game + 0x9F1B50);
        const bool cooldownReady = MH_CreateHook(reinterpret_cast<void*>(game + 0x909B00), reinterpret_cast<void*>(OnCooldownUpdate), reinterpret_cast<void**>(&g_OriginalCooldownUpdate)) == MH_OK;
        g_ReduceCooldown = reinterpret_cast<decltype(g_ReduceCooldown)>(game + 0x9099B0);
        g_DisableFog = reinterpret_cast<decltype(g_DisableFog)>(game + Offsets::FogOfWar_Disable);
        g_EnableFog = reinterpret_cast<decltype(g_EnableFog)>(game + Offsets::FogOfWar_Enable);
        g_OverrideView = reinterpret_cast<decltype(g_OverrideView)>(game + Offsets::LocalPlayer_OverrideOrthoSize);
        g_ResetView = reinterpret_cast<decltype(g_ResetView)>(game + Offsets::LocalPlayer_ResetOrthoSize);
        g_GameHooksReady = g_GameHooksReady && updateReady && preCullReady && spectatorReady && roundReady && revealReady && deathReady && reportReady && eventsReady && speedReady && cooldownReady;
        if (!g_GameHooksReady) {
            MH_Uninitialize();
            return RuntimeFailure(L"game callback hooks");
        }

        // Hook ResizeBuffers (Index 13)
        void* pResizeTarget = (void*)vtable[13];
        if (MH_CreateHook(pResizeTarget, (LPVOID)hkResizeBuffers, (LPVOID*)&g_oResizeBuffers) != MH_OK ||
            MH_EnableHook(MH_ALL_HOOKS) != MH_OK) {
            MH_DisableHook(MH_ALL_HOOKS);
            MH_Uninitialize();
            return RuntimeFailure(L"enabling graphics hooks");
        }

        RuntimeLog(L"Hooks installed; waiting for first interface frame");

        return 0;
    }

    void Start(HMODULE hMod) {
        g_Running = true;
        g_hModule = hMod;
        HMODULE pinned = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
                           reinterpret_cast<LPCWSTR>(hMod), &pinned);
        wchar_t name[80]{};
        swprintf_s(name, L"Local\\34Cheat.Module.%lu", GetCurrentProcessId());
        g_ModuleMarker = CreateMutexW(nullptr, FALSE, name);
        if (g_ModuleMarker && GetLastError() == ERROR_ALREADY_EXISTS) {
            CloseHandle(g_ModuleMarker); g_ModuleMarker = nullptr; g_Running = false;
            return;
        }
        DisableThreadLibraryCalls(hMod);
        HANDLE thread = CreateThread(nullptr, 0, InitThread, nullptr, 0, nullptr);
        if (thread) CloseHandle(thread);
    }

    void Shutdown() {
        g_Running = false;
        MH_DisableHook(MH_ALL_HOOKS);
        MH_Uninitialize();

        if (g_GameHwnd && g_oWndProc) {
            SetWindowLongPtr(g_GameHwnd, GWLP_WNDPROC, (LONG_PTR)g_oWndProc);
        }
        g_oWndProc = nullptr;
        if (IsGameThread()) {
            for (auto& f : g_Overrides) f.touched = false;
            RestoreUntouchedOverrides();
            ApplyFog(0);
            RestoreColliders();
            ApplyDrone(0, false, 1.0f);
        } else {
            g_Overrides.clear(); g_Colliders.clear();
        }
        if (g_ImGuiContext) {
            ImGui::SetCurrentContext(g_ImGuiContext);
            if (g_DX11Initialized) ImGui_ImplDX11_Shutdown();
            if (g_Win32Initialized) ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext(g_ImGuiContext);
            g_ImGuiContext = nullptr;
        }
        g_DX11Initialized = g_Win32Initialized = g_D3DInitialized = false;
        CleanupRenderTarget();
        if (g_pContext) { g_pContext->Release(); g_pContext = nullptr; }
        if (g_pDevice) { g_pDevice->Release(); g_pDevice = nullptr; }
        g_SwapChain = nullptr;
        if (g_ModuleMarker) { CloseHandle(g_ModuleMarker); g_ModuleMarker = nullptr; }

    }
}

extern "C" __declspec(dllexport) LRESULT CALLBACK GgdMessageHook(int code, WPARAM w, LPARAM l) {
    return CallNextHookEx(nullptr, code, w, l);
}

// -------------------------------------------------------------
// DLL Entry Point for 34Cheat.dll
// -------------------------------------------------------------
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            InternalCheat::Start(hModule);
            break;
        case DLL_PROCESS_DETACH:
            if (!lpReserved) InternalCheat::Shutdown();
            break;
    }
    return TRUE;
}
