# 34Cheat

Mã nguồn C++ cho Windows x64: DLL internal và loader DX11/ImGui.

## Build

Cần Visual Studio 2022 / Build Tools với **Desktop development with C++**, MSVC x64, Windows SDK và Python 3 có trong `PATH`. ImGui và MinHook được kèm source. Không cần DLL game hoặc metadata để biên dịch.

Chạy trong thư mục này:

```bat
build.bat
```

Build mặc định tạo `34Cheat.dll`, sinh `EmbeddedDll.h`, rồi đóng gói thành `34Cheat.exe`. EXE chứa DLL; header nhúng và binary là file sinh tự động. Script cũng chép EXE/DLL lên thư mục cha và EXE vào `../release/`.

## Kiểm thử

```bat
test_internal.bat
test_loader.bat
```

Test tạo artifact trong `tests/out/`. Loader tests dùng tiến trình và DLL fixture riêng. Fixture không thay thế kiểm thử toàn bộ tính năng trong game thật.

## Sử dụng

Mở `34Cheat.exe`, chọn đường dẫn game nếu cần, rồi chọn **Load** hoặc **Launch & load**. Bấm **Insert** để mở/đóng menu. Khi thay DLL, cần thoát game và mở lại.

- **Visuals:** ESP, nhãn tên/role và thông báo góc màn hình.
- **Movement:** Speed, No clip, No cooldown và nút Auto task cho nhiệm vụ được giao. Hoàn thành nhiệm vụ vẫn phụ thuộc phản hồi của game.
- **Camera:** Drone view và giữ mức zoom khi camera bị tạo lại hoặc chuyển trạng thái.
- **Players:** tìm người chơi, role, phe và trạng thái.
- **Events:** kill feed độc lập với menu, nguyên nhân chết khi có dữ liệu, cùng lịch sử sự kiện.
- **Config:** tự tải/lưu và nút Save/Load; dữ liệu cục bộ nằm ở `%LOCALAPPDATA%\34Cheat`.

Role `Unknown` nghĩa là client chưa nhận được role chính xác. Vào phòng từ đầu trận không bảo đảm nhận được role của người khác. Source không cung cấp danh sách role ẩn đầy đủ.

Game có thể thay đổi layout và địa chỉ giữa các bản cập nhật. `SupportedGame.h` chứa dấu nhận dạng build được hỗ trợ; loader kiểm tra trước khi nạp. Biên dịch thành công không xác nhận tương thích game mới.

## Cấu trúc

| File/thư mục | Nội dung |
| --- | --- |
| `Internal.cpp`, `Internal.h`, `InternalRoles.h` | Runtime internal, menu, sự kiện và phân loại role |
| `Injector.cpp`, `LoaderCore.h`, `DesktopSurface.h` | Loader và xử lý nạp DLL |
| `SupportedGame.h` | Dấu nhận dạng game được hỗ trợ |
| `Offsets.h` | Layout và kiểu dữ liệu dùng chung cho internal |
| `embed.py`, `Loader.rc`, `Loader.manifest` | Đóng gói loader |
| `imgui/`, `minhook/`, `include/` | Thư viện và header tương thích |
| `tests/` | Fixture và kiểm thử hồi quy |

## Giấy phép

Mã nguồn riêng của dự án dùng [34Cheat Attribution License 1.0](../LICENSE), là license tùy chỉnh. Được phép sửa, phát triển và bán lại theo điều kiện trong license. Khi bán phần mềm hoặc sản phẩm chứa mã nguồn này, kể cả bản đã sửa đổi, mỗi trang bán hàng/bài đăng bán phải ghi rõ **CiyE (0xciye)** và link repo trong phần mô tả chính, đọc được trước khi mua:

> Based on 34Cheat by CiyE (0xciye). Source: https://github.com/0xciye/34Cheat

Chỉ ghi trong ứng dụng hoặc file license là chưa đủ cho sản phẩm bán. Phải giữ thông báo bản quyền và license khi phân phối. Đây không phải BSD-3-Clause chuẩn hay license được OSI phê duyệt. Thư viện bên thứ ba giữ giấy phép riêng; xem [THIRD_PARTY.md](THIRD_PARTY.md).
