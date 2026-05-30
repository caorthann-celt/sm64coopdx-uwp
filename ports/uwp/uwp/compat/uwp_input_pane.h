#pragma once

// DirectX owns the CoreWindow, so bridge the bits SDL doesn't get to own itself
#include "SDL.h"
#include "pc/gfx/gfx_window_manager_api.h"

#ifdef __cplusplus
extern "C" {
#endif

bool uwp_show_input_pane(void);
bool uwp_hide_input_pane(void);
void uwp_input_set_keyboard_callbacks(kb_callback_t on_key_down, kb_callback_t on_key_up, void (*on_all_keys_up)(void),
                                      void (*on_text_input)(char*), void (*on_text_editing)(char*, int));
void uwp_input_set_scroll_callback(void (*on_scroll)(float, float));
void uwp_input_pump_sdl_events(void (*on_quit)(void));

#ifdef __cplusplus
}
#endif
