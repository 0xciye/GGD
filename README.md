# 34Cheat

**Internal runtime and standalone loader for Goose Goose Duck on Windows x64.**

Built with C++17, DirectX 11, Dear ImGui and MinHook. Maintained by [CiyE (0xciye)](https://github.com/0xciye).

[Features](#features) · [Build](#build-from-source) · [Usage](#usage) · [Development guide](ggd_cheat/README.md) · [License](#license-and-attribution)

> **Compatibility:** the loader targets a specific game build, identified in [SupportedGame.h](ggd_cheat/SupportedGame.h). Game updates may require code and offset changes. A successful build does not establish compatibility with another game version.

## Overview

34Cheat consists of an internal DLL and a desktop loader. The build embeds the DLL into `34Cheat.exe`, so the resulting executable can be distributed without a separate DLL or development tools.

This repository contains project source, vendored dependencies and regression fixtures. Game binaries, extracted metadata, investigation logs and generated build artifacts are excluded.

## Features

| Area | Capabilities |
| --- | --- |
| Visuals | Player ESP, corner boxes, name and known-role labels, configurable colors and appearance |
| Movement | Speed multiplier, No clip, No cooldown and an Auto task command for assigned tasks |
| Camera | Drone view with configurable zoom and restoration after camera or scene transitions |
| Players | Searchable player list with available role, team and state information |
| Events | On-screen kill feed, death reasons when available, meeting and round events, and room event history |
| Configuration | Automatic loading and saving, plus manual Save/Load controls |

The kill feed remains visible when the menu is closed or ESP is disabled.

### Current limits

- **Hidden roles:** `Unknown` means the client has not received a confirmed role. Joining before the round starts does not guarantee other players' roles will be available.
- **Auto task:** completion depends on the game's response. The command does not guarantee that every assigned task will be accepted or completed immediately.
- **Event details:** killer names, death reasons and role revelations depend on the information available to the client.
- **Runtime validation:** build and fixture tests pass; they do not verify every feature or every transition in a live match.

## Build from source

### Requirements

| Requirement | Details |
| --- | --- |
| Platform | Windows 10/11, x64 |
| C++ toolchain | Visual Studio 2022 or Build Tools with **Desktop development with C++**, MSVC x64 and Windows SDK |
| Python | Python 3, available as `python` in `PATH` |
| Git | Required to clone the repository |

Dear ImGui and MinHook source are included. No game DLL or metadata dump is needed to compile.

Run these commands in Command Prompt, or run the batch file from PowerShell:

```bat
git clone https://github.com/0xciye/34Cheat.git
cd 34Cheat\ggd_cheat
.\build.bat
```

The script locates the installed MSVC toolchain, compiles the internal DLL, generates `EmbeddedDll.h` and builds the loader with the embedded payload.

| Output | Purpose |
| --- | --- |
| `ggd_cheat/34Cheat.exe` | Standalone loader with embedded DLL |
| `ggd_cheat/34Cheat.dll` | Internal runtime |
| `release/34Cheat.exe` | Copy of the standalone executable for distribution |

The script also creates the legacy-named `34Cheat_Injector.exe` alias and copies the main EXE/DLL to the repository root. Generated files are ignored by Git. See the [development guide](ggd_cheat/README.md) for build details.

## Usage

1. Open the built `34Cheat.exe`.
2. If the game is not detected, use **Browse...** to select `Goose Goose Duck.exe`.
3. Select **Load** for a running game or **Launch & load** to start it and load the runtime.
4. Press **Insert** in-game to open or close the menu.

After updating the embedded DLL, end the existing game session and load the new build. The loader does not replace a runtime already loaded into that session.

### Settings and diagnostics

Local data is stored under `%LOCALAPPDATA%\34Cheat`.

| File | Purpose |
| --- | --- |
| `config.ini` | Visual, notification, camera and movement settings |
| `loader.log` | Loader diagnostics |

Settings load at startup and save when the menu closes. **Save config** and **Load config** provide manual controls. Event history and the Auto task command are not persisted as configuration.

For a loader failure, use **Copy error** and include the relevant log when reporting the problem. Include the game build, the action that triggered the issue and whether it reproduces with a fresh game session. Remove personal information before sharing logs.

## Testing

From `ggd_cheat/`:

```bat
.\test_internal.bat
.\test_loader.bat
```

Internal fixtures cover runtime state, camera and scene transitions, movement, roles, events and configuration. Loader fixtures cover payload integrity, paths, readiness and duplicate-load handling using a controlled test process. Outputs are written to `ggd_cheat/tests/out/`.

Both suites and a clean internal build passed during source preparation on **4 November 2026**. Live-match behavior still requires testing against the supported game build.

## Project layout

| Path | Responsibility |
| --- | --- |
| [Internal.cpp](ggd_cheat/Internal.cpp) | Runtime hooks, feature state, menu, ESP and events |
| [InternalRoles.h](ggd_cheat/InternalRoles.h), [Offsets.h](ggd_cheat/Offsets.h) | Role classification and shared game layouts |
| [Injector.cpp](ggd_cheat/Injector.cpp), [LoaderCore.h](ggd_cheat/LoaderCore.h) | Loader interface, validation and DLL loading |
| [DesktopSurface.h](ggd_cheat/DesktopSurface.h), [UiTheme.h](ggd_cheat/UiTheme.h) | Desktop rendering and interface styling |
| [SupportedGame.h](ggd_cheat/SupportedGame.h) | Supported game fingerprint |
| [build.bat](ggd_cheat/build.bat), [embed.py](ggd_cheat/embed.py) | Build and payload packaging |
| [tests/](ggd_cheat/tests) | Regression fixtures |
| [imgui/](ggd_cheat/imgui), [minhook/](ggd_cheat/minhook) | Vendored dependencies |

## License and attribution

Project-owned code is licensed under the custom [34Cheat Attribution License 1.0](LICENSE). Use, modification and commercial distribution are permitted subject to its conditions.

**Every sales page or listing you publish or control for the software, a modified version, or a product incorporating it must display this credit in the main product description, readable before purchase:**

> Based on 34Cheat by CiyE (0xciye).
>
> Source: https://github.com/0xciye/34Cheat

Credit only inside the application, a license file or a separate credits page does not satisfy the sales-page requirement. Copyright and license notices must also accompany redistributed source and binaries. The [full license](LICENSE) defines the applicable conditions.

This is a custom license, not standard BSD-3-Clause or an OSI-approved license. Third-party components retain their own licenses; see [third-party notices](ggd_cheat/THIRD_PARTY.md).
