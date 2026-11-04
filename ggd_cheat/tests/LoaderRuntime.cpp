#include <windows.h>
#include <cstdio>
static HANDLE marker = nullptr;
static void Signal(const wchar_t* phase) {
    wchar_t name[96]{};
    swprintf_s(name, L"Local\\34Cheat.%s.%lu", phase, GetCurrentProcessId());
    HANDLE event = OpenEventW(EVENT_MODIFY_STATE, FALSE, name);
    if (event) { SetEvent(event); CloseHandle(event); }
}
static DWORD WINAPI Ready(LPVOID) {
    Sleep(30); Signal(L"Engine");
    Sleep(30); Signal(L"Ready");
    return 0;
}
extern "C" __declspec(dllexport) LRESULT CALLBACK GgdMessageHook(int code, WPARAM w, LPARAM l) {
    return CallNextHookEx(nullptr, code, w, l);
}
BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        HMODULE pinned = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
                           reinterpret_cast<LPCWSTR>(instance), &pinned);
        wchar_t name[96]{};
        swprintf_s(name, L"Local\\34Cheat.Module.%lu", GetCurrentProcessId());
        marker = CreateMutexW(nullptr, FALSE, name);
        HANDLE thread = CreateThread(nullptr, 0, Ready, nullptr, 0, nullptr);
        if (thread) CloseHandle(thread);
    }
    return TRUE;
}
