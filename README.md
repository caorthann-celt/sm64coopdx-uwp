![sm64coopdx Logo](textures/segment2/custom_coopdx_logo.rgba32.png)

sm64coopdx is an online multiplayer project for the Super Mario 64 PC port that synchronizes all entities and every level for multiple players. The project was started by the Coop Deluxe Team. The purpose is to actively maintain and improve, but also continue sm64ex-coop, created by djoslin0. More features, customization, and power to the Lua API allow modders and players to enjoy Super Mario 64 more than ever!

Feel free to report bugs or contribute to the project. 

## Initial Goal (Accomplished)
Create a mod for the PC port where multiple people can play together online.

Unlike previous multiplayer projects, this one synchronizes enemies and events. This allows players to interact with the same world at the same time.

Interestingly enough though, the goal of the project has slowly evolved over time from simply just making a Super Mario 64 multiplayer mod to constantly maintaining and improving the project (notably the Lua API.)

## Lua
sm64coopdx is moddable via Lua, similar to Roblox and Garry's Mod's Lua APIs. To get started, click [here](docs/lua/lua.md) to see the Lua documentation.

## Wiki
The wiki is made using GitHub's wiki feature, you can go to the wiki tab or click [here](https://github.com/coop-deluxe/sm64coopdx/wiki).

## UWP Port
The Xbox UWP port now lives in [`ports/uwp`](ports/uwp). That directory holds the UWP host, the checked-in runtime pieces the port depends on, the vendored CoopNet source used by the UWP build, and the packaging/build files for Xbox Dev Mode.

It is very much a hobby porting effort: the goal was to keep the normal game code as intact as possible, keep the UWP-specific glue in one place, and make the result understandable enough that somebody can pick it up later without needing to reverse-engineer the whole setup first.

A lot of the UWP groundwork used to get this moving was inspired by Daniel Worley's UWP/OpenGL/Xbox work, especially:

- [uwp-dep](https://github.com/worleydl/uwp-dep)
- [libuwp](https://github.com/worleydl/libuwp)
- [mesa-uwp](https://github.com/worleydl/mesa-uwp)
- [SDL-uwp-gl](https://github.com/worleydl/SDL-uwp-gl)
- [uwp_gl_sample](https://github.com/worleydl/uwp_gl_sample)
- [WorleyDL's UWP porting notes](https://wiki.sternserv.xyz/docs/helpful-links/worleydls-uwp-porting-notes)

If you are looking for the UWP-specific docs, build notes, and bundled dependency layout, start in [`ports/uwp/README.md`](ports/uwp/README.md).
