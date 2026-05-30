#pragma once

// DirectX owns the CoreWindow, so ask UWP directly for the soft keyboard
#ifdef __cplusplus
extern "C" {
#endif

bool uwp_show_input_pane(void);
bool uwp_hide_input_pane(void);

#ifdef __cplusplus
}
#endif
