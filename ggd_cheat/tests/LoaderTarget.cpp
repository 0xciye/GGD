#include <windows.h>
#include <cstdio>
int main() {
    wchar_t name[96]{};
    swprintf_s(name, L"Local\\34Cheat.TestStop.%lu", GetCurrentProcessId());
    HANDLE stop = CreateEventW(nullptr, TRUE, FALSE, name);
    if (!stop) return 1;
    HWND window = CreateWindowExW(0, L"STATIC", L"34Cheat test target", WS_POPUP, 0, 0, 100, 100,
                                   nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
    ULONGLONG start = GetTickCount64();
    while (WaitForSingleObject(stop, 0) != WAIT_OBJECT_0 && GetTickCount64() - start < 90000) {
        MSG message{};
        while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&message); DispatchMessageW(&message);
        }
        MsgWaitForMultipleObjects(1, &stop, FALSE, 100, QS_ALLINPUT);
    }
    DestroyWindow(window);
    CloseHandle(stop);
    return 0;
}
