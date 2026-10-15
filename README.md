# Nhiệm vụ: Module Independent Watchdog (IWDG)

## Mục tiêu

Triển khai module IWDG trên Zephyr để tích hợp vào firmware sản phẩm. Watchdog đã được bật trong Kconfig nhưng chưa có phần arm và feed. Cần thực hiện module hoàn chỉnh và chứng minh khả năng hồi phục khi treo trên các board STM32 được hỗ trợ.

## Đầu ra mong muốn

| File | Nội dung |
|-----------|----------|
| `include/wdt_token.h` | Khai báo hàm |
| `src/wdt_token.c` | Phần code triển khai |
| Ghi chú ngắn | Tổng hợp board đã dùng, `timeout_ms`, chu kỳ feed, kết quả kiểm thử |

Gửi `wdt_token.c` / `wdt_token.h` đã hoàn thiện, ghi chú tích hợp, và log console.

## Yêu cầu

1. Implement `wdt_token_init()` và `wdt_token_feed()` bằng Zephyr watchdog API (`zephyr/drivers/watchdog.h`).
2. Timeout **3–5 giây**. Chu kỳ feed **nhỏ hơn timeout / 2**.
3. `wdt_token_force_hang_for_test()` chỉ có khi bật `CONFIG_TOKEN_DEV` hoặc `WDT_TOKEN_TEST`.
4. Kiểm thử trên `nucleo_f401re`, `nucleo_f411re` hoặc `nucleo_g474re`:
   - Vòng lặp feed bình thường: không reset trong vài phút
   - Buộc treo: reset trong cửa sổ timeout, sau đó chạy lại bình thường
5. Ghi rõ điểm gọi khi tích hợp:
   - `wdt_token_init(...)` sau khi init hệ thống, trước vòng lặp chính
   - `wdt_token_feed()` mỗi vòng lặp chính (và tại các chỗ chờ lâu nếu có)

## Cài đặt môi trường (Ubuntu)

```bash
sudo apt update
sudo apt install --no-install-recommends git cmake ninja-build gperf \
  ccache dfu-util device-tree-compiler wget \
  python3-dev python3-pip python3-setuptools python3-tk python3-wheel \
  xz-utils file make gcc gcc-multilib g++-multilib libsdl2-dev libmagic1

mkdir -p ~/zephyrproject
python3 -m venv ~/zephyrproject/.venv
source ~/zephyrproject/.venv/bin/activate
pip install west

west init -m https://github.com/zephyrproject-rtos/zephyr \
  --mr v4.2.0 ~/zephyrproject
cd ~/zephyrproject && west update && west zephyr-export && west packages pip --install
cd ~/zephyrproject/zephyr && west sdk install --toolchains arm-zephyr-eabi
```

Tạo app Zephyr tối giản, thêm module này và `conf/wdt.fragment.conf`, rồi:

```bash
source ~/zephyrproject/.venv/bin/activate
west build -p always -b nucleo_f401re <thu-muc-app>
west flash

west build -p always -b black_f407ve sample_app
west flash --runner dfu-util

```

Khuyến khích nên thực hiện trên OS Ubuntu. Tham khảo thêm ở: [Zephyr Getting Started](https://docs.zephyrproject.org/latest/develop/getting_started/index.html)
