#pragma once
#define NOMINMAX
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <dwmapi.h>
#include <wincodec.h>
#include <wrl/client.h>
#include <vector>
#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx11.h"
#include "UiTheme.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

class DesktopSurface {
    template <class T> using Ptr = Microsoft::WRL::ComPtr<T>;
    Ptr<ID3D11Device> device;
    Ptr<ID3D11DeviceContext> context;
    Ptr<IDXGISwapChain> swapChain;
    Ptr<ID3D11RenderTargetView> target;
    bool win32Ready = false, dxReady = false;
    bool comOwned = false;
    bool CreateTarget() {
        Ptr<ID3D11Texture2D> buffer;
        return SUCCEEDED(swapChain->GetBuffer(0, IID_PPV_ARGS(&buffer))) &&
               SUCCEEDED(device->CreateRenderTargetView(buffer.Get(), nullptr, &target));
    }
    static LRESULT CALLBACK WindowProc(HWND h, UINT message, WPARAM w, LPARAM l) {
        auto* self = reinterpret_cast<DesktopSurface*>(GetWindowLongPtrW(h, GWLP_USERDATA));
        if (ImGui::GetCurrentContext() && ImGui_ImplWin32_WndProcHandler(h, message, w, l)) return 1;
        if (message == WM_CLOSE) { if (self) self->running = false; return 0; }
        if (message == WM_GETMINMAXINFO) {
            auto* info = reinterpret_cast<MINMAXINFO*>(l);
            info->ptMinTrackSize = {640, 440}; return 0;
        }
        if (message == WM_SIZE && self && self->swapChain && w != SIZE_MINIMIZED) {
            self->context->OMSetRenderTargets(0, nullptr, nullptr);
            self->target.Reset();
            if (SUCCEEDED(self->swapChain->ResizeBuffers(0, LOWORD(l), HIWORD(l), DXGI_FORMAT_UNKNOWN, 0))) self->CreateTarget();
            return 0;
        }
        if (message == WM_SYSCOMMAND && (w & 0xFFF0) == SC_KEYMENU) return 0;
        return DefWindowProcW(h, message, w, l);
    }
public:
    HWND window = nullptr;
    bool running = true;
    ~DesktopSurface() { Shutdown(); }
    bool Initialize(bool visible, int width = 720, int height = 480) {
        HRESULT com = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        comOwned = SUCCEEDED(com);
        ImGui_ImplWin32_EnableDpiAwareness();
        WNDCLASSEXW wc{sizeof(wc)};
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = GetModuleHandleW(nullptr);
        wc.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
        wc.lpszClassName = L"34Cheat.Loader";
        if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) return false;
        RECT rect{0, 0, width, height};
        AdjustWindowRectEx(&rect, WS_OVERLAPPEDWINDOW, FALSE, 0);
        int x = (GetSystemMetrics(SM_CXSCREEN) - (rect.right - rect.left)) / 2;
        int y = (GetSystemMetrics(SM_CYSCREEN) - (rect.bottom - rect.top)) / 2;
        window = CreateWindowExW(0, wc.lpszClassName, L"34Cheat", WS_OVERLAPPEDWINDOW,
                                 x, y, rect.right - rect.left, rect.bottom - rect.top, nullptr, nullptr, wc.hInstance, nullptr);
        if (!window) return false;
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
        BOOL dark = TRUE;
        DwmSetWindowAttribute(window, 20, &dark, sizeof(dark));
        DXGI_SWAP_CHAIN_DESC desc{};
        desc.BufferCount = 2;
        desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.OutputWindow = window;
        desc.SampleDesc.Count = 1;
        desc.Windowed = TRUE;
        auto create = [&](D3D_DRIVER_TYPE driver) {
            return D3D11CreateDeviceAndSwapChain(nullptr, driver, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION,
                                                &desc, &swapChain, &device, nullptr, &context);
        };
        if (FAILED(create(visible ? D3D_DRIVER_TYPE_HARDWARE : D3D_DRIVER_TYPE_WARP))) {
            swapChain.Reset(); context.Reset(); device.Reset();
            if (FAILED(create(D3D_DRIVER_TYPE_WARP))) return false;
        }
        if (!CreateTarget()) return false;
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        ImGui::GetIO().IniFilename = nullptr;
        UiTheme::Apply(); UiTheme::Font();
        win32Ready = ImGui_ImplWin32_Init(window);
        dxReady = win32Ready && ImGui_ImplDX11_Init(device.Get(), context.Get());
        if (!dxReady) return false;
        if (visible) { ShowWindow(window, SW_SHOW); UpdateWindow(window); }
        return true;
    }
    bool Begin() {
        MSG message{};
        while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
            if (message.message == WM_QUIT) running = false;
            TranslateMessage(&message); DispatchMessageW(&message);
        }
        if (!running) return false;
        if (IsIconic(window)) { Sleep(40); return false; }
        if (!target && !CreateTarget()) { Sleep(40); return false; }
        ImGui_ImplDX11_NewFrame(); ImGui_ImplWin32_NewFrame(); ImGui::NewFrame();
        return true;
    }
    bool End(const wchar_t* capture = nullptr) {
        ImGui::Render();
        constexpr float clear[]{0.075f, 0.085f, 0.105f, 1};
        auto* view = target.Get();
        context->OMSetRenderTargets(1, &view, nullptr);
        context->ClearRenderTargetView(view, clear);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        bool saved = !capture || SavePng(capture);
        if (FAILED(swapChain->Present(1, 0))) running = false;
        return saved;
    }
    bool SavePng(const wchar_t* path) {
        Ptr<ID3D11Texture2D> back, staging;
        if (FAILED(swapChain->GetBuffer(0, IID_PPV_ARGS(&back)))) return false;
        D3D11_TEXTURE2D_DESC desc{};
        back->GetDesc(&desc);
        desc.Usage = D3D11_USAGE_STAGING; desc.BindFlags = 0;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ; desc.MiscFlags = 0;
        if (FAILED(device->CreateTexture2D(&desc, nullptr, &staging))) return false;
        context->CopyResource(staging.Get(), back.Get());
        D3D11_MAPPED_SUBRESOURCE mapped{};
        if (FAILED(context->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped))) return false;
        Ptr<IWICImagingFactory> factory;
        Ptr<IWICStream> stream;
        Ptr<IWICBitmapEncoder> encoder;
        Ptr<IWICBitmapFrameEncode> frame;
        HRESULT result = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory));
        if (SUCCEEDED(result)) result = factory->CreateStream(&stream);
        if (SUCCEEDED(result)) result = stream->InitializeFromFilename(path, GENERIC_WRITE);
        if (SUCCEEDED(result)) result = factory->CreateEncoder(GUID_ContainerFormatPng, nullptr, &encoder);
        if (SUCCEEDED(result)) result = encoder->Initialize(stream.Get(), WICBitmapEncoderNoCache);
        if (SUCCEEDED(result)) result = encoder->CreateNewFrame(&frame, nullptr);
        if (SUCCEEDED(result)) result = frame->Initialize(nullptr);
        if (SUCCEEDED(result)) result = frame->SetSize(desc.Width, desc.Height);
        WICPixelFormatGUID format = GUID_WICPixelFormat32bppBGRA;
        if (SUCCEEDED(result)) result = frame->SetPixelFormat(&format);
        std::vector<BYTE> pixels(static_cast<size_t>(desc.Width) * desc.Height * 4);
        for (UINT y = 0; y < desc.Height; ++y) {
            auto* source = static_cast<BYTE*>(mapped.pData) + y * mapped.RowPitch;
            auto* out = pixels.data() + static_cast<size_t>(y) * desc.Width * 4;
            for (UINT x = 0; x < desc.Width; ++x) {
                out[x*4] = source[x*4+2]; out[x*4+1] = source[x*4+1];
                out[x*4+2] = source[x*4]; out[x*4+3] = source[x*4+3];
            }
        }
        if (SUCCEEDED(result)) result = frame->WritePixels(desc.Height, desc.Width * 4, static_cast<UINT>(pixels.size()), pixels.data());
        if (SUCCEEDED(result)) result = frame->Commit();
        if (SUCCEEDED(result)) result = encoder->Commit();
        context->Unmap(staging.Get(), 0);
        return SUCCEEDED(result);
    }
    void Shutdown() {
        if (dxReady) ImGui_ImplDX11_Shutdown();
        if (win32Ready) ImGui_ImplWin32_Shutdown();
        dxReady = win32Ready = false;
        if (ImGui::GetCurrentContext()) ImGui::DestroyContext();
        target.Reset(); swapChain.Reset(); context.Reset(); device.Reset();
        if (window) { DestroyWindow(window); window = nullptr; }
        UnregisterClassW(L"34Cheat.Loader", GetModuleHandleW(nullptr));
        if (comOwned) { CoUninitialize(); comOwned = false; }
    }
};
