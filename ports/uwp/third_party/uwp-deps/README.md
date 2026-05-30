## UWP SDL/OpenGL deps

Small UWP/Xbox SDL2 and Mesa helper pack for the sm64coopdx UWP package.

It keeps the same basic shape as `uwp-dep`:

```text
x64/bin
x64/lib
x64/include
lic
```

Runtime files staged from here:

```text
SDL2.dll
opengl32.dll
libgallium_wgl.dll
libuwp.dll
dxil.dll
z-1.dll
```

Import libraries staged from here:

```text
SDL2.lib
libuwp.lib
```

SDL2 owns the window/event path. Mesa's `opengl32.dll` supplies the OpenGL renderer backend used by upstream.
