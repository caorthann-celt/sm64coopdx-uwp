This folder is the UWP build's supply shed.

The main game code still lives in the repo root. This directory just holds the extra source trees, headers, libraries, and runtime files that make the UWP/Xbox build possible without asking people to chase down a bunch of external checkouts first.

Current layout:

- `coopnet/`
- `glew/`
- `lua-5.3.5/`
- `uwp-deps/` for SDL2, Mesa OpenGL, libuwp, and runtime DLLs
- `vcpkg-deps/` for zlib and libjuice
- optional `sm64coopdx/`

The port uses one SDL/OpenGL dependency bundle for the Mesa path: `uwp-deps` is based on [worleydl/uwp-dep](https://github.com/worleydl/uwp-dep). The package stages the Mesa runtime DLLs from that bundle beside the UWP executable, while the game can still use its DirectX renderer path.

## CoopNet Notice

This UWP port includes a modified copy of CoopNet in `third_party/coopnet`.

The changes are UWP/Xbox compatibility changes made so CoopNet can build and run inside the UWP package environment. CoopNet remains under its original license; see `third_party/coopnet/LICENSE`.

## Credits

A big shout goes to Daniel Worley for the UWP/Xbox groundwork and documentation that helped make this port much less painful to figure out:

- [worleydl/uwp-dep](https://github.com/worleydl/uwp-dep)
- [worleydl/libuwp](https://github.com/worleydl/libuwp)
- [worleydl/uwp_gl_sample](https://github.com/worleydl/uwp_gl_sample)
- [WorleyDL's UWP porting notes](https://wiki.sternserv.xyz/docs/helpful-links/worleydls-uwp-porting-notes)

Credit also goes to SternXD for the Xbox HDMI display size work this port's `uwp_display_size` helper is based on. That helper asks UWP for the HDMI output mode, clamps the render size by Xbox model, and keeps both the SDL/Mesa path and the DirectX swapchain path from blindly trusting the 1080p CoreWindow size.

The SDL/OpenGL route uses the upstream SDL2/OpenGL path with Mesa's UWP `opengl32.dll`. GLEW is built statically because Windows OpenGL exposes shader and buffer entrypoints through runtime lookup rather than the import library.

And of course, credit is also due to the original upstream projects that are actually doing the heavy lifting:

- [sm64coopdx](https://github.com/coop-deluxe/sm64coopdx)
- SDL2
- Mesa
- GLEW
- libjuice
- zlib
- Lua
