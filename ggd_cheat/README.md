# Hướng dẫn phát triển

Source internal và loader của **34Cheat**. Xem [README chính](../README.md) để biết tính năng, cách sử dụng và giới hạn hiện tại.

## Chuẩn bị môi trường

- Windows 10/11 x64.
- Visual Studio 2022 hoặc Build Tools, có **Desktop development with C++**, MSVC x64 và Windows SDK.
- Python 3 có trong `PATH` dưới tên `python`.

`setup_msvc.bat` dùng `vswhere.exe` để tìm toolchain rồi gọi `vcvars64.bat`. Không cần sửa đường dẫn Visual Studio trong source. ImGui và MinHook đã kèm source; không cần DLL game hoặc metadata để build.

## Build internal và loader

Chạy từ thư mục `ggd_cheat`:

```bat
.\build.bat
```

Script thực hiện ba bước:

1. Biên dịch `Internal.cpp` cùng ImGui/MinHook thành `34Cheat.dll`.
2. Dùng `embed.py` sinh `EmbeddedDll.h`, gồm payload và SHA-256.
3. Biên dịch loader thành `34Cheat.exe` với DLL nhúng bên trong.

Có thể dùng `build.bat internal` tương đương. Project chỉ còn internal và loader.

| Artifact | Vị trí / mục đích |
| --- | --- |
| `34Cheat.dll` | Runtime internal, tạo trong thư mục này và chép lên thư mục cha |
| `34Cheat.exe` | Loader độc lập, tạo tại đây, chép lên thư mục cha và `../release/` |
| `34Cheat_Injector.exe` | Bản sao loader giữ tên cũ |
| `EmbeddedDll.h` | File sinh tự động, không chỉnh tay hoặc commit |
| `*.pdb` | Symbol phục vụ debug, không cần để chạy loader |

EXE dùng static CRT và chứa DLL. Máy chạy bản build không cần Python hay Visual Studio. Vẫn cần game đúng build được loader hỗ trợ và môi trường đồ họa phù hợp.

## Kiểm thử hồi quy

```bat
.\test_internal.bat
.\test_loader.bat
```

| Bộ test | Nội dung |
| --- | --- |
| `test_internal.bat` | State runtime, camera/chuyển cảnh, movement, role, sự kiện và config |
| `test_loader.bat` | Payload, đường dẫn, readiness handshake và ngăn nạp trùng bằng tiến trình fixture |

Artifact của test nằm trong `tests/out/`. Mỗi script trả exit code khác 0 khi lỗi. Fixture không xác nhận toàn bộ hành vi trong trận thật.

## Chẩn đoán lỗi

| Triệu chứng | Kiểm tra |
| --- | --- |
| Không tìm thấy MSVC | Cài workload C++ và Windows SDK; kiểm tra có `vswhere.exe` trong Visual Studio Installer |
| Không gọi được Python | Kiểm tra `python --version` trong cùng terminal |
| Loader từ chối game build | Đối chiếu game hiện tại với dấu nhận dạng trong `SupportedGame.h`; cần kiểm tra lại layout/địa chỉ khi game cập nhật |
| Vẫn chạy DLL cũ | Thoát phiên game đang chạy rồi nạp bằng EXE mới |
| Role hiển thị `Unknown` | Client chưa có role xác nhận; vào trận từ đầu không bảo đảm nhận được role của người khác |
| Auto task chưa hoàn thành | Kiểm tra trạng thái trận, nhiệm vụ được giao và phản hồi từ game |

Dùng **Copy error** trong loader khi báo lỗi. Log ở `%LOCALAPPDATA%\34Cheat\loader.log`; config ở `%LOCALAPPDATA%\34Cheat\config.ini`. Loại thông tin cá nhân trước khi chia sẻ log.

## Phân chia mã nguồn

| Thành phần | File |
| --- | --- |
| Runtime, menu và tính năng | `Internal.cpp`, `Internal.h` |
| Role và layout dùng chung | `InternalRoles.h`, `Offsets.h` |
| Loader và nạp DLL | `Injector.cpp`, `LoaderCore.h` |
| Giao diện desktop | `DesktopSurface.h`, `UiTheme.h` |
| Nhận dạng game build | `SupportedGame.h` |
| Đóng gói | `embed.py`, `Loader.rc`, `Loader.manifest` |
| Dependency | `imgui/`, `minhook/`, `include/` |
| Kiểm thử | `tests/` |

Giữ binary, log, config cục bộ và header nhúng ngoài commit theo `.gitignore`. Khi sửa logic runtime, chạy bộ test liên quan và kiểm tra lại trong game nếu thay đổi phụ thuộc trạng thái thực tế.

## License và credit

Mã nguồn riêng dùng [34Cheat Attribution License 1.0](../LICENSE), là license tùy chỉnh. Khi bán phần mềm, bản sửa đổi hoặc sản phẩm chứa mã nguồn này, mỗi trang bán hàng/bài đăng bán do bạn đăng hoặc kiểm soát phải ghi credit trong phần mô tả chính, đọc được trước khi mua:

> Based on 34Cheat by CiyE (0xciye).
>
> Source: https://github.com/0xciye/GGD

Chỉ ghi trong ứng dụng hoặc file license là chưa đủ cho sản phẩm bán. Phải giữ thông báo bản quyền và license khi phân phối; xem toàn bộ điều kiện trong [LICENSE](../LICENSE). Thư viện bên thứ ba giữ giấy phép riêng tại [THIRD_PARTY.md](THIRD_PARTY.md).
