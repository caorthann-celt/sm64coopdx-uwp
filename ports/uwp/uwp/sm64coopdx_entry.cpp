#include "SDL.h"

extern "C" int sm64coopdx_main(int argc, char** argv);

extern "C" int SDL_main(int argc, char** argv)
{
    SDL_SetMainReady();

#if defined(UWP_BUILD)
    static char appName[] = "sm64coopdx-uwp";
    static char* safeArgv[] = { appName, nullptr };
    return sm64coopdx_main(1, safeArgv);
#else
    if (argc <= 0 || argv == nullptr || argv[0] == nullptr) {
        static char appName[] = "sm64coopdx-uwp";
        static char* safeArgv[] = { appName, nullptr };
        return sm64coopdx_main(1, safeArgv);
    }

    return sm64coopdx_main(argc, argv);
#endif
}
