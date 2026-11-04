# Third-party code

- **Dear ImGui** (`imgui/`): MIT, copyright Omar Cornut and contributors. Full notice: [imgui/LICENSE.txt](imgui/LICENSE.txt). Upstream: https://github.com/ocornut/imgui. Vendored headers identify version 1.93.1 WIP; no exact upstream commit was recorded.
- **MinHook** (`minhook/` and compatibility copies of `MinHook.h`): BSD-style license, copyright Tsuda Kageyu. Includes Hacker Disassembler Engine by Vyacheslav Patkov. Full notices: [minhook/LICENSE.txt](minhook/LICENSE.txt). Upstream: https://github.com/TsudaKageyu/minhook. The vendored decoder contains local modifications; it is not an unmodified upstream checkout.
- **Embedded stb code and fonts**: licensing notices remain inside `imgui/imstb_*.h` and `imgui/imgui_draw.cpp`; retain these notices when redistributing.

These licenses apply to their respective components, not automatically to the project's own code. No game binaries, extracted metadata, IDA databases or third-party game assets are included in the public source package.
