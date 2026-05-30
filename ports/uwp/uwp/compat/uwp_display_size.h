#pragma once

// Ask Xbox what the HDMI render size really is, instead of trusting CoreWindow
#ifdef __cplusplus
extern "C" {
#endif

int sm64coopdx_uwp_get_render_size(int* width, int* height);

#ifdef __cplusplus
}
#endif
