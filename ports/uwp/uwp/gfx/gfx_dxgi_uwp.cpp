#define DECLARE_GFX_DXGI_FUNCTIONS

#include <cstdio>
#include <cstring>

#include <windows.h>
#include <wrl/client.h>
#include <dxgi1_3.h>
#include <d3d11.h>

#include <winrt/Windows.UI.Core.h>

#include "SDL.h"

#include "PR/ultratypes.h"
#include "macros.h"
#include "pc/gfx/gfx_dxgi.h"
#include "pc/gfx/gfx_window_manager_api.h"
#include "uwp_display_size.h"
#include "uwp_input_pane.h"

using Microsoft::WRL::ComPtr;
using winrt::Windows::UI::Core::CoreProcessEventsOption;
using winrt::Windows::UI::Core::CoreWindow;

extern "C" void game_exit(void);

static struct {
    ComPtr<IDXGIFactory2> factory;
    ComPtr<IDXGISwapChain1> swap_chain;
    HANDLE waitable_object;
    uint32_t width;
    uint32_t height;
} s_dxgi;

static void gfx_dxgi_refresh_dimensions(void) {
    int width = 0;
    int height = 0;
    if (sm64coopdx_uwp_get_render_size(&width, &height)) {
        s_dxgi.width = (uint32_t)width;
        s_dxgi.height = (uint32_t)height;
    } else {
        s_dxgi.width = 1920;
        s_dxgi.height = 1080;
    }
}

static void gfx_dxgi_init(UNUSED const char *window_title) {
    SDL_SetHint(SDL_HINT_WINRT_HANDLE_BACK_BUTTON, "1");
    SDL_Init(SDL_INIT_VIDEO);
    gfx_dxgi_refresh_dimensions();
}

static void gfx_dxgi_set_keyboard_callbacks(bool (*on_key_down)(int scancode), bool (*on_key_up)(int scancode), void (*on_all_keys_up)(void),
                                            void (*on_text_input)(char*), void (*on_text_editing)(char*, int)) {
    uwp_input_set_keyboard_callbacks(on_key_down, on_key_up, on_all_keys_up, on_text_input, on_text_editing);
}

static void gfx_dxgi_set_scroll_callback(void (*on_scroll)(float, float)) {
    uwp_input_set_scroll_callback(on_scroll);
}

static void gfx_dxgi_main_loop(void (*run_one_game_iter)(void)) {
    run_one_game_iter();
}

static void gfx_dxgi_get_dimensions(uint32_t *width, uint32_t *height) {
    if (width) *width = s_dxgi.width;
    if (height) *height = s_dxgi.height;
}

static void gfx_dxgi_handle_events(void) {
    try {
        CoreWindow::GetForCurrentThread().Dispatcher().ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);
    } catch (...) {
    }

    uwp_input_pump_sdl_events(game_exit);
}

static bool gfx_dxgi_start_frame(void) {
    return true;
}

static void gfx_dxgi_swap_buffers_begin(void) {
}

static void gfx_dxgi_swap_buffers_end(void) {
    if (s_dxgi.swap_chain) {
        ThrowIfFailed(s_dxgi.swap_chain->Present(1, 0));
        if (s_dxgi.waitable_object) {
            WaitForSingleObjectEx(s_dxgi.waitable_object, 1000, TRUE);
        }
    }
}

static double gfx_dxgi_get_time(void) {
    LARGE_INTEGER frequency;
    LARGE_INTEGER counter;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart / (double)frequency.QuadPart;
}

void gfx_dxgi_create_factory_and_device(bool debug, UNUSED int d3d_version, bool (*create_device_fn)(IDXGIAdapter1 *adapter, bool test_only)) {
    UINT flags = debug ? DXGI_CREATE_FACTORY_DEBUG : 0;
    HRESULT hr = CreateDXGIFactory2(flags, IID_PPV_ARGS(s_dxgi.factory.GetAddressOf()));
    if (FAILED(hr)) {
        ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(s_dxgi.factory.GetAddressOf())));
    }

    ComPtr<IDXGIAdapter1> adapter;
    ComPtr<IDXGIAdapter1> selected_adapter;
    for (UINT i = 0; s_dxgi.factory->EnumAdapters1(i, adapter.ReleaseAndGetAddressOf()) != DXGI_ERROR_NOT_FOUND; i++) {
        DXGI_ADAPTER_DESC1 desc;
        ThrowIfFailed(adapter->GetDesc1(&desc));
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
            continue;
        }
        if (create_device_fn(adapter.Get(), true)) {
            selected_adapter = adapter;
            break;
        }
    }

    if (!selected_adapter) {
        ThrowIfFailed(DXGI_ERROR_UNSUPPORTED);
    }

    create_device_fn(selected_adapter.Get(), false);
}

ComPtr<IDXGISwapChain1> gfx_dxgi_create_swap_chain(IUnknown *device) {
    gfx_dxgi_refresh_dimensions();

    DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {};
    swap_chain_desc.BufferCount = 2;
    swap_chain_desc.Width = s_dxgi.width;
    swap_chain_desc.Height = s_dxgi.height;
    swap_chain_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_chain_desc.Scaling = DXGI_SCALING_STRETCH;
    swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    swap_chain_desc.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
    swap_chain_desc.SampleDesc.Count = 1;

    IUnknown *core_window = reinterpret_cast<IUnknown *>(winrt::get_abi(CoreWindow::GetForCurrentThread()));
    ThrowIfFailed(s_dxgi.factory->CreateSwapChainForCoreWindow(device, core_window, &swap_chain_desc, nullptr, s_dxgi.swap_chain.GetAddressOf()));

    ComPtr<IDXGISwapChain2> swap_chain2;
    if (SUCCEEDED(s_dxgi.swap_chain.As(&swap_chain2))) {
        ThrowIfFailed(swap_chain2->SetMaximumFrameLatency(1));
        s_dxgi.waitable_object = swap_chain2->GetFrameLatencyWaitableObject();
    }

    ThrowIfFailed(s_dxgi.swap_chain->GetDesc1(&swap_chain_desc));
    s_dxgi.width = swap_chain_desc.Width;
    s_dxgi.height = swap_chain_desc.Height;
    return s_dxgi.swap_chain;
}

void gfx_dxgi_delay(u32 ms) {
    SDL_Delay(ms);
}

static int gfx_dxgi_get_max_msaa(void) {
    return 1;
}

static void gfx_dxgi_set_window_title(UNUSED const char *title) {
}

static void gfx_dxgi_reset_window_title(void) {
}

static bool gfx_dxgi_has_focus(void) {
    return true;
}

extern "C" HWND gfx_dxgi_get_h_wnd(void) {
    return nullptr;
}

void gfx_dxgi_shutdown(void) {
    s_dxgi.swap_chain.Reset();
    s_dxgi.factory.Reset();
    if (s_dxgi.waitable_object) {
        CloseHandle(s_dxgi.waitable_object);
        s_dxgi.waitable_object = nullptr;
    }
}

void gfx_dxgi_start_text_input(void) {
    SDL_StartTextInput();
    uwp_show_input_pane();
}

void gfx_dxgi_stop_text_input(void) {
    SDL_StopTextInput();
    uwp_hide_input_pane();
}

static char *gfx_dxgi_get_clipboard_text(void) {
    return nullptr;
}

static void gfx_dxgi_set_clipboard_text(UNUSED const char *text) {
}

static void gfx_dxgi_set_cursor_visible(UNUSED bool visible) {
}

void ThrowIfFailed(HRESULT res) {
    if (FAILED(res)) {
        throw res;
    }
}

void ThrowIfFailed(HRESULT res, UNUSED HWND h_wnd, const char *message) {
    if (FAILED(res)) {
        if (message) {
            fprintf(stderr, "DXGI/D3D error: 0x%08X: %s\n", (unsigned int)res, message);
        }
        throw res;
    }
}

extern "C" GfxWindowManagerAPI gfx_dxgi = {
    gfx_dxgi_init,
    gfx_dxgi_set_keyboard_callbacks,
    gfx_dxgi_set_scroll_callback,
    gfx_dxgi_main_loop,
    gfx_dxgi_get_dimensions,
    gfx_dxgi_handle_events,
    gfx_dxgi_start_frame,
    gfx_dxgi_swap_buffers_begin,
    gfx_dxgi_swap_buffers_end,
    gfx_dxgi_get_time,
    gfx_dxgi_shutdown,
    gfx_dxgi_start_text_input,
    gfx_dxgi_stop_text_input,
    gfx_dxgi_get_clipboard_text,
    gfx_dxgi_set_clipboard_text,
    gfx_dxgi_set_cursor_visible,
    gfx_dxgi_delay,
    gfx_dxgi_get_max_msaa,
    gfx_dxgi_set_window_title,
    gfx_dxgi_reset_window_title,
    gfx_dxgi_has_focus,
};
