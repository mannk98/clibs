# clibs

Bộ thư viện C để phát triển ứng dụng **IoT** trên các dòng vi điều khiển như
**ATmega** (AVR) và **ESP** (ESP32/ESP8266, ESP-IDF).

Repo này là một *workspace*: code do clibs sở hữu nằm trực tiếp trong `common/`,
`esp-libs/` và `esp-gate/`; còn code đặc thù từng nền tảng (avr, ESP-IDF template)
được tách ra thành các **git submodule** riêng để dùng lại độc lập. Đây là
**thư viện** (libraries only) — ứng dụng node thật build ở repo riêng.

## Cấu trúc

| Thư mục              | Loại            | Mô tả                                                        |
| -------------------- | --------------- | ------------------------------------------------------------ |
| `common/`            | code của clibs  | Thư viện C thuần, không phụ thuộc nền tảng — `dlinklist`, `log`, `ringbuffchar`. |
| `esp-libs/`          | code của clibs  | Building blocks cho node smart-home **ESP8266** (ESP8266 RTOS SDK, FreeRTOS): logic thuần L1 (host-test Unity), wrapper SDK L2 (`wifi_sta`, `mqtt_node`, `nvs_kv`…), driver thiết bị L3 (`relay`, `dht`, `ds18b20`…). Cũng là một ESP8266 SDK component (`component.mk`). Xem `esp-libs/README.md`. |
| `esp-gate/`          | code của clibs  | Dự án "compile-gate": link toàn bộ `esp-libs` với SDK thật để chứng minh build + link (chưa chạy trên phần cứng). |
| `avr-libs/`          | submodule       | Thư viện cho ATmega328p viết bằng C thuần ([mannk98/avr-libs](git@mannk98:mannk98/avr-libs.git)). |
| `esp-idf-template/`  | submodule       | Template ứng dụng ESP-IDF cho dòng ESP ([mannk98/esp-idf-template](git@mannk98:mannk98/esp-idf-template.git)). |

Workspace này **không có build chung ở cấp trên cùng** — mỗi phần dùng toolchain
riêng (host GCC cho `common`/`esp-libs` L1, xtensa SDK cho `esp-gate`, avr-gcc cho
`avr-libs`, ESP-IDF cho `esp-idf-template`).

## Build & test (host)

Phần C thuần (`common/` + L1 của `esp-libs/`) test bằng host GCC, không cần phần
cứng — cũng chính là thứ CI chạy (xem `.github/workflows/ci.yml`):

```sh
make -C common   test && make -C common   strict
make -C esp-libs test && make -C esp-libs strict   # 18 host suites (Unity)
```

Wrapper SDK (L2/L3 của `esp-libs`) không host-test được; chúng được kiểm bằng
**compile-gate** `esp-gate/` (cần SDK ESP8266 — xem `esp-libs/README.md`).

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

Code của clibs (`common/`, `esp-libs/`, `esp-gate/`) phát hành theo giấy phép MIT
(xem `common/LICENSE`). Mỗi submodule giữ license riêng của nó.
