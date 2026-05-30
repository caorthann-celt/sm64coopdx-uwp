#include <Windows.h>

#include "SDL.h"
#include "SDL_main.h"

int CALLBACK WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    SDL_SetHint(SDL_HINT_WINRT_HANDLE_BACK_BUTTON, "1");
    return SDL_WinRTRunApp(SDL_main, nullptr);
}
