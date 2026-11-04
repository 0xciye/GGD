#include "../LoaderCore.h"
#include <cassert>
#include <iostream>

int main() {
    using namespace Loader;
    fs::path temp = fs::temp_directory_path() / (L"34Cheat-tests-" + std::to_wstring(GetCurrentProcessId()));
    fs::create_directories(temp);
    const unsigned char data[]{'a', 'b', 'c'};
    const std::string hash = "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad";
    fs::path dll;
    assert(StagePayload(temp / L"folder with spaces" / L"ti\u1ebfng Vi\u1ec7t", data, sizeof(data), hash, &dll).ok);
    assert(Sha256(dll) == hash);
    assert(dll.is_absolute());
    fs::path same;
    assert(StagePayload(temp / L"folder with spaces" / L"ti\u1ebfng Vi\u1ec7t", data, sizeof(data), hash, &same).ok && dll == same);
    { std::ofstream stale(dll, std::ios::binary | std::ios::trunc); stale << "stale"; }
    assert(StagePayload(temp / L"folder with spaces" / L"ti\u1ebfng Vi\u1ec7t", data, sizeof(data), hash, &same).ok);
    assert(Sha256(dll) == hash);
    assert(!StagePayload(temp, nullptr, sizeof(data), hash, &same).ok);
    assert(!StagePayload(temp, data, sizeof(data), "../bad", &same).ok);
    assert(!StagePayload(temp, data, sizeof(data), std::string(64, '0'), &same).ok);
    assert(!RegularFile(temp / L"missing.exe"));
    assert(!Launch(temp / L"missing.exe", nullptr).ok);
    auto environment = GameEnvironment();
    assert(environment.size() > 2 && environment.back() == 0 && environment[environment.size()-2] == 0);
    bool nullClient = false;
    for (const wchar_t* entry = environment.data(); *entry; entry += wcslen(entry) + 1)
        if (wcscmp(entry, L"EOS_USE_ANTICHEATCLIENTNULL=1") == 0) nullClient = true;
    assert(nullClient);
    assert(Utf8(L"ti\u1ebfng Vi\u1ec7t").find("Vi") != std::string::npos);
    // A controlled target has no GameAssembly module. The DLL's IPC handshake must
    // still complete; the old module-snapshot wait stalled at this exact boundary.
    STARTUPINFOW startup{sizeof(startup)};
    PROCESS_INFORMATION child{};
    auto target = fs::absolute(L"LoaderTarget.exe");
    assert(CreateProcessW(target.c_str(), nullptr, nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &startup, &child));
    std::atomic_bool cancel{false};
    std::vector<std::wstring> stages;
    auto loaded = LoadModule(child.dwProcessId, fs::absolute(L"LoaderRuntime.dll"), cancel,
                             [&](const std::wstring& text) { stages.push_back(text); });
    auto duplicate = LoadModule(child.dwProcessId, fs::absolute(L"LoaderRuntime.dll"), cancel);
    std::wstring stopName = L"Local\\34Cheat.TestStop." + std::to_wstring(child.dwProcessId);
    HANDLE stop = OpenEventW(EVENT_MODIFY_STATE, FALSE, stopName.c_str());
    if (stop) { SetEvent(stop); CloseHandle(stop); }
    WaitForSingleObject(child.hProcess, 5000);
    CloseHandle(child.hThread); CloseHandle(child.hProcess);
    if (!loaded.ok) std::wcerr << loaded.message << '\n';
    assert(loaded.ok && !duplicate.ok);
    assert(std::find(stages.begin(), stages.end(), L"Loading module...") != stages.end());
    // Only remove the uniquely named directory created by this test, inside the temp root.
    assert(fs::equivalent(temp.parent_path(), fs::temp_directory_path()));
    assert(temp.filename().native().rfind(L"34Cheat-tests-", 0) == 0);
    fs::remove_all(temp);
    std::cout << "PASS: loader payload, paths, DLL-owned readiness handshake, duplicate-load guard and progress fixtures\n";
}
