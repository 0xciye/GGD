// Runs the real internal helpers against fixtures; never starts or attaches to a game.
#include "../Internal.cpp"
#include "../imgui/imgui_internal.h"
#include <array>
#include <cassert>
#include <limits>
#include <thread>
#include "../DesktopSurface.h"

using namespace InternalCheat;

template <size_t N, typename T>
static void Put(std::array<unsigned char, N>& bytes, size_t offset, T value) {
    assert(offset + sizeof(T) <= bytes.size());
    memcpy(bytes.data() + offset, &value, sizeof(T));
}

static uintptr_t transformOwner = 0, managedTransform = 0;
static uintptr_t FakeTransform(uintptr_t object, const void* method) {
    assert(object == transformOwner && !method);
    return managedTransform;
}
static void FakePosition(uintptr_t native, Vector3* p) { assert(native == 789); *p = {0, 0, 0}; }
static void FakeProjection(uintptr_t, const Vector3* world, int eye, Vector3* p) {
    assert(eye == 2);
    *p = {50 + world->x * 10, 50 + world->y * 10, world->z};
}
static bool FakeEnabled(uintptr_t native) { return native != 2; }
static void FakeBounds(uintptr_t native, Bounds* b) {
    *b = native == 1 ? Bounds{{0, 1, 5}, {1, 1, 0}} : Bounds{{0, 3, 5}, {2, 1, 0}};
}

static float testView = 5.5f;
static float FakeView(uintptr_t) { return testView; }
static void FakeSetView(uintptr_t, float value) { testView = value; }
static int viewOverrides = 0, viewResets = 0;
static void FakeOverrideView(uintptr_t local, float value, const void*) {
    testView = value; ++viewOverrides;
    SafeWrite(SafeReadVal<uintptr_t>(local + 0x88) + 0xBC, value);
    SafeWrite(SafeReadVal<uintptr_t>(local + 0x80) + 0xB8, uintptr_t(0));
}
static void FakeResetView(uintptr_t, const void*) { testView = 5.5f; ++viewResets; }
static int fogDisables = 0, fogEnables = 0;
static void FakeDisableFog(uintptr_t fog, const void*) {
    SafeWrite(fog + Offsets::FogOfWarLayout::disabled, true); ++fogDisables;
}
static void FakeEnableFog(uintptr_t fog, const void*) {
    SafeWrite(fog + Offsets::FogOfWarLayout::disabled, false); ++fogEnables;
}
static bool triggerValues[8]{};
static bool FakeTrigger(uintptr_t id) { assert(id < 8); return triggerValues[id]; }
static void FakeSetTrigger(uintptr_t id, bool value) { assert(id < 8); triggerValues[id] = value; }
static int cooldownCalls = 0;
static void FakeReduceCooldown(uintptr_t button, float amount, const void*) {
    assert(button == 65 && amount > 0); ++cooldownCalls;
}

static void CheckUiUnlocked() {
    bool acquired = false;
    std::thread probe([&] {
        acquired = g_UiMutex.try_lock();
        if (acquired) g_UiMutex.unlock();
    });
    probe.join();
    assert(acquired);
}
static LRESULT CALLBACK FakeWindowProc(HWND, UINT, WPARAM, LPARAM) {
    CheckUiUnlocked();
    return 123;
}
static HRESULT __stdcall FakePresent(IDXGISwapChain*, UINT, UINT) {
    CheckUiUnlocked();
    return S_OK;
}
static int updateCalls = 0;
static void FakeUpdate(uintptr_t, const void*) { ++updateCalls; }
static int cameraCalls = 0;
static void FakeCamera(uintptr_t, uintptr_t, const void*) { ++cameraCalls; }
static int preCullCalls = 0;
static void FakePreCull(uintptr_t, const void*) { ++preCullCalls; }
static float speedBase = 4.0f;
static int speedUpdates = 0;
static GameFloat FakeMakeGameFloat(float number, const void*) {
    GameFloat value{};
    memcpy(value.bytes, "TEST", 4); memcpy(value.bytes + 16, &number, sizeof(number)); return value;
}
static float FakeReadGameFloat(GameFloat value, const void*) {
    assert(memcmp(value.bytes, "TEST", 4) == 0);
    float number = 0; memcpy(&number, value.bytes + 16, sizeof(number)); return number;
}
static void FakeMovementSpeed(uintptr_t local, const void*) {
    LocalContext context; assert(ReadLocalContext(local, &context));
    SafeWrite(context.fields + 0x28, FakeMakeGameFloat(speedBase, nullptr)); ++speedUpdates;
}
static int deathCalls = 0, reportCalls = 0;
static void FakeDeath(uintptr_t, bool, uintptr_t, uintptr_t, const void*) { ++deathCalls; }
static void FakeDeathClearCause(uintptr_t, bool, uintptr_t, uintptr_t killed, const void*) {
    ++deathCalls; SafeWrite(killed + 0xD8, uint8_t(0));
}
static void FakeReport(uintptr_t, uintptr_t, uintptr_t, const void*) { ++reportCalls; }
static int simpleEventCalls = 0, playerEventCalls = 0;
static void FakeSimpleEvent(uintptr_t, const void*) { ++simpleEventCalls; }
static void FakePlayerEvent(uintptr_t, uintptr_t, const void*) { ++playerEventCalls; }
static std::vector<std::string> submittedTaskIds;
static uintptr_t expectedTaskHandler = 0, taskEntryToRemove = 0;
static int localTaskEvents = 0;
static void FakeCompleteTask(uintptr_t handler, uintptr_t id, bool remote, bool silent, bool unassign, bool disableAnim, const void*) {
    assert(handler == expectedTaskHandler && !remote && !silent && !unassign && disableAnim);
    submittedTaskIds.push_back(ReadIl2CppString(id));
    if (taskEntryToRemove) { SafeWrite(taskEntryToRemove, uintptr_t(0)); taskEntryToRemove = 0; }
}
static void FakeLocalTaskEvent(uintptr_t manager, uintptr_t id, const void*) {
    assert(manager == 222 && id); ++localTaskEvents;
}
static void TestAutoTasks() {
    constexpr int count = 20;
    alignas(8) std::array<unsigned char, 0xE8> tasks[count]{};
    alignas(8) std::array<unsigned char, 0x40> ids[count]{};
    alignas(8) std::array<unsigned char, 0x20 + count * 24> entries{};
    alignas(8) std::array<unsigned char, 0x40> dictionary{};
    alignas(8) std::array<unsigned char, 0x60> handler{};
    alignas(8) std::array<unsigned char, 0x208> player{};
    Put(entries, 0x18, uintptr_t(count));
    Put(dictionary, 0x18, reinterpret_cast<uintptr_t>(entries.data())); Put(dictionary, 0x20, int32_t(count));
    Put(handler, 0x10, uintptr_t(123)); Put(handler, 0x30, reinterpret_cast<uintptr_t>(dictionary.data()));
    expectedTaskHandler = reinterpret_cast<uintptr_t>(handler.data());
    LocalContext context{111, 0, reinterpret_cast<uintptr_t>(player.data()), 0};
    for (int i = 0; i < count; ++i) {
        auto name = L"task-" + std::to_wstring(i);
        Put(ids[i], 0x10, int32_t(name.size())); memcpy(ids[i].data() + 0x14, name.data(), name.size() * 2);
        Put(tasks[i], 0x10, reinterpret_cast<uintptr_t>(ids[i].data()));
        Put(tasks[i], 0xD1, i >= 5);
        Put(entries, 0x20 + i * 24, int32_t(i));
        Put(entries, 0x20 + i * 24 + 16, reinterpret_cast<uintptr_t>(tasks[i].data()));
    }
    Put(tasks[1], 0x53, true); Put(tasks[2], 0xD1, true); Put(tasks[3], 0x80, true);
    g_CompleteTask = FakeCompleteTask; g_LocalTaskCompleted = FakeLocalTaskEvent;
    ResetAutoTasks(); ClearEvents(); submittedTaskIds.clear(); localTaskEvents = 0;
    assert(AssignedNormalTasks(expectedTaskHandler).size() == 2);
    g_AutoTaskRequested = true;
    ProcessAutoTasks(context, 222, expectedTaskHandler, 2, 1000);
    assert(g_AutoTaskBusy && submittedTaskIds.size() == 2 && localTaskEvents == 2);
    ProcessAutoTasks(context, 222, expectedTaskHandler, 2, 1100);
    assert(submittedTaskIds.size() == 2); // Wait for acknowledgement instead of resending every frame.
    Put(tasks[0], 0x3C, int32_t(1));
    ProcessAutoTasks(context, 222, expectedTaskHandler, 2, 1200);
    assert(submittedTaskIds.size() == 3);
    Put(entries, 0x20 + 16, uintptr_t(0)); Put(entries, 0x20 + 4 * 24 + 16, uintptr_t(0));
    ProcessAutoTasks(context, 222, expectedTaskHandler, 2, 1500); assert(g_AutoTaskBusy);
    ProcessAutoTasks(context, 222, expectedTaskHandler, 2, 2000);
    assert(!g_AutoTaskBusy && EventHistory().back().message.find("3 steps, 0 remaining") != std::string::npos);
    Put(entries, 0x20 + 16, reinterpret_cast<uintptr_t>(tasks[0].data()));
    Put(entries, 0x20 + 4 * 24 + 16, reinterpret_cast<uintptr_t>(tasks[4].data()));
    auto calls = submittedTaskIds.size();
    Put(player, 0x200, true); g_AutoTaskRequested = true;
    ProcessAutoTasks(context, 222, expectedTaskHandler, 2, 3000);
    assert(!g_AutoTaskBusy && submittedTaskIds.size() == calls);
    Put(player, 0x200, false); g_AutoTaskRequested = true;
    ProcessAutoTasks(context, 222, expectedTaskHandler, 5, 3000);
    assert(!g_AutoTaskBusy && submittedTaskIds.size() == calls);
    g_AutoTaskRequested = true; ProcessAutoTasks(context, 222, expectedTaskHandler, 2, 4000);
    calls = submittedTaskIds.size(); Put(handler, 0x10, uintptr_t(124));
    ProcessAutoTasks(context, 222, expectedTaskHandler, 2, 4100);
    assert(!g_AutoTaskBusy && submittedTaskIds.size() == calls);
    g_AutoTaskRequested = true; ProcessAutoTasks(context, 222, expectedTaskHandler, 2, 5000);
    calls = submittedTaskIds.size();
    ProcessAutoTasks(context, 222, expectedTaskHandler, 2, 20000);
    assert(!g_AutoTaskBusy && submittedTaskIds.size() == calls);
    g_AutoTaskRequested = true; g_AutoTaskCancel = true;
    ProcessAutoTasks(context, 222, expectedTaskHandler, 2, 21000);
    assert(!g_AutoTaskBusy && !g_AutoTaskRequested && submittedTaskIds.size() == calls);
    // A synchronous callback can remove another task while processing the batch.
    taskEntryToRemove = reinterpret_cast<uintptr_t>(entries.data()) + 0x20 + 4 * 24 + 16;
    g_AutoTaskRequested = true; ProcessAutoTasks(context, 222, expectedTaskHandler, 2, 22000);
    assert(submittedTaskIds.size() == calls + 1); ResetAutoTasks();
    // Larger lists are spread across frames and every task stage is sent once.
    for (int i = 0; i < count; ++i) {
        Put(tasks[i], 0xD1, false); Put(tasks[i], 0x53, false); Put(tasks[i], 0x80, false);
        Put(entries, 0x20 + i * 24 + 16, reinterpret_cast<uintptr_t>(tasks[i].data()));
    }
    submittedTaskIds.clear(); g_AutoTaskRequested = true;
    ProcessAutoTasks(context, 222, expectedTaskHandler, 2, 23000); assert(submittedTaskIds.size() == 16);
    ProcessAutoTasks(context, 222, expectedTaskHandler, 2, 23100); assert(submittedTaskIds.size() == 20);
    ProcessAutoTasks(context, 222, expectedTaskHandler, 2, 23200); assert(submittedTaskIds.size() == 20);
    ResetAutoTasks(); ClearEvents(); g_CompleteTask = nullptr; g_LocalTaskCompleted = nullptr;
}

static void TestEventFeed() {
    ClearEvents();
    PushEvent("Same died", EventKind::Death, 1000, "id-a");
    PushEvent("Same died", EventKind::Death, 1001, "id-b");
    PushEvent("Same died", EventKind::Death, 1002, "id-a");
    assert(EventHistory().size() == 2); // Different victims with identical nicknames are separate deaths.
    ClearEvents();
    PushEvent("A killed B", EventKind::Death, 1000);
    PushEvent("A killed B", EventKind::Death, 1100);
    assert(ActiveEvents(1100).size() == 1);
    assert(ActiveEvents(5999).size() == 1);
    assert(ActiveEvents(6000).empty());
    assert(EventHistory().size() == 1); // Expired toasts remain available in the log.
    for (int i = 0; i < 8; ++i) PushEvent(std::to_string(i), EventKind::Report, 7000 + i);
    auto events = ActiveEvents(7100);
    assert(events.size() == 5 && events.front().message == "3" && events.back().message == "7");
    for (int i = 8; i < 110; ++i) PushEvent(std::to_string(i), EventKind::Player, 7000 + i);
    assert(EventHistory().size() == 100 && EventHistory().front().message == "10");
    ClearEvents();
    alignas(8) std::array<unsigned char, 0x30> name{};
    alignas(8) std::array<unsigned char, 0xE0> player{};
    Put(name, 0x10, int32_t(3)); memcpy(name.data() + 0x14, L"A\nB", 6);
    Put(player, Offsets::Entity::nickname, reinterpret_cast<uintptr_t>(name.data()));
    auto actor = reinterpret_cast<uintptr_t>(player.data());
    assert(EventPlayerName(actor) == "A B");
    g_OriginalDeath = FakeDeath; g_OriginalReport = FakeReport;
    OnPlayerDeath(0, false, actor, actor, nullptr);
    assert(deathCalls == 1 && ActiveEvents().back().message == "A B died — cause unknown");
    ClearEvents();
    Put(player, 0xD8, uint8_t(1));
    OnPlayerDeath(0, true, actor, actor, nullptr);
    assert(ActiveEvents().back().message == "A B died — normal kill");
    assert(DeathMessage("B", "A", true, 2) == "A killed B — explosion");
    assert(DeathMessage("B", "A", false, 8) == "B died — assassination");
    assert(DeathMessage("B", "A", true, 7) == "B died — voted out");
    assert(DeathMessage("B", "A", true, 3) == "B died — Lover linked death");
    assert(DeathMessage("Same", "Same", true, 1) == "Same killed Same — normal kill");
    assert(strcmp(DeathReasonName(17), "fell off bridge") == 0);
    assert(strcmp(DeathReasonName(255), "cause unknown") == 0);
    for (int reason = 1; reason <= 37; ++reason) assert(strcmp(DeathReasonName(uint8_t(reason)), "cause unknown") != 0);
    g_OriginalDeath = FakeDeathClearCause;
    Put(player, 0xD8, uint8_t(2));
    OnPlayerDeath(0, false, 0, actor, nullptr);
    assert(ActiveEvents().back().message == "A B died — explosion"); // Copy before the original clears the object.
    g_OriginalDeath = FakeDeath;
    ClearEvents();
    OnReport(0, 0, 0, nullptr);
    assert(reportCalls == 1 && ActiveEvents().back().kind == EventKind::Meeting);
    ClearEvents();
    OnReport(0, 0, reinterpret_cast<uintptr_t>(name.data()), nullptr);
    assert(reportCalls == 2 && ActiveEvents().back().kind == EventKind::Report);
    g_OriginalSimpleEvents[2] = FakeSimpleEvent;
    g_OriginalPlayerEvents[1] = FakePlayerEvent;
    g_OriginalSabotageEvents[0] = FakePlayerEvent;
    g_OriginalEaten = FakeReport;
    g_OriginalVote = FakeReport;
    OnSimpleEvent<2>(0, nullptr);
    assert(simpleEventCalls == 1 && EventHistory().back().message == "Meeting started");
    OnPlayerEvent<1>(0, actor, nullptr);
    assert(playerEventCalls == 1 && EventHistory().back().message == "A B left the room");
    OnSabotageEvent<0>(0, 0, nullptr);
    assert(playerEventCalls == 2 && EventHistory().back().message == "Sabotage started");
    OnEaten(0, actor, actor, nullptr);
    assert(EventHistory().back().message == "A B swallowed A B");
    OnVote(0, reinterpret_cast<uintptr_t>(name.data()), 0, nullptr);
    assert(EventHistory().back().message == "Player voted");
    auto* context = ImGui::CreateContext();
    auto& io = ImGui::GetIO(); io.IniFilename = nullptr;
    io.DisplaySize = {740, 530}; io.DeltaTime = 1.0f / 60;
    unsigned char* pixels; int w, h; io.Fonts->GetTexDataAsRGBA32(&pixels, &w, &h);
    g_ShowMenu = false; g_EnableESP = false;
    ImGui::NewFrame(); DrawEventFeed();
    InternalPlayer visual{};
    visual.name = "Người chơi có tên rất dài"; visual.roleName = "Unknown"; visual.visible = true;
    visual.screenPos = {120, 150}; visual.headScreenY = 100; visual.feetScreenY = 200; visual.boxWidth = 30;
    auto* background = ImGui::GetBackgroundDrawList();
    DrawESP({visual});
    assert(background->VtxBuffer.Size == 0); // Disabling ESP leaves the independent feed visible.
    g_EnableESP = true;
    DrawESP({visual});
    assert(background->VtxBuffer.Size > 0);
    const int visibleVertices = background->VtxBuffer.Size;
    visual.isLocal = true; DrawESP({visual});
    assert(background->VtxBuffer.Size == visibleVertices);
    visual.isLocal = false; visual.screenPos.x = std::numeric_limits<float>::quiet_NaN();
    DrawESP({visual}); assert(background->VtxBuffer.Size == visibleVertices);
    auto shortLabel = FitLabel(visual.name, ImGui::GetFontSize(), 70);
    assert(MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, shortLabel.c_str(), -1, nullptr, 0) > 0);
    assert(ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, 0, shortLabel.c_str()).x <= 70);
    ImGui::Render();
    assert(ImGui::GetForegroundDrawList()->VtxBuffer.Size > 0);
    assert(!io.WantCaptureMouse && !io.WantCaptureKeyboard);
    ImGui::DestroyContext(context);
    g_ShowMenu = g_EnableESP = true;
    ClearEvents();
}

static void TestConfig() {
    wchar_t folder[MAX_PATH]{}, filename[MAX_PATH]{};
    assert(GetTempPathW(MAX_PATH, folder) && GetTempFileNameW(folder, L"34c", 0, filename));
    const bool oldBoxes = g_ShowBox;
    const float oldSpeed = g_SpeedScale, oldOpacity = g_LineOpacity;
    const auto oldDuck = g_DuckColor;
    const int oldLimit = g_FeedLimit;
    g_ShowBox = false; g_SpeedScale = 8.5f; g_LineOpacity = 0.18f;
    g_DuckColor = {0.1f, 0.2f, 0.3f, 0.4f}; g_FeedLimit = 7;
    assert(SaveConfig(filename));
    g_ShowBox = true; g_SpeedScale = 1; g_LineOpacity = 1; g_DuckColor = {}; g_FeedLimit = 3;
    assert(LoadConfig(filename));
    assert(!g_ShowBox && g_SpeedScale == 8.5f && fabs(g_LineOpacity - 0.18f) < 0.001f && g_FeedLimit == 7);
    assert(fabs(g_DuckColor.z - 0.3f) < 0.001f && fabs(g_DuckColor.w - 0.4f) < 0.001f);
    WritePrivateProfileStringW(L"34Cheat", L"Boxes", L"invalid", filename);
    WritePrivateProfileStringW(L"34Cheat", L"LineOpacity", L"nan", filename);
    WritePrivateProfileStringW(L"34Cheat", L"SpeedScale", L"99999", filename);
    WritePrivateProfileStringW(L"34Cheat", L"FeedLimit", L"-20", filename);
    assert(LoadConfig(filename));
    assert(!g_ShowBox && fabs(g_LineOpacity - 0.18f) < 0.001f && g_SpeedScale == 10 && g_FeedLimit == 3);
    WritePrivateProfileStringW(L"34Cheat", L"Version", L"99", filename);
    assert(!LoadConfig(filename));
    assert(!SaveConfig(L"") && !LoadConfig(L""));
    assert(DeleteFileW(filename));
    g_ShowBox = oldBoxes; g_SpeedScale = oldSpeed; g_LineOpacity = oldOpacity; g_DuckColor = oldDuck; g_FeedLimit = oldLimit;
}

static void TestMouseClicks() {
    HWND window = CreateWindowExW(0, L"STATIC", L"Input fixture", WS_OVERLAPPEDWINDOW,
                                  0, 0, 320, 240, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
    assert(window);
    g_ImGuiContext = ImGui::CreateContext();
    assert(ImGui_ImplWin32_Init(window));
    g_D3DInitialized = true;
    g_oWndProc = FakeWindowProc;
    assert(hkWndProc(window, WM_MOUSEACTIVATE, 0, 0) == MA_ACTIVATE);
    auto& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.DisplaySize = {320, 240}; io.DeltaTime = 1.0f / 60;
    unsigned char* pixels; int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    // Opening the menu must use the OS cursor, even if software drawing was set.
    io.MouseDrawCursor = true;
    ConfigureMenuCursor(true);
    assert(!io.MouseDrawCursor);
    assert(!(io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange));
    SetCursor(nullptr);
    assert(ImGui_ImplWin32_WndProcHandler(window, WM_SETCURSOR, WPARAM(window), MAKELPARAM(HTCLIENT, WM_MOUSEMOVE)) == 1);
    HCURSOR nativeCursor = GetCursor();
    assert(nativeCursor);
    // Closing the menu must leave the game's own cursor unchanged.
    ConfigureMenuCursor(false);
    assert(io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange);
    assert(ImGui_ImplWin32_WndProcHandler(window, WM_SETCURSOR, WPARAM(window), MAKELPARAM(HTCLIENT, WM_MOUSEMOVE)) == 0);
    assert(GetCursor() == nativeCursor);
    bool checked = false;
    ImVec2 target{};
    auto frame = [&] {
        ImGui::NewFrame();
        ImGui::SetNextWindowPos({0, 0}); ImGui::SetNextWindowSize({320, 240});
        ImGui::Begin("Input fixture", nullptr, ImGuiWindowFlags_NoTitleBar);
        ImGui::Checkbox("Feature", &checked);
        target = ImGui::GetItemRectMin(); target.x += 8; target.y += 8;
        ImGui::End(); ImGui::Render();
    };
    frame(); frame();
    io.AddMousePosEvent(target.x, target.y);
    frame();
    for (UINT down : {WM_LBUTTONDOWN, WM_LBUTTONDBLCLK, WM_LBUTTONDOWN, WM_LBUTTONDBLCLK}) {
        const bool before = checked;
        // A button message carries its own position; WM_MOUSEMOVE may be absent.
        io.AddMousePosEvent(300, 220);
        frame();
        hkWndProc(window, down, MK_LBUTTON, MAKELPARAM(int(target.x), int(target.y)));
        frame();
        hkWndProc(window, WM_LBUTTONUP, 0, MAKELPARAM(int(target.x), int(target.y)));
        frame();
        assert(checked != before);
    }
    // Fast press/release between frames must also count as exactly one click.
    const bool before = checked;
    hkWndProc(window, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(int(target.x), int(target.y)));
    hkWndProc(window, WM_LBUTTONUP, 0, MAKELPARAM(int(target.x), int(target.y)));
    frame(); frame(); frame();
    assert(checked != before);
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext(g_ImGuiContext); g_ImGuiContext = nullptr;
    g_D3DInitialized = false; g_oWndProc = nullptr;
    DestroyWindow(window);
}

int main(int argc, char** argv) {
    g_GameThread = GetCurrentThreadId();
    if (argc >= 3 && strcmp(argv[1], "--snapshot") == 0) {
        g_MenuPage = argc > 3 ? atoi(argv[3]) : 0;
        DesktopSurface surface;
        if (!surface.Initialize(false, g_MenuPage == 6 ? 1280 : 740, g_MenuPage == 6 ? 720 : 530)) return 1;
        g_ShowMenu = true; g_GameHooksReady = true; g_CameraReady = true;
        const bool espPreview = g_MenuPage == 6;
        const bool feedPreview = g_MenuPage == 5 || espPreview;
        if (feedPreview || g_MenuPage == 4) {
            if (feedPreview) { g_ShowMenu = false; g_EnableESP = false; }
            PushEvent("Alice killed Bob", EventKind::Death);
            PushEvent("Charlie reported Bob", EventKind::Report);
            PushEvent("Dana called a meeting", EventKind::Meeting);
            PushEvent("Meeting started", EventKind::Meeting);
            PushEvent("Sabotage ended", EventKind::Sabotage);
        }
        std::vector<InternalPlayer> preview;
        for (int i = 0; i < 5; ++i) {
            InternalPlayer p{};
            p.name = i == 0 ? "You" : "Player " + std::to_string(i);
            p.roleName = i == 1 ? "Duck" : i == 2 ? "Sheriff" : i == 3 ? "Dodo" : "Goose";
            p.team = i == 1 ? TeamClassification::Impostor : i == 3 ? TeamClassification::Neutral : TeamClassification::Crewmate;
            p.teamKnown = true; p.isLocal = i == 0; p.isGhost = i == 4; preview.push_back(p);
        }
        if (espPreview) {
            g_EnableESP = true; g_ShowLocalPlayer = true;
            for (int i = 0; i < static_cast<int>(preview.size()); ++i) {
                auto& p = preview[i];
                p.visible = true; p.screenPos = {70.0f + 135.0f * i, 280};
                p.headScreenY = 230 + i * 10.0f; p.feetScreenY = 330 + i * 10.0f; p.boxWidth = 48;
            }
            preview[2].name = "Người chơi có tên rất dài để kiểm tra nhãn";
            preview[3].roleName = "Unknown"; preview[3].teamKnown = false;
        }
        for (int frame = 0; frame < 4;) {
            if (!surface.Begin()) continue;
            if (espPreview) DrawESP(preview);
            DrawEventFeed();
            if (!feedPreview) {
                ImGui::SetNextWindowPos({0, 0});
                DrawMenu(preview);
            }
            std::wstring path;
            int n = MultiByteToWideChar(CP_UTF8, 0, argv[2], -1, nullptr, 0);
            path.resize(n);
            MultiByteToWideChar(CP_UTF8, 0, argv[2], -1, path.data(), n);
            if (!surface.End(++frame == 4 ? path.c_str() : nullptr)) return 1;
        }
        return 0;
    }
    TestMouseClicks();
    TestEventFeed();
    TestConfig();
    TestAutoTasks();
    g_oPresent = FakePresent;
    assert(hkPresent(nullptr, 0, DXGI_PRESENT_TEST) == S_OK);
    g_oPresent = nullptr;
    assert(ShouldToggleMenu(WM_KEYDOWN, VK_INSERT, 0, true));
    assert(!ShouldToggleMenu(WM_KEYDOWN, VK_INSERT, 1LL << 30, true));
    assert(!ShouldToggleMenu(WM_KEYUP, VK_INSERT, 0, true));
    assert(!ShouldToggleMenu(WM_KEYDOWN, VK_INSERT, 0, false));
    assert(AdjustSpeed(4.0f, false, 3.0f) == 4.0f);
    assert(AdjustSpeed(4.0f, true, 1.5f) == 6.0f);
    assert(AdjustSpeed(4.0f, true, 10.0f) == 40.0f);
    assert(AdjustSpeed(4.0f, true, 100.0f) == 40.0f);
    assert(AdjustSpeed(4.0f, true, -1.0f) == 4.0f);
    assert(AdjustSpeed(4.0f, true, std::numeric_limits<float>::quiet_NaN()) == 4.0f);
    alignas(8) std::array<unsigned char, 4096> image{};
    auto* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(image.data());
    dos->e_magic = IMAGE_DOS_SIGNATURE; dos->e_lfanew = 0x100;
    auto* nt = reinterpret_cast<IMAGE_NT_HEADERS64*>(image.data() + 0x100);
    nt->Signature = IMAGE_NT_SIGNATURE;
    nt->FileHeader.Machine = IMAGE_FILE_MACHINE_AMD64;
    nt->FileHeader.TimeDateStamp = SupportedGame::Timestamp;
    nt->OptionalHeader.SizeOfImage = SupportedGame::ImageSize;
    assert(SupportedImage(reinterpret_cast<HMODULE>(image.data())));
    nt->FileHeader.TimeDateStamp ^= 1;
    assert(!SupportedImage(reinterpret_cast<HMODULE>(image.data())));
    assert(!SupportedImage(nullptr));
    // Role names past the old limit are preserved; invalid values remain unknown.
    assert(strcmp(GetInternalRoleName(static_cast<RoleType>(48)), "IdentityThief") == 0);
    assert(strcmp(GetInternalRoleName(static_cast<RoleType>(114)), "TTWereduck") == 0);
    assert(strcmp(GetInternalRoleName(RoleType::None), "Unknown") == 0);
    assert(strcmp(GetInternalRoleName(static_cast<RoleType>(-1)), "Unknown") == 0);
    assert(strcmp(GetInternalRoleName(static_cast<RoleType>(115)), "Unknown") == 0);
    assert(GetInternalRoleInfo(static_cast<RoleType>(56)).team == TeamClassification::Crewmate);
    assert(GetInternalRoleInfo(static_cast<RoleType>(57)).team == TeamClassification::Neutral);
    assert(GetInternalRoleInfo(static_cast<RoleType>(100)).team == TeamClassification::Crewmate);
    assert(GetInternalRoleInfo(static_cast<RoleType>(55)).team == TeamClassification::Impostor);
    assert(GetInternalRoleInfo(RoleType::Party).team == TeamClassification::Impostor);
    assert(strcmp(GetInternalRoleInfo(RoleType::None).name, "Unknown") == 0);
    alignas(8) std::array<unsigned char, 0xB0> roleEntity{};
    alignas(8) std::array<unsigned char, 0x20> roleObject{};
    Put(roleEntity, Offsets::Entity::playerRole, reinterpret_cast<uintptr_t>(roleObject.data()));
    Put(roleObject, 0x10, int16_t(56));
    const auto roleActor = reinterpret_cast<uintptr_t>(roleEntity.data());
    assert(EntityRole(roleActor) == RoleType::None); // A stale pointer is not a completed role assignment.
    Put(roleEntity, 0xA8, true);
    assert(EntityRole(roleActor) == static_cast<RoleType>(56));
    Put(roleEntity, 0xA8, uint8_t(2)); assert(EntityRole(roleActor) == RoleType::None);
    Put(roleEntity, 0xA8, true);
    Put(roleObject, 0x10, int16_t(115)); assert(EntityRole(roleActor) == RoleType::None);
    Put(roleObject, 0x10, int16_t(0)); assert(EntityRole(roleActor) == RoleType::None);
    Put(roleEntity, Offsets::Entity::playerRole, uintptr_t(0)); assert(EntityRole(roleActor) == RoleType::None);
    Put(roleEntity, Offsets::Entity::playerRole, uintptr_t(1)); assert(EntityRole(roleActor) == RoleType::None);
    assert(!ActiveRolePhase(0) && !ActiveRolePhase(1) && !ActiveRolePhase(-1) && !ActiveRolePhase(8));
    assert(ActiveRolePhase(2) && ActiveRolePhase(7));
    // Mid-round spectator snapshots identify exact roles by persistent ID.
    alignas(8) std::array<unsigned char, 0x30> roleIdA{}, roleIdB{}, spectatorPacket{};
    alignas(8) std::array<unsigned char, 0x28> spectatorInfos[2]{};
    alignas(8) std::array<unsigned char, 0x30> spectatorArray{};
    Put(roleIdA, 0x10, int32_t(4)); memcpy(roleIdA.data() + 0x14, L"id-a", 8);
    Put(roleIdB, 0x10, int32_t(4)); memcpy(roleIdB.data() + 0x14, L"id-b", 8);
    alignas(8) std::array<unsigned char, 0x30> assignedData{}, assignedKeys{}, assignedValues{};
    Put(assignedData, 0x10, reinterpret_cast<uintptr_t>(assignedKeys.data()));
    Put(assignedData, 0x18, reinterpret_cast<uintptr_t>(assignedValues.data()));
    Put(assignedKeys, 0x18, uintptr_t(2)); Put(assignedValues, 0x18, uintptr_t(2));
    Put(assignedKeys, 0x20, reinterpret_cast<uintptr_t>(roleIdA.data()));
    Put(assignedKeys, 0x28, reinterpret_cast<uintptr_t>(roleIdB.data()));
    Put(assignedValues, 0x20, int16_t(57)); Put(assignedValues, 0x22, int16_t(0));
    auto assigned = AssignedRoles(reinterpret_cast<uintptr_t>(assignedData.data()));
    assert(assigned.size() == 1 && assigned.at("id-a") == static_cast<RoleType>(57));
    assert(assigned.find("id-b") == assigned.end()); // No role is invented for redacted players.
    Put(assignedValues, 0x18, uintptr_t(1));
    assert(AssignedRoles(reinterpret_cast<uintptr_t>(assignedData.data())).empty());
    Put(assignedValues, 0x18, uintptr_t(2));
    Put(assignedKeys, 0x28, reinterpret_cast<uintptr_t>(roleIdA.data()));
    assert(AssignedRoles(reinterpret_cast<uintptr_t>(assignedData.data())).empty()); // Later redaction revokes the earlier value.
    Put(assignedValues, 0x22, int16_t(2));
    assigned = AssignedRoles(reinterpret_cast<uintptr_t>(assignedData.data()));
    assert(assigned.size() == 1 && assigned.at("id-a") == RoleType::Duck);
    Put(assignedValues, 0x22, int16_t(115));
    assert(AssignedRoles(reinterpret_cast<uintptr_t>(assignedData.data())).empty());
    Put(spectatorInfos[0], 0x10, reinterpret_cast<uintptr_t>(roleIdA.data()));
    Put(spectatorInfos[0], 0x20, int32_t(11));
    Put(spectatorInfos[1], 0x10, reinterpret_cast<uintptr_t>(roleIdB.data()));
    Put(spectatorInfos[1], 0x20, int32_t(2));
    Put(spectatorArray, 0x18, uintptr_t(2));
    Put(spectatorArray, 0x20, reinterpret_cast<uintptr_t>(spectatorInfos[0].data()));
    Put(spectatorArray, 0x28, reinterpret_cast<uintptr_t>(spectatorInfos[1].data()));
    Put(spectatorPacket, 0x18, reinterpret_cast<uintptr_t>(spectatorArray.data()));
    auto packet = reinterpret_cast<uintptr_t>(spectatorPacket.data());
    CaptureSpectatorRoles(packet, 42);
    assert(SpectatorRole("id-a", 42) == RoleType::Sheriff);
    assert(SpectatorRole("id-b", 42) == RoleType::Duck);
    Put(spectatorInfos[1], 0x10, reinterpret_cast<uintptr_t>(roleIdA.data()));
    Put(spectatorInfos[1], 0x20, int32_t(0));
    CaptureSpectatorRoles(packet, 42);
    assert(SpectatorRole("id-a", 42) == RoleType::None);
    Put(spectatorInfos[1], 0x10, reinterpret_cast<uintptr_t>(roleIdB.data()));
    Put(spectatorInfos[1], 0x20, int32_t(2));
    CaptureSpectatorRoles(packet, 42);
    assert(SpectatorRole("id-a", 43) == RoleType::None);
    assert(SpectatorRole("missing", 42) == RoleType::None);
    CaptureSpectatorRoles(0, 42); // Incremental packets preserve the initial roster.
    assert(SpectatorRole("id-a", 42) == RoleType::Sheriff);
    Put(spectatorInfos[0], 0x20, int32_t(0));
    Put(spectatorInfos[1], 0x20, int32_t(65538));
    CaptureSpectatorRoles(packet, 42);
    assert(SpectatorRole("id-a", 42) == RoleType::None);
    assert(SpectatorRole("id-b", 42) == RoleType::None);
    CaptureRevealedRole("id-a", 11, 42);
    CaptureSpectatorRoles(packet, 42); // Redacted roster must not erase an actual reveal.
    assert(SpectatorRole("id-a", 42) == RoleType::Sheriff);
    CaptureRevealedRole("id-a", 0, 42);
    CaptureRevealedRole("id-a", 115, 42);
    CaptureRevealedRole("id-a", 2, 0);
    assert(SpectatorRole("id-a", 42) == RoleType::Sheriff);
    assert(SpectatorRole("id-a", 43) == RoleType::None);
    ClearSpectatorRoles();
    assert(SpectatorRole("id-a", 42) == RoleType::None);
    CaptureRevealedRole("id-a", 11, 42);
    CaptureSpectatorRoles(packet, 43);
    assert(SpectatorRole("id-a", 43) == RoleType::None);
    ClearSpectatorRoles();
    Put(spectatorInfos[0], 0x20, int32_t(11));
    CaptureSpectatorRoles(packet, 0);
    g_RolesReceivedAt = GetTickCount64() - 5001;
    assert(SpectatorRole("id-a", 42) == RoleType::None);
    CaptureSpectatorRoles(packet, 0);
    assert(SpectatorRole("id-a", 42) == RoleType::Sheriff && g_RoleOwner == 42);
    ClearSpectatorRoles();
    assert(SpectatorRole("id-a", 42) == RoleType::None);

    int value = 0;
    assert(!SafeRead<int>(0, &value));
    assert(!SafeRead<int>(1, &value));
    assert(!SafeWrite<int>(1, 42));

    alignas(8) std::array<unsigned char, 1024> entity{};
    uintptr_t object = reinterpret_cast<uintptr_t>(entity.data());
    Put(entity, 0x00, uintptr_t(0x1234));
    Put(entity, 0x10, uintptr_t(123));
    g_Unity.cameraSize = FakeView; g_Unity.setCameraSize = FakeSetView;
    g_OverrideView = FakeOverrideView; g_ResetView = FakeResetView;
    // Built-in render pipelines may never emit the SRP camera callback.
    alignas(8) std::array<unsigned char, 0x200> localBytes{};
    alignas(8) std::array<unsigned char, 0xC0> localClass{};
    alignas(8) std::array<unsigned char, 0x40> localStatics{};
    alignas(8) std::array<unsigned char, 0x100> stateCamera{}, virtualCamera{};
    uintptr_t local = reinterpret_cast<uintptr_t>(localBytes.data());
    Put(localBytes, 0, reinterpret_cast<uintptr_t>(localClass.data()));
    Put(localBytes, 0x10, uintptr_t(456));
    Put(localClass, Offsets::Il2CppClass_StaticFields, reinterpret_cast<uintptr_t>(localStatics.data()));
    Put(localStatics, Offsets::LocalPlayer::Instance, local);
    Put(localStatics, Offsets::LocalPlayer::Player, object);
    Put(localBytes, Offsets::LocalPlayer::mainCamera, object);
    Put(stateCamera, 0x10, uintptr_t(901)); Put(virtualCamera, 0x10, uintptr_t(902));
    Put(localBytes, Offsets::LocalPlayer::stateCamera, reinterpret_cast<uintptr_t>(stateCamera.data()));
    Put(localBytes, Offsets::LocalPlayer::scriptableState, reinterpret_cast<uintptr_t>(virtualCamera.data()));
    Put(entity, Offsets::Entity::isLocal, true);
    LocalContext room;
    assert(ReadLocalContext(local, &room) && room.player == object && room.camera == object);
    Put(localStatics, Offsets::LocalPlayer::Instance, uintptr_t(0));
    assert(!ReadLocalContext(local, &room) && !room.local);
    Put(localStatics, Offsets::LocalPlayer::Instance, local);
    Put(localBytes, 0, reinterpret_cast<uintptr_t>(localClass.data()) | 1);
    assert(!ReadLocalContext(local, &room));
    Put(localBytes, 0, reinterpret_cast<uintptr_t>(localClass.data()));
    g_OriginalSpeedUpdate = FakeMovementSpeed; g_ReadGameFloat = FakeReadGameFloat; g_MakeGameFloat = FakeMakeGameFloat;
    GameRequest speedRequest; speedRequest.speed = true; speedRequest.speedScale = 10;
    RefreshSpeed(local, speedRequest);
    assert(FakeReadGameFloat(SafeReadVal<GameFloat>(reinterpret_cast<uintptr_t>(localStatics.data()) + 0x28), nullptr) == 40);
    const int beforeSpeedUpdates = speedUpdates;
    RefreshSpeed(local, speedRequest);
    assert(speedUpdates == beforeSpeedUpdates); // Repeated frames must not turn 10x into 100x.
    speedRequest.speedScale = 5; RefreshSpeed(local, speedRequest);
    assert(FakeReadGameFloat(SafeReadVal<GameFloat>(reinterpret_cast<uintptr_t>(localStatics.data()) + 0x28), nullptr) == 20);
    speedBase = 2; g_Request = speedRequest; OnMovementSpeed(local, nullptr);
    assert(FakeReadGameFloat(SafeReadVal<GameFloat>(reinterpret_cast<uintptr_t>(localStatics.data()) + 0x28), nullptr) == 10);
    speedRequest.speed = false; RefreshSpeed(local, speedRequest);
    assert(FakeReadGameFloat(SafeReadVal<GameFloat>(reinterpret_cast<uintptr_t>(localStatics.data()) + 0x28), nullptr) == 2);
    assert(g_AppliedSpeedScale == 1);
    RestoreSpeed(); speedBase = 4; g_Request = {};
    g_OriginalSpeedUpdate = nullptr; g_ReadGameFloat = nullptr; g_MakeGameFloat = nullptr;
    g_Unity.worldToScreen = FakeProjection;
    g_OriginalUpdate = FakeUpdate;
    g_FrameWidth = 320; g_FrameHeight = 240;
    g_Request.drone = true; g_Request.viewScale = 2;
    OnLocalUpdate(local, nullptr);
    assert(updateCalls == 1 && g_LocalObject == local);
    assert(testView == 11.0f && g_Frame.sampledAt != 0);
    g_Request.drone = false;
    OnLocalUpdate(local, nullptr);
    assert(testView == 5.5f);
    // A render-thread callback must never claim ownership of Unity operations.
    const DWORD updateThread = g_GameThread.load();
    g_OriginalCamera = FakeCamera;
    std::thread renderThread([&] { OnCameraRendering(0, object, nullptr); });
    renderThread.join();
    assert(cameraCalls == 1 && g_GameThread == updateThread && g_LocalObject == local);
    g_OriginalCamera = nullptr;
    // Scene teardown clears stale data without running features on dead objects.
    Put(localBytes, 0x10, uintptr_t(0));
    g_Request.drone = true;
    OnLocalUpdate(local, nullptr);
    assert(updateCalls == 3 && !g_LocalObject && !g_Frame.sampledAt && testView == 5.5f);
    Put(localBytes, 0x10, uintptr_t(456));
    Put(localStatics, Offsets::LocalPlayer::Player, uintptr_t(0));
    OnLocalUpdate(local, nullptr);
    assert(updateCalls == 4 && !g_LocalObject);
    Put(localStatics, Offsets::LocalPlayer::Player, object);
    Put(localBytes, Offsets::LocalPlayer::mainCamera, uintptr_t(0));
    OnLocalUpdate(local, nullptr);
    assert(updateCalls == 5 && !g_LocalObject);
    Put(localBytes, Offsets::LocalPlayer::mainCamera, object);
    Put(entity, Offsets::Entity::isLocal, false);
    assert(!ReadLocalContext(local, &room));
    Put(entity, Offsets::Entity::isLocal, true);
    g_Request.drone = false;
    OnLocalUpdate(local, nullptr);
    assert(updateCalls == 6 && g_LocalObject == local);
    g_OriginalPreCull = FakePreCull;
    g_Frame.sampledAt = 123;
    OnCameraPreCull(object, nullptr);
    assert(preCullCalls == 1 && g_RenderCamera == object && g_Frame.sampledAt != 123);
    g_Frame.sampledAt = 123;
    OnLocalUpdate(local, nullptr);
    assert(updateCalls == 7 && g_Frame.sampledAt == 123); // Update must not overwrite the final camera sample.
    g_RenderCamera = 0; g_OriginalPreCull = nullptr;
    g_FrameWidth = g_FrameHeight = 0;
    g_LocalObject = 0; g_OriginalUpdate = nullptr;
    ApplyDrone(local, true, 2.0f);
    assert(testView == 11.0f);
    const int beforeViewUpdates = viewOverrides;
    ApplyDrone(local, true, 2.0f);
    assert(testView == 11.0f); // Repeated updates do not multiply the zoom again.
    assert(viewOverrides == beforeViewUpdates);
    Put(virtualCamera, 0xBC, 5.5f); Put(stateCamera, 0xB8, uintptr_t(1234)); // Vote/scene code restored the camera state.
    ApplyDrone(local, true, 2.0f);
    assert(testView == 11.0f && viewOverrides == beforeViewUpdates + 1);
    assert(SafeReadVal<uintptr_t>(reinterpret_cast<uintptr_t>(stateCamera.data()) + 0xB8) == 0);
    const int afterSceneReapply = viewOverrides;
    testView = 8.0f; // Intermediate Cinemachine blend must not become a new base zoom.
    ApplyDrone(local, true, 2.0f);
    assert(g_OriginalView == 5.5f && g_AppliedView == 11.0f && viewOverrides == afterSceneReapply);
    ApplyDrone(0, false, 2.0f); testView = 11.0f; // Reused output camera still contains the old zoom at room entry.
    ApplyDrone(local, true, 2.0f);
    assert(g_OriginalView == 5.5f && g_AppliedView == 11.0f); // No zoom multiplication across rooms.
    ApplyDrone(local, true, 3.0f);
    assert(testView == 16.5f);
    ApplyDrone(local, false, 2.0f);
    assert(testView == 5.5f);
    assert(viewResets > 0);
    // Restore through the game's fog API and preserve an already-disabled fog.
    alignas(8) std::array<unsigned char, 0x1A0> fogBytes{};
    uintptr_t fog = reinterpret_cast<uintptr_t>(fogBytes.data());
    Put(fogBytes, 0, uintptr_t(0x1000)); Put(fogBytes, 0x10, uintptr_t(123));
    g_DisableFog = FakeDisableFog; g_EnableFog = FakeEnableFog;
    ApplyFog(fog); ApplyFog(fog);
    assert(fogDisables == 1 && fogEnables == 0);
    ApplyFog(0);
    assert(fogEnables == 1 && !SafeReadVal<bool>(fog + Offsets::FogOfWarLayout::disabled));
    Put(fogBytes, Offsets::FogOfWarLayout::disabled, true);
    ApplyFog(fog); ApplyFog(0);
    assert(fogDisables == 1 && fogEnables == 1);
    g_DisableFog = nullptr; g_EnableFog = nullptr;
    alignas(8) std::array<unsigned char, 0x20> colliders[4]{};
    g_Unity.isTrigger = FakeTrigger; g_Unity.setTrigger = FakeSetTrigger;
    int colliderIndex = 0;
    for (uintptr_t offset : {0x2D8, 0x2E0, 0x2E8, 0x2F8}) {
        Put(colliders[colliderIndex], 0x10, uintptr_t(colliderIndex + 1));
        Put(entity, offset, reinterpret_cast<uintptr_t>(colliders[colliderIndex].data()));
        ++colliderIndex;
    }
    triggerValues[2] = true;
    ApplyNoClip(object, true);
    for (int i = 1; i <= 4; ++i) assert(triggerValues[i]);
    ApplyNoClip(object, false);
    assert(!triggerValues[1] && triggerValues[2] && !triggerValues[3] && !triggerValues[4]);
    assert(g_Colliders.empty());
    Put(entity, Offsets::Entity::previousPosition, Vector3{0, 0, 0});
    Put(entity, Offsets::Entity::oldTransformPosition, Vector3{999, 999, 999});
    Vector3 position{7, 7, 7};
    g_Unity = {};
    assert(ReadPosition(object, &position));
    assert(position.x == 0 && position.y == 0); // Origin is valid, not a missing sample.
    g_Unity.getTransform = FakeTransform;
    g_Unity.getPosition = FakePosition;
    alignas(8) std::array<unsigned char, 0x20> transformBytes{};
    Put(transformBytes, 0x10, uintptr_t(789));
    transformOwner = object;
    managedTransform = reinterpret_cast<uintptr_t>(transformBytes.data());
    assert(ReadPosition(object, &position) && position.x == 0);
    // Destroyed managed wrappers must fall back without passing handles to native code.
    Put(transformBytes, 0x10, uintptr_t(0));
    assert(ReadPosition(object, &position) && position.x == 0);
    Put(transformBytes, 0x10, uintptr_t(789));
    Put(entity, Offsets::Entity::previousPosition, Vector3{std::numeric_limits<float>::quiet_NaN(), 0, 0});
    g_Unity.getTransform = nullptr;
    assert(!ReadPosition(object, &position));

    // Pixel scaling, Y inversion, off-screen data and behind-camera rejection.
    g_Unity.worldToScreen = FakeProjection;
    Vector2 point{};
    assert(Project(1, {0, 0, 5}, 200, 100, 100, 100, &point));
    assert(point.x == 100 && point.y == 50);
    assert(Project(1, {10, 1, 5}, 200, 100, 100, 100, &point));
    assert(point.x == 300 && point.y == 40); // No line-of-sight clipping in projection.
    assert(!Project(1, {0, 0, -1}, 200, 100, 100, 100, &point));
    assert(!Project(1, {0, 0, 5}, 200, 100, 0, 100, &point));
    std::thread wrongThread([&] {
        Vector2 rejected{};
        assert(!Project(1, {0, 0, 5}, 200, 100, 100, 100, &rejected));
        assert(!ReadPosition(object, &position));
    });
    wrongThread.join();

    // Restore toggles, honor game-owned changes and do not restore reused objects.
    Put(entity, 0x98, false);
    OverrideValue(object, 0x98, true);
    assert(SafeReadVal<bool>(object + 0x98));
    for (auto& f : g_Overrides) f.touched = false;
    RestoreUntouchedOverrides();
    assert(!SafeReadVal<bool>(object + 0x98) && g_Overrides.empty());
    Put(entity, 0x184, 3.0f);
    OverrideValue(object, 0x184, 0.0f);
    Put(entity, 0x184, 7.0f);
    for (auto& f : g_Overrides) f.touched = false;
    RestoreUntouchedOverrides();
    assert(SafeReadVal<float>(object + 0x184) == 7.0f);
    OverrideValue(object, 0x98, true);
    Put(entity, 0x10, uintptr_t(456));
    for (auto& f : g_Overrides) f.touched = false;
    RestoreUntouchedOverrides();
    assert(SafeReadVal<bool>(object + 0x98));

    // More than 64 allocated slots, deleted entries, duplicate values and bad capacity.
    alignas(8) std::array<unsigned char, 0x20 + 65 * 24> entries{};
    alignas(8) std::array<unsigned char, 64> dict{};
    Put(dict, 0x18, reinterpret_cast<uintptr_t>(entries.data()));
    Put(dict, 0x20, int32_t(65));
    Put(entries, 0x18, uintptr_t(65));
    for (int i = 0; i < 65; ++i) {
        Put(entries, 0x20 + i * 24, int32_t(i == 1 ? -1 : i));
        Put(entries, 0x20 + i * 24 + 0x10, uintptr_t(i == 2 ? 1 : i + 1));
    }
    std::vector<uintptr_t> players;
    CollectPlayers(reinterpret_cast<uintptr_t>(dict.data()), players);
    assert(players.size() == 63 && players.back() == 65);
    alignas(8) std::array<unsigned char, 0x400> controller{};
    Put(controller, 0x3F0, reinterpret_cast<uintptr_t>(dict.data()));
    assert(ButtonRegistered(reinterpret_cast<uintptr_t>(controller.data()), 65));
    assert(!ButtonRegistered(reinterpret_cast<uintptr_t>(controller.data()), 999));
    g_ReduceCooldown = FakeReduceCooldown;
    auto ui = reinterpret_cast<uintptr_t>(controller.data());
    ReduceRegisteredCooldown(ui, 65, false);
    ReduceRegisteredCooldown(ui, 999, true);
    assert(cooldownCalls == 0);
    ReduceRegisteredCooldown(ui, 65, true);
    assert(cooldownCalls == 1);
    g_ReduceCooldown = nullptr;
    Put(entries, 0x18, uintptr_t(64));
    players.clear();
    CollectPlayers(reinterpret_cast<uintptr_t>(dict.data()), players);
    assert(players.empty());

    // Auto-fit unions enabled sprite bounds, including a cosmetic above the body.
    alignas(8) std::array<unsigned char, 0x20 + 3 * 8> sprites{};
    alignas(8) std::array<unsigned char, 0x20> renderers[3]{};
    Put(entity, 0x210, reinterpret_cast<uintptr_t>(sprites.data()));
    Put(sprites, 0x18, uintptr_t(3));
    for (int i = 0; i < 3; ++i) {
        Put(sprites, 0x20 + i * 8, reinterpret_cast<uintptr_t>(renderers[i].data()));
        Put(renderers[i], 0x10, uintptr_t(i + 1));
    }
    g_Unity.getBounds = FakeBounds;
    g_Unity.rendererEnabled = FakeEnabled;
    Bounds bounds{};
    assert(ReadSpriteBounds(object, &bounds));
    assert(bounds.center.y == 2 && bounds.extents.y == 2 && bounds.extents.x == 2);

    // Calling cleanup before any graphics initialization, twice, must be harmless.
    Shutdown();
    Shutdown();
    puts("PASS: spectator role IDs/cache lifecycle, managed Transform marshalling, fog API restoration, stable Cinemachine zoom, final-camera sampling, speed 10x, scene teardown/reentry, native cursor/clicks and existing regressions");
}
