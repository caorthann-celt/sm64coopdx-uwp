This folder is the UWP build's little supply shed.

The main game code still lives in the repo root. This directory just holds the extra source trees, headers, libraries, and runtime files that make the UWP/Xbox build possible without asking people to chase down a bunch of external checkouts first.

Current layout:

- `coopnet/`
- `uwp-deps/`
- `glad/`
- `vcpkg-deps/`
- optional `sm64coopdx/`

The goal here was convenience and preservation. If somebody opens this port up months later, hopefully they should be able to see what it depends on and build it without doing archaeology.

## Credits

A big shout goes to Daniel Worley for the UWP/OpenGL/Xbox groundwork and documentation that helped make this port much less painful to figure out:

- [worleydl/uwp-dep](https://github.com/worleydl/uwp-dep)
- [worleydl/libuwp](https://github.com/worleydl/libuwp)
- [worleydl/mesa-uwp](https://github.com/worleydl/mesa-uwp)
- [worleydl/SDL-uwp-gl](https://github.com/worleydl/SDL-uwp-gl)
- [worleydl/uwp_gl_sample](https://github.com/worleydl/uwp_gl_sample)
- [WorleyDL's UWP porting notes](https://wiki.sternserv.xyz/docs/helpful-links/worleydls-uwp-porting-notes)

And of course, credit is also due to the original upstream projects that are actually doing the heavy lifting:

- [sm64coopdx](https://github.com/coop-deluxe/sm64coopdx)
- SDL2
- Mesa
- GLAD
- libjuice
- zlib
- Lua
