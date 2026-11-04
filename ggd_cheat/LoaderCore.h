#pragma once
#define NOMINMAX
#include <windows.h>
#include <tlhelp32.h>
#include <shlobj.h>
#include <shellapi.h>
#include <bcrypt.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <array>
#include <atomic>
#include <algorithm>
#include <functional>
#include "SupportedGame.h"

namespace Loader {
    namespace fs = std::filesystem;
    inline constexpr wchar_t ProcessName[] = L"Goose Goose Duck.exe";
    // The RVAs in this release were checked against this GameAssembly snapshot.
    inline constexpr const char* SupportedGameHash = SupportedGame::Sha256;

    struct Handle {
        HANDLE value = nullptr;
        explicit Handle(HANDLE h = nullptr) : value(h) {}
        ~Handle() { if (value && value != INVALID_HANDLE_VALUE) CloseHandle(value); }
        Handle(const Handle&) = delete;
        Handle& operator=(const Handle&) = delete;
        explicit operator bool() const { return value && value != INVALID_HANDLE_VALUE; }
    };
    struct Result {
        bool ok = false;
        std::wstring message;
        bool restartRequired = false;
    };
    inline std::wstring Error(const wchar_t* action, DWORD code = GetLastError()) {
        wchar_t* buffer = nullptr;
        FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                       nullptr, code, 0, reinterpret_cast<wchar_t*>(&buffer), 0, nullptr);
        std::wstring result = std::wstring(action) + L": " + (buffer ? buffer : L"Windows error");
        if (buffer) LocalFree(buffer);
        while (!result.empty() && (result.back() == L'\r' || result.back() == L'\n' || result.back() == L' ')) result.pop_back();
        return result;
    }
    inline std::string Utf8(const std::wstring& s) {
        if (s.empty()) return {};
        int n = WideCharToMultiByte(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0, nullptr, nullptr);
        std::string out(n, '\0');
        if (n) WideCharToMultiByte(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), out.data(), n, nullptr, nullptr);
        return out;
    }
    inline fs::path DataDirectory() {
        wchar_t* path = nullptr;
        if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &path))) {
            fs::path result = fs::path(path) / L"34Cheat";
            CoTaskMemFree(path);
            return result;
        }
        wchar_t temp[MAX_PATH]{};
        if (!GetTempPathW(MAX_PATH, temp)) return {};
        return fs::path(temp) / L"34Cheat";
    }
    inline bool RegularFile(const fs::path& path) {
        std::error_code ec;
        return fs::is_regular_file(path, ec);
    }
    inline DWORD ProcessId() {
        Handle snapshot(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));
        if (!snapshot) return 0;
        PROCESSENTRY32W entry{sizeof(entry)};
        if (Process32FirstW(snapshot.value, &entry)) do {
            if (_wcsicmp(entry.szExeFile, ProcessName) == 0) return entry.th32ProcessID;
        } while (Process32NextW(snapshot.value, &entry));
        return 0;
    }
    inline fs::path ProcessPath(DWORD pid) {
        Handle process(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid));
        std::wstring buffer(32768, L'\0');
        DWORD count = static_cast<DWORD>(buffer.size());
        if (!process || !QueryFullProcessImageNameW(process.value, 0, buffer.data(), &count)) return {};
        buffer.resize(count);
        return buffer;
    }
    inline fs::path RegistryPath(HKEY root, const wchar_t* keyName, const wchar_t* valueName, REGSAM view = 0) {
        HKEY key = nullptr;
        if (RegOpenKeyExW(root, keyName, 0, KEY_READ | view, &key) != ERROR_SUCCESS) return {};
        wchar_t value[32768]{};
        DWORD count = sizeof(value), type = 0;
        LSTATUS result = RegQueryValueExW(key, valueName, nullptr, &type, reinterpret_cast<BYTE*>(value), &count);
        RegCloseKey(key);
        if (result != ERROR_SUCCESS || (type != REG_SZ && type != REG_EXPAND_SZ)) return {};
        value[32767] = 0;
        if (type == REG_EXPAND_SZ) {
            wchar_t expanded[32768]{};
            DWORD n = ExpandEnvironmentStringsW(value, expanded, 32768);
            return n && n <= 32768 ? fs::path(expanded) : fs::path{};
        }
        return value;
    }
    inline fs::path FindGame() {
        if (DWORD pid = ProcessId()) {
            auto path = ProcessPath(pid);
            if (RegularFile(path)) return path;
        }
        auto dir = DataDirectory();
        if (!dir.empty()) {
            wchar_t saved[32768]{};
            GetPrivateProfileStringW(L"Game", L"Path", L"", saved, 32768, (dir / L"settings.ini").c_str());
            if (RegularFile(saved)) return saved;
        }
        const wchar_t* uninstall = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Steam App 1568590";
        for (HKEY root : {HKEY_CURRENT_USER, HKEY_LOCAL_MACHINE}) {
            for (REGSAM view : {REGSAM(KEY_WOW64_32KEY), REGSAM(KEY_WOW64_64KEY)}) {
                fs::path install = RegistryPath(root, uninstall, L"InstallLocation", view);
                if (!install.empty() && RegularFile(install / ProcessName)) return install / ProcessName;
            }
        }
        fs::path steam = RegistryPath(HKEY_CURRENT_USER, L"Software\\Valve\\Steam", L"SteamPath");
        if (!steam.empty() && RegularFile(steam / L"steamapps/common/Goose Goose Duck" / ProcessName))
            return steam / L"steamapps/common/Goose Goose Duck" / ProcessName;
        return {};
    }
    inline void SaveGamePath(const fs::path& game) {
        auto dir = DataDirectory();
        if (dir.empty()) return;
        std::error_code ec;
        fs::create_directories(dir, ec);
        if (!ec) WritePrivateProfileStringW(L"Game", L"Path", game.c_str(), (dir / L"settings.ini").c_str());
    }
    inline std::string Sha256(const fs::path& path) {
        std::ifstream input(path, std::ios::binary);
        if (!input) return {};
        BCRYPT_ALG_HANDLE algorithm = nullptr;
        BCRYPT_HASH_HANDLE hash = nullptr;
        if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) < 0) return {};
        if (BCryptCreateHash(algorithm, &hash, nullptr, 0, nullptr, 0, 0) < 0) {
            BCryptCloseAlgorithmProvider(algorithm, 0); return {};
        }
        std::array<char, 65536> block{};
        bool ok = true;
        while (input.read(block.data(), block.size()) || input.gcount()) {
            if (BCryptHashData(hash, reinterpret_cast<PUCHAR>(block.data()), static_cast<ULONG>(input.gcount()), 0) < 0) { ok = false; break; }
        }
        std::array<unsigned char, 32> digest{};
        if (input.bad() || BCryptFinishHash(hash, digest.data(), static_cast<ULONG>(digest.size()), 0) < 0) ok = false;
        BCryptDestroyHash(hash); BCryptCloseAlgorithmProvider(algorithm, 0);
        if (!ok) return {};
        const char* hex = "0123456789abcdef";
        std::string result;
        for (auto c : digest) { result += hex[c >> 4]; result += hex[c & 15]; }
        return result;
    }
    inline Result StagePayload(const fs::path& directory, const unsigned char* bytes, size_t size, const std::string& hash, fs::path* output) {
        if (directory.empty() || !bytes || !size || hash.size() != 64 || hash.find_first_not_of("0123456789abcdef") != std::string::npos)
            return {false, L"The bundled module is invalid."};
        fs::path folder = directory / L"Runtime" / hash;
        std::error_code ec;
        fs::create_directories(folder, ec);
        if (ec) return {false, L"Cannot create the runtime folder. Check folder permissions."};
        fs::path target = folder / L"34Cheat.dll";
        if (Sha256(target) != hash) {
            fs::path temp = folder / (L"payload-" + std::to_wstring(GetCurrentProcessId()) + L".tmp");
            std::ofstream file(temp, std::ios::binary | std::ios::trunc);
            if (!file || !file.write(reinterpret_cast<const char*>(bytes), static_cast<std::streamsize>(size)))
                return {false, L"Cannot write the runtime module. Check folder permissions."};
            file.close();
            if (!file || Sha256(temp) != hash) { fs::remove(temp, ec); return {false, L"The runtime module failed its integrity check."}; }
            if (!MoveFileExW(temp.c_str(), target.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
                DWORD code = GetLastError();
                fs::remove(temp, ec); return {false, Error(L"Cannot install the runtime module", code)};
            }
        }
        *output = target;
        return {true, L"Module ready."};
    }
    inline bool LoadedModule(DWORD pid, const wchar_t* name) {
        Handle snapshot(CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid));
        if (!snapshot) return false;
        MODULEENTRY32W entry{sizeof(entry)};
        if (Module32FirstW(snapshot.value, &entry)) do {
            if (_wcsicmp(entry.szModule, name) == 0) return true;
        } while (Module32NextW(snapshot.value, &entry));
        return false;
    }
    inline bool WaitForModule(DWORD pid, std::atomic_bool& cancel) {
        ULONGLONG start = GetTickCount64();
        while (!cancel && GetTickCount64() - start < 45000) {
            if (LoadedModule(pid, L"GameAssembly.dll")) return true;
            Handle process(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid));
            DWORD code = 0;
            if (!process || !GetExitCodeProcess(process.value, &code) || code != STILL_ACTIVE) return false;
            Sleep(100);
        }
        return false;
    }
    inline DWORD WindowThread(DWORD pid, HWND* selected = nullptr) {
        struct Search { DWORD pid, fallback = 0, visible = 0; HWND window = nullptr; } search{pid};
        EnumWindows([](HWND window, LPARAM data) -> BOOL {
            auto* found = reinterpret_cast<Search*>(data);
            DWORD owner = 0;
            DWORD thread = GetWindowThreadProcessId(window, &owner);
            if (owner != found->pid || GetWindow(window, GW_OWNER)) return TRUE;
            found->fallback = thread;
            found->window = window;
            if (IsWindowVisible(window)) { found->visible = thread; return FALSE; }
            return TRUE;
        }, reinterpret_cast<LPARAM>(&search));
        if (selected) *selected = search.window;
        return search.visible ? search.visible : search.fallback;
    }
    inline bool SessionAllowsInternal(DWORD pid) {
        Handle process(OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, FALSE, pid));
        if (!process) return false;
        using QueryObject = LONG (WINAPI*)(HANDLE, ULONG, void*, ULONG, ULONG*);
        auto query = reinterpret_cast<QueryObject>(GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "NtQueryObject"));
        if (!query) return true;
        struct BasicInfo { ULONG attributes, access, handleCount, pointerCount, reserved[10]; } info{};
        ULONG needed = 0;
        if (query(process.value, 0, &info, sizeof(info), &needed) < 0) return true;
        return (info.access & (PROCESS_VM_OPERATION | PROCESS_VM_WRITE)) == (PROCESS_VM_OPERATION | PROCESS_VM_WRITE);
    }
    inline Result LoadModule(DWORD pid, const fs::path& dll, std::atomic_bool& cancel,
                             const std::function<void(const std::wstring&)>& progress = {}) {
        std::wstring suffix = std::to_wstring(pid);
        Handle existing(OpenMutexW(SYNCHRONIZE, FALSE, (L"Local\\34Cheat.Module." + suffix).c_str()));
        if (existing || LoadedModule(pid, L"34Cheat.dll"))
            return {false, L"34Cheat is already loaded. Restart the game before loading a new version.", true};
        if (!SessionAllowsInternal(pid))
            return {false, L"This session blocks internal loading. Use Restart & load.", true};
        Handle process(OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, FALSE, pid));
        if (!process) return {false, Error(L"Cannot access the game")};
        BOOL wow64 = FALSE;
        if (!IsWow64Process(process.value, &wow64) || wow64) return {false, L"This loader requires the 64-bit game."};
        Handle ready(CreateEventW(nullptr, TRUE, FALSE, (L"Local\\34Cheat.Ready." + suffix).c_str()));
        Handle engine(CreateEventW(nullptr, TRUE, FALSE, (L"Local\\34Cheat.Engine." + suffix).c_str()));
        Handle failed(CreateEventW(nullptr, TRUE, FALSE, (L"Local\\34Cheat.Failed." + suffix).c_str()));
        if (!ready || !engine || !failed) return {false, Error(L"Cannot initialize the loader session")};
        ResetEvent(ready.value); ResetEvent(engine.value); ResetEvent(failed.value);
        size_t bytes = (dll.native().size() + 1) * sizeof(wchar_t);
        void* remote = VirtualAllocEx(process.value, nullptr, bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!remote) return {false, Error(L"Cannot prepare the module path"), true};
        SIZE_T written = 0;
        if (!WriteProcessMemory(process.value, remote, dll.c_str(), bytes, &written) || written != bytes) {
            auto message = Error(L"Cannot copy the module path");
            VirtualFreeEx(process.value, remote, 0, MEM_RELEASE); return {false, message};
        }
        auto load = GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "LoadLibraryW");
        if (!load) { VirtualFreeEx(process.value, remote, 0, MEM_RELEASE); return {false, L"Windows module loader is unavailable."}; }
        if (progress) progress(L"Loading module...");
        Handle thread(CreateRemoteThread(process.value, nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(load), remote, 0, nullptr));
        if (!thread) {
            auto message = Error(L"Windows blocked loading the module");
            VirtualFreeEx(process.value, remote, 0, MEM_RELEASE); return {false, message, true};
        }
        ULONGLONG start = GetTickCount64();
        bool complete = false;
        while (!cancel && GetTickCount64() - start < 30000) {
            if (WaitForSingleObject(thread.value, 100) == WAIT_OBJECT_0) { complete = true; break; }
        }
        if (!complete) return {false, cancel ? L"Cancelled. Restart the game before retrying." : L"Module loading timed out. Restart the game and try again.", true};
        VirtualFreeEx(process.value, remote, 0, MEM_RELEASE);
        DWORD code = 0;
        if (!GetExitCodeThread(thread.value, &code) || code == 0) return {false, L"The game could not load the module. Restart and try again.", true};
        if (progress) progress(L"Waiting for game engine...");
        bool engineSeen = false;
        start = GetTickCount64();
        while (!cancel && GetTickCount64() - start < 45000) {
            if (WaitForSingleObject(ready.value, 100) == WAIT_OBJECT_0)
                return {true, L"Loaded. Press Insert in the game."};
            if (WaitForSingleObject(failed.value, 0) == WAIT_OBJECT_0)
                return {false, L"Menu initialization failed. Restart the game. Details are in runtime.log.", true};
            if (!engineSeen && WaitForSingleObject(engine.value, 0) == WAIT_OBJECT_0) {
                engineSeen = true;
                if (progress) progress(L"Preparing in-game menu...");
            }
            if (!GetExitCodeProcess(process.value, &code) || code != STILL_ACTIVE)
                return {false, L"The game closed before the menu was ready. Open it and try again."};
        }
        return {false, cancel ? L"Cancelled. Restart the game before retrying." :
            engineSeen ? L"The menu did not render. Bring the game window to the front and restart before retrying." :
                         L"The game engine did not initialize. Restart the game and try again.", true};
    }
    inline std::vector<wchar_t> GameEnvironment() {
        wchar_t* environment = GetEnvironmentStringsW();
        if (!environment) return {};
        std::vector<std::wstring> values;
        for (const wchar_t* p = environment; *p; p += wcslen(p) + 1) {
            if (_wcsnicmp(p, L"EOS_USE_ANTICHEATCLIENTNULL=", 28) &&
                _wcsnicmp(p, L"SteamAppId=", 11) && _wcsnicmp(p, L"SteamGameId=", 12)) values.emplace_back(p);
        }
        FreeEnvironmentStringsW(environment);
        values.emplace_back(L"EOS_USE_ANTICHEATCLIENTNULL=1");
        values.emplace_back(L"SteamAppId=1568590");
        values.emplace_back(L"SteamGameId=1568590");
        std::sort(values.begin(), values.end(), [](const std::wstring& a, const std::wstring& b) { return _wcsicmp(a.c_str(), b.c_str()) < 0; });
        std::vector<wchar_t> block;
        for (const auto& value : values) { block.insert(block.end(), value.begin(), value.end()); block.push_back(0); }
        block.push_back(0);
        return block;
    }
    inline Result Launch(const fs::path& game, DWORD* pid) {
        if (!pid) return {false, L"The launch request is invalid."};
        if (!RegularFile(game) || _wcsicmp(game.filename().c_str(), ProcessName)) return {false, L"Choose Goose Goose Duck.exe."};
        std::wstring command = L"\"" + game.native() + L"\"";
        STARTUPINFOW startup{sizeof(startup)};
        PROCESS_INFORMATION process{};
        auto environment = GameEnvironment();
        if (environment.empty()) return {false, L"Cannot prepare the game environment."};
        if (!CreateProcessW(game.c_str(), command.data(), nullptr, nullptr, FALSE, CREATE_UNICODE_ENVIRONMENT, environment.data(), game.parent_path().c_str(), &startup, &process))
            return {false, Error(L"Cannot start the game")};
        *pid = process.dwProcessId;
        CloseHandle(process.hThread); CloseHandle(process.hProcess);
        return {true, L"Starting game..."};
    }
    inline Result LaunchGame(const fs::path& game, DWORD* pid, std::atomic_bool& cancel) {
        if (cancel) return {false, L"Cancelled."};
        return Launch(game, pid);
    }
    inline Result CloseForRestart(DWORD pid, std::atomic_bool& cancel) {
        HWND window = nullptr;
        WindowThread(pid, &window);
        if (!window || !PostMessageW(window, WM_CLOSE, 0, 0)) return {false, L"Close the game manually, then click Launch & load."};
        Handle process(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid));
        if (!process) return {false, L"Close the game manually, then click Launch & load."};
        ULONGLONG start = GetTickCount64();
        while (!cancel && GetTickCount64() - start < 15000) {
            DWORD code = 0;
            if (GetExitCodeProcess(process.value, &code) && code != STILL_ACTIVE) return {true, L"Game closed."};
            Sleep(100);
        }
        return {false, cancel ? L"Cancelled." : L"Close the game manually, then click Launch & load."};
    }
}
