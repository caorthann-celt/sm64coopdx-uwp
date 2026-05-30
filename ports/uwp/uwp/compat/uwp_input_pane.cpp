#include "uwp_input_pane.h"

#include <winrt/Windows.UI.ViewManagement.h>

extern "C" {
#include "pc/controller/controller_bind_mapping.h"
}

static kb_callback_t s_key_down = nullptr;
static kb_callback_t s_key_up = nullptr;
static void (*s_all_keys_up)(void) = nullptr;
static void (*s_text_input)(char*) = nullptr;
static void (*s_text_editing)(char*, int) = nullptr;
static void (*s_scroll)(float, float) = nullptr;

// Best effort only: Xbox may say no
bool uwp_show_input_pane(void) {
    try {
        return winrt::Windows::UI::ViewManagement::InputPane::GetForCurrentView().TryShow();
    } catch (...) {
        return false;
    }
}

void uwp_input_set_keyboard_callbacks(kb_callback_t on_key_down, kb_callback_t on_key_up, void (*on_all_keys_up)(void),
                                      void (*on_text_input)(char*), void (*on_text_editing)(char*, int)) {
    s_key_down = on_key_down;
    s_key_up = on_key_up;
    s_all_keys_up = on_all_keys_up;
    s_text_input = on_text_input;
    s_text_editing = on_text_editing;
}

void uwp_input_set_scroll_callback(void (*on_scroll)(float, float)) {
    s_scroll = on_scroll;
}

void uwp_input_pump_sdl_events(void (*on_quit)(void)) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_TEXTINPUT:
                if (s_text_input) { s_text_input(event.text.text); }
                break;
            case SDL_TEXTEDITING:
                if (s_text_editing) { s_text_editing(event.edit.text, event.edit.start); }
                break;
            case SDL_KEYDOWN:
                if (s_key_down) { s_key_down(translate_sdl_scancode(event.key.keysym.scancode)); }
                break;
            case SDL_KEYUP:
                if (s_key_up) { s_key_up(translate_sdl_scancode(event.key.keysym.scancode)); }
                break;
            case SDL_MOUSEWHEEL:
                if (s_scroll) { s_scroll(event.wheel.preciseX, event.wheel.preciseY); }
                break;
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_FOCUS_LOST && s_all_keys_up) {
                    s_all_keys_up();
                }
                break;
            case SDL_QUIT:
                if (on_quit) { on_quit(); }
                break;
        }
    }
}

bool uwp_hide_input_pane(void) {
    try {
        return winrt::Windows::UI::ViewManagement::InputPane::GetForCurrentView().TryHide();
    } catch (...) {
        return false;
    }
}
