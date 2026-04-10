# sm64coopdx UWP Port

This folder is the Xbox UWP Dev Mode side of the project.

The whole idea here was pretty simple from the start: keep as much of the normal game as possible, keep the uwp specific glue boxed up in one place.

## What This Folder Is

This is the UWP host and build setup for running `sm64coopdx` in Xbox Dev Mode.

It keeps the main game in the normal repo, then adds the extra bits the UWP build needs:

- UWP entry code
- checked in runtime files and libraries
- vendored CoopNet source used by the port
- packaging files for the MSIX build

## Current Shape Of The Port

Right now this build:

- builds a UWP x64 package from the main repo
- uses the checked in SDL2, Mesa, GLAD, Lua, zlib, libjuice, and CoopNet pieces in this directory
- supports a flat `LocalState` path for app-local storage
- supports `E:\\sm64coopdx\\baserom.us.z64` as the external storage ROM path on Xbox
- keeps `.tmp` local so remote mod downloads do not crawl when the main writable root is on `E:`
- lets packaged default content act as a read fallback while the active writable root fills in over time

This is still very much a Dev Mode project. It is meant to hopefully (fingers crossed) be practical, understandable, and easy to keep updating.

## Folder Layout

Everything the UWP build expects lives here:

- repo root game source
- `third_party/coopnet`
- `third_party/uwp-deps`
- `third_party/glad`
- `third_party/vcpkg-deps`
- optional `third_party/sm64coopdx`

By default, `CMakeLists.txt` treats the repo root as `SM64COOPDX_ROOT`, so the normal game source stays in one place and the UWP extras stay here.

## Tooling Setup

There are really two environments involved here:

- PowerShell / Visual Studio for the UWP configure and build steps
- MSYS2 MINGW64 for the asset-generation Makefile that fills in `build/us_pc`

### MSYS2 / MINGW64

I expect MSYS2 to be installed at:

```text
C:\msys64
```

Download the newest MSYS2 installer, install it there, then open `mingw64.exe`.

Run this first:

```sh
pacman -Syuu
```

If it tells you to close the shell, do that, reopen `mingw64.exe`, and run the same command again. That gets the package set fully up to date.

Then install the packages this port currently needs:

```sh
pacman -S unzip make git mingw-w64-x86_64-gcc mingw-w64-x86_64-glew mingw-w64-x86_64-SDL2 mingw-w64-x86_64-SDL python3
```

That gives me the pieces `tools/Makefile.assets` depends on, especially:

- `bash`
- `make`
- `cpp` / MinGW GCC
- `python3`

### PowerShell / Windows Side

On the PowerShell side, I do not need another compiler toolchain for the asset step. I just need the normal Windows/UWP build tools:

- Visual Studio 2022 with the UWP / Windows application workload
- CMake
- the Windows SDK

PowerShell itself does not need extra packages for `Makefile.assets`. The only important bit is that CMake can find:

```text
C:\msys64\usr\bin\bash.exe
```

That is what the UWP `CMakeLists.txt` now uses to run `tools/Makefile.assets` during configure.

## Build Notes

From `ports/uwp`, configure with:

```powershell
cmake -S . `
  -B build-real `
  -G "Visual Studio 17 2022" -A x64 `
  -DSM64COOPDX_USE_PLACEHOLDER_APP=OFF
```

Then build with:

```powershell
cmake --build build-real --config Release --target sm64coopdx-uwp -- /m
```

During configure, the UWP CMake now runs `tools/Makefile.assets` first so the generated `build/us_pc` files already exist before the main project is generated.

Use `sm64coopdx-uwp` as the Visual Studio startup project, not `ALL_BUILD`.

## ROM Paths

For local Visual Studio testing, put `baserom.us.z64` in the app's `LocalState`.

For Xbox Dev Mode external-storage testing, use:

```text
E:\sm64coopdx\baserom.us.z64
```

The ROM should match the vanilla US hash:

```text
20b854b239203baf6c961b850a4a51a2
```

## Storage Behavior

The port now uses a pretty straightforward model:

- `LocalState` is the normal app-local home
- if the ROM is found on `E:\`, then `E:\sm64coopdx` becomes the active writable root for that session
- packaged default content is still available as a read fallback
- missing default files can be seeded into the active writable root without holding startup hostage

That keeps first launch much less painful than blocking the whole boot sequence on a giant recursive copy.

## Credits

This port stands on a lot of earlier UWP/Xbox homebrew groundwork, and Daniel Worley deserves real credit here. His projects and notes were genuinely helpful reference material while figuring out how to get SDL, OpenGL, Mesa, and UWP to play nicely together:

- [worleydl/uwp-dep](https://github.com/worleydl/uwp-dep)
- [worleydl/libuwp](https://github.com/worleydl/libuwp)
- [worleydl/mesa-uwp](https://github.com/worleydl/mesa-uwp)
- [worleydl/SDL-uwp-gl](https://github.com/worleydl/SDL-uwp-gl)
- [worleydl/uwp_gl_sample](https://github.com/worleydl/uwp_gl_sample)
- [WorleyDL's UWP porting notes](https://wiki.sternserv.xyz/docs/helpful-links/worleydls-uwp-porting-notes)

Other important pieces this port leans on:

- the main [sm64coopdx](https://github.com/coop-deluxe/sm64coopdx) project
- SDL2
- Mesa
- GLAD
- libjuice
- zlib
- Lua

## A Few Practical Notes

- Generated `build/` and `build-real/` folders under `ports/uwp` are machine specific and should stay untracked.
- Generated `build/us_pc` content comes from `tools/Makefile.assets`, and the UWP CMake now calls that for me during configure.
- The checked-in `third_party` content is what this build uses. You do not need a separate `uwp-dep`, `uwp_gl_sample`, or `vcpkg` checkout to build this version.
- This is a Dev Mode hobby port, not an upstream release branch.
