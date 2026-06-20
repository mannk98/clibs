# clibs

Bộ thư viện C để phát triển ứng dụng **IoT** trên các dòng vi điều khiển như
**ATmega** (AVR) và **ESP** (ESP32/ESP8266, ESP-IDF).

Repo này là một *workspace*: phần code portable do clibs sở hữu nằm trực tiếp
trong `common/`, còn code đặc thù từng nền tảng được tách ra thành các
**git submodule** riêng để dùng lại độc lập.

## Cấu trúc

| Thư mục              | Loại            | Mô tả                                                        |
| -------------------- | --------------- | ------------------------------------------------------------ |
| `common/`            | code của clibs  | Thư viện C thuần, không phụ thuộc nền tảng — `dlinklist`, `log`, `ringbuffchar`. |
| `avr-libs/`          | submodule       | Thư viện cho ATmega328p viết bằng C thuần ([mannk98/avr-libs](git@mannk98:mannk98/avr-libs.git)). |
| `esp-idf-template/`  | submodule       | Template ứng dụng ESP-IDF cho dòng ESP ([mannk98/esp-idf-template](git@mannk98:mannk98/esp-idf-template.git)). |

## Clone

Vì có submodule, clone kèm `--recurse-submodules`:

```sh
git clone --recurse-submodules git@mannk98:mannk98/clibs.git
```

Nếu đã lỡ clone thường, kéo submodule về sau:

```sh
git submodule update --init --recursive
```

## Làm việc với submodule

```sh
# Cập nhật submodule lên commit mới nhất trên remote của nó
git submodule update --remote <ten-submodule>

# Sửa code bên trong một submodule: commit & push TRONG submodule trước,
# sau đó commit con trỏ mới ở repo cha (clibs).
cd avr-libs && git add -A && git commit && git push
cd .. && git add avr-libs && git commit -m "chore: bump avr-libs"
```

Submodule được **pin theo commit** (không bám branch) để clone lại luôn ra đúng
trạng thái đã ghi.

## License

`common/` phát hành theo giấy phép MIT (xem `common/LICENSE`). Mỗi submodule giữ
license riêng của nó.
