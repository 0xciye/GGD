// Portable Windows loader. The runtime DLL is embedded at build time.
#include "LoaderCore.h"
#include "DesktopSurface.h"
#include "EmbeddedDll.h"
#include <commdlg.h>
#include <thread>
#include <mutex>
#include <shellapi.h>

namespace {
    struct Operation {
        std::atomic_bool busy{false}, cancel{false};
        std::mutex mutex;
        std::wstring message;
        bool error = false;
        bool restartRequired = false;
        DWORD loadedPid = 0;
        bool logEnabled = false;
        std::thread worker;
        void Status(const std::wstring& text, bool failed = false) {
            {
                std::lock_guard<std::mutex> lock(mutex);
                message = text; error = failed;
            }
            if (logEnabled && !text.empty()) {
                auto dir = Loader::DataDirectory();
                std::error_code ec;
                if (!dir.empty()) Loader::fs::create_directories(dir, ec);
                if (!dir.empty() && !ec) {
                    SYSTEMTIME time{}; GetSystemTime(&time);
                    std::ofstream log(dir / L"loader.log", std::ios::app);
                    log << time.wYear << '-' << time.wMonth << '-' << time.wDay << ' '
                        << time.wHour << ':' << time.wMinute << ':' << time.wSecond << " UTC  "
                        << Loader::Utf8(text) << '\n';
                }
            }
        }
        ~Operation() { cancel = true; if (worker.joinable()) worker.join(); }
    };

    Loader::fs::path Browse(HWND window) {
        wchar_t path[32768]{};
        OPENFILENAMEW dialog{sizeof(dialog)};
        dialog.hwndOwner = window;
        dialog.lpstrFilter = L"Goose Goose Duck.exe\0Goose Goose Duck.exe\0Executables\0*.exe\0\0";
        dialog.lpstrFile = path; dialog.nMaxFile = 32768;
        dialog.lpstrTitle = L"Select Goose Goose Duck.exe";
        dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
        return GetOpenFileNameW(&dialog) ? Loader::fs::path(path) : Loader::fs::path{};
    }

    void StartLoad(Operation& state, Loader::fs::path selected, bool restart = false) {
        if (state.busy.exchange(true)) return;
        if (state.worker.joinable()) state.worker.join();
        state.cancel = false;
        state.Status(L"Checking game...");
        state.worker = std::thread([&state, selected, restart] {
            try {
                DWORD pid = Loader::ProcessId();
                auto game = pid ? Loader::ProcessPath(pid) : selected;
                Loader::Result result;
                if (!Loader::RegularFile(game) || _wcsicmp(game.filename().c_str(), Loader::ProcessName))
                    result = {false, L"Choose Goose Goose Duck.exe to continue."};
                else if (Loader::Sha256(game.parent_path() / L"GameAssembly.dll") != Loader::SupportedGameHash)
                    result = {false, L"This game version is not supported. Update 34Cheat before loading."};
                else {
                    if (restart && pid) {
                        state.Status(L"Closing game...");
                        result = Loader::CloseForRestart(pid, state.cancel);
                        if (!result.ok) { state.Status(result.message, true); state.busy = false; return; }
                        pid = 0;
                    }
                    state.Status(L"Preparing module...");
                    Loader::fs::path dll;
                    result = Loader::StagePayload(Loader::DataDirectory(), g_EmbeddedDllData, g_EmbeddedDllSize, g_EmbeddedDllSha256, &dll);
                    if (result.ok && !state.cancel) {
                        if (!pid) {
                            state.Status(L"Starting game...");
                            result = Loader::LaunchGame(game, &pid, state.cancel);
                        }
                        if (result.ok && !state.cancel) {
                            state.Status(L"Loading...");
                            auto runningGame = Loader::ProcessPath(pid);
                            if (runningGame.empty() || Loader::Sha256(runningGame.parent_path() / L"GameAssembly.dll") != Loader::SupportedGameHash)
                                result = {false, L"The running game version is not supported. Update 34Cheat before loading."};
                            else result = Loader::LoadModule(pid, dll, state.cancel, [&](const std::wstring& message) { state.Status(message); });
                        }
                    }
                }
                if (state.cancel) result = {false, L"Cancelled."};
                {
                    std::lock_guard<std::mutex> lock(state.mutex);
                    if (result.ok) state.loadedPid = pid;
                    state.restartRequired = result.restartRequired;
                }
                state.Status(result.message, !result.ok);
            } catch (const std::exception&) {
                state.Status(L"Loading failed. Check folder permissions and try again.", true);
            }
            state.busy = false;
        });
    }

    void DrawLoader(DesktopSurface& surface, Operation& state, Loader::fs::path& game, DWORD pid) {
        auto& io = ImGui::GetIO();
        ImGui::SetNextWindowPos({0, 0}); ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
        ImGui::Begin("Loader", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
        ImGui::PushFont(nullptr, 30);
        ImGui::TextUnformatted("34Cheat");
        ImGui::PopFont();
        ImGui::TextDisabled("Goose Goose Duck");
        ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
        ImGui::TextUnformatted("Game");
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - 90);
        ImGui::BeginDisabled(state.busy);
        if (ImGui::SmallButton("Browse...")) {
            auto chosen = Browse(surface.window);
            if (!chosen.empty()) {
                if (_wcsicmp(chosen.filename().c_str(), Loader::ProcessName)) state.Status(L"Select Goose Goose Duck.exe.", true);
                else { game = chosen; Loader::SaveGamePath(game); state.Status(L""); }
            }
        }
        ImGui::EndDisabled();
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
        ImGui::BeginChild("GamePath", {0, 76}, ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding);
        if (game.empty()) ImGui::TextDisabled("Select the game executable to continue.");
        else ImGui::TextWrapped("%s", Loader::Utf8(game.native()).c_str());
        ImGui::EndChild(); ImGui::PopStyleColor();
        ImGui::Spacing();
        std::wstring message; bool error = false, restartRequired = false; DWORD loaded = 0;
        { std::lock_guard<std::mutex> lock(state.mutex); message = state.message; error = state.error; restartRequired = state.restartRequired; loaded = state.loadedPid; }
        bool alreadyLoaded = pid && pid == loaded;
        const char* status = state.busy ? "Loading" : alreadyLoaded ? "Loaded" : pid ? "Ready to load" : "Not running";
        ImGui::TextUnformatted(status);
        if (!message.empty()) {
            ImGui::PushStyleColor(ImGuiCol_Text, error ? ImVec4(1.0f, 0.68f, 0.64f, 1) : ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
            ImGui::TextWrapped("%s", Loader::Utf8(message).c_str());
            ImGui::PopStyleColor();
            if (error && ImGui::SmallButton("Copy error")) ImGui::SetClipboardText(Loader::Utf8(message).c_str());
        }
        ImGui::SetCursorPosY((std::max)(ImGui::GetCursorPosY() + 20, io.DisplaySize.y - 74));
        ImGui::BeginDisabled(state.busy || alreadyLoaded || (!pid && game.empty()));
        if (ImGui::Button(pid ? (restartRequired ? "Restart & load" : "Load") : "Launch & load", {180, 42})) StartLoad(state, game, pid && restartRequired);
        ImGui::EndDisabled();
        if (state.busy) {
            ImGui::SameLine();
            if (ImGui::Button("Cancel", {100, 42})) state.cancel = true;
        }
        ImGui::SameLine();
        ImGui::TextDisabled("Insert opens the in-game menu.");
        ImGui::End(); ImGui::PopStyleVar();
    }
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    int argc = 0;
    wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    bool snapshot = argc >= 3 && wcscmp(argv[1], L"--snapshot") == 0;
    std::wstring capture = snapshot ? argv[2] : L"";
    std::wstring previewState = snapshot && argc >= 4 ? argv[3] : L"";
    int width = snapshot && argc >= 5 ? _wtoi(argv[4]) : 720;
    int height = snapshot && argc >= 6 ? _wtoi(argv[5]) : 480;
    bool selfTest = argc >= 3 && wcscmp(argv[1], L"--self-test") == 0;
    std::wstring testFolder = selfTest ? argv[2] : L"";
    if (argv) LocalFree(argv);
    if (width < 640 || width > 3840 || height < 440 || height > 2160) return 2;
    if (selfTest) {
        Loader::fs::path dll;
        auto result = Loader::StagePayload(testFolder, g_EmbeddedDllData, g_EmbeddedDllSize, g_EmbeddedDllSha256, &dll);
        if (!result.ok || Loader::Sha256(dll) != g_EmbeddedDllSha256) return 1;
        std::ofstream report(Loader::fs::path(testFolder) / L"self-test.txt");
        report << "PASS: embedded payload staged and verified without project files\n" << g_EmbeddedDllSha256 << "\n";
        return report ? 0 : 1;
    }
    DesktopSurface surface;
    if (!surface.Initialize(!snapshot, width, height)) {
        if (!snapshot) MessageBoxW(nullptr, L"The interface could not start. Check your graphics driver and try again.", L"34Cheat", MB_ICONERROR);
        return 1;
    }
    Operation operation;
    operation.logEnabled = !snapshot;
    auto game = Loader::FindGame();
    DWORD pid = 0;
    if (snapshot) {
        if (previewState == L"empty") game.clear();
        else if (previewState == L"error") operation.Status(L"This game version is not supported. Update 34Cheat before loading.", true);
        else if (previewState == L"loading") { operation.Status(L"Waiting for the game..."); operation.busy = true; }
        else if (previewState == L"loaded") { operation.Status(L"Loaded. Press Insert in the game."); pid = operation.loadedPid = 1; }
    }
    ULONGLONG lastPoll = 0;
    int frames = 0;
    while (surface.running) {
        if (!snapshot && GetTickCount64() - lastPoll > 1000) {
            pid = Loader::ProcessId(); lastPoll = GetTickCount64();
        }
        if (!surface.Begin()) continue;
        DrawLoader(surface, operation, game, pid);
        bool last = snapshot && ++frames == 4;
        if (!surface.End(last ? capture.c_str() : nullptr)) return 1;
        if (last) break;
    }
    operation.cancel = true;
    return 0;
}
