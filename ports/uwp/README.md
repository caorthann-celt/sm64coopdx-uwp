# sm64coopdx UWP build notes

This folder is the Xbox UWP side of the port.

The shape is deliberately simpler now:

- Game code builds as `sm64coopdx_game.dll`
- The UWP package starts from `ports/uwp/uwp/main.cpp`
- DirectX is the default renderer on UWP
- The upstream SDL2/OpenGL path is still available through Mesa UWP
- SDL2 and the Mesa OpenGL runtime come from `ports/uwp/third_party/uwp-deps`
- The wrapper references normal `Microsoft.VCLibs`; the clang-built game DLL is linked against the UWP app CRT
- Saves, config, mods, DynOS packs, and the ROM live in the active storage root

## Build Requirements

You need a few bits installed before this port will build. It is a two-lane setup:

- Ninja builds the `clang-cl` game DLL
- Visual Studio/MSBuild builds the UWP wrapper and MSIX package
- MSYS2 runs the upstream Makefile far enough to generate the normal game assets
- `third_party/uwp-deps` provides SDL2, Mesa, libuwp, and the runtime DLLs

Install these:

- Visual Studio 2022 with C++ UWP support
- Windows 10 SDK `10.0.19041.0` or newer
- LLVM/Clang tools for Visual Studio, so `clang-cl` is available from `VsDevCmd`
- CMake
- Ninja
- Python 3
- MSYS2

If you are starting from a clean Windows install, this is the reliable route:

```powershell
winget install --id Microsoft.VisualStudio.2022.Community -e
winget install --id Kitware.CMake -e
winget install --id Ninja-build.Ninja -e
winget install --id Python.Python.3.12 -e
winget install --id MSYS2.MSYS2 -e
winget install --id Git.Git -e
```

Then open the Visual Studio Installer and make sure these components are selected:

```text
Universal Windows Platform development
Desktop development with C++
C++ Clang tools for Windows
C++ CMake tools for Windows
Windows 10 SDK 10.0.19041.0 or newer
```

If you prefer the command line, Visual Studio components can be added with `vs_installer.exe modify`. Adjust `<VS install path>` for Community, Professional, or Build Tools:

```powershell
& "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vs_installer.exe" modify `
  --installPath "<VS install path>" `
  --add Microsoft.VisualStudio.Workload.Universal `
  --add Microsoft.VisualStudio.Workload.NativeDesktop `
  --add Microsoft.VisualStudio.Component.VC.Llvm.Clang `
  --add Microsoft.VisualStudio.Component.VC.CMake.Project `
  --add Microsoft.VisualStudio.Component.Windows10SDK.19041 `
  --includeRecommended `
  --passive `
  --norestart
```

MSYS2 needs the normal Makefile toolchain pieces. After installing MSYS2, run:

```powershell
$env:MSYS2_ROOT = "<path-to-msys2>"
& "$env:MSYS2_ROOT\usr\bin\bash.exe" -lc "pacman -Syu --needed --noconfirm"
& "$env:MSYS2_ROOT\usr\bin\bash.exe" -lc "pacman -S --needed --noconfirm base-devel git make python mingw-w64-x86_64-gcc"
```

If the first MSYS2 update asks you to close the terminal, close it and run the second command afterwards.

The CMake prebuild step expects a `C:\msys64` MSYS2 install by default. If yours lives somewhere else, pass it during configure:

```powershell
cmake --preset uwp-game-dll-release -DSM64COOPDX_MSYS2_ROOT="$env:MSYS2_ROOT"
```

## Storage

On Xbox Dev Mode the port checks external storage first:

```text
E:\sm64coopdx
```

If that folder exists, contains a `.z64` ROM, and is writable, it becomes the active root. If not, the game falls back to plain app `LocalState`.

The port creates these folders automatically:

```text
mods
dynos
dynos\packs
sav
```

Temporary mod/cache work stays in LocalState even when the active root is on `E:`. That keeps USB/external-drive writes from making downloads and cache work feel crunchy.

## Runtime Dependencies

The UWP runtime bundle lives in:

```text
ports\uwp\third_party\uwp-deps
```

That folder is vendored with the port, so the build does not need a separate local checkout for the runtime DLLs.

The package stages `SDL2.dll`, `opengl32.dll`, `libgallium_wgl.dll`, `libuwp.dll`, `d3dcompiler_47.dll`, `dxil.dll`, and `z-1.dll` beside the executable.

## Assets

The UWP CMake build runs the upstream asset prebuild step through:

```text
ports\uwp\cmake\PrebuildAssets.cmake
```

That keeps asset generation close to upstream instead of maintaining a second asset pipeline.

## Build

Open a Visual Studio UWP developer prompt first, or run `VsDevCmd.bat` yourself with:

```text
-arch=x64 -host_arch=x64 -app_platform=UWP
```

If the presets have already been configured once, you can build from this folder:

```powershell
cd <repo>\ports\uwp
cmake --build --preset uwp-game-dll-release
cmake --build --preset uwp-package-release
```

For a completely fresh tree, configure/build the DLL before configuring the package:

```powershell
cd <repo>\ports\uwp
cmake --preset uwp-game-dll-release
cmake --build --preset uwp-game-dll-release
cmake --preset uwp-package-release
cmake --build --preset uwp-package-release
```

There are two stages:

- `uwp-game-dll-release` builds the game DLL with `clang-cl`
- `uwp-package-release` builds the UWP wrapper and MSIX package

The package lands under:

```text
ports\uwp\build-package\AppPackages\sm64coopdx-uwp
```

## Third Party Notes

This port vendors a modified UWP-compatible copy of CoopNet under:

```text
ports\uwp\third_party\coopnet
```

See `ports/uwp/third_party/README.md` and the CoopNet license there for the modification notice.
