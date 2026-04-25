#include <Windows.h>

#include "SDL.h"
#include "SDL_main.h"

int CALLBACK WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    return SDL_WinRTRunApp(SDL_main, nullptr);
}
