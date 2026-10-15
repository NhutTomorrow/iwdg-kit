#include "wdt_token.h"

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/sys/printk.h>

/* Xác định node Watchdog từ Devicetree cho STM32 */
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_watchdog)
#define WDT_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(st_stm32_watchdog)
#elif DT_NODE_HAS_STATUS_OKAY(DT_ALIAS(watchdog0))
#define WDT_NODE DT_ALIAS(watchdog0)
#elif DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(iwdg))
#define WDT_NODE DT_NODELABEL(iwdg)
#else
#define WDT_NODE DT_INVALID_NODE
#endif

#if DT_NODE_EXISTS(WDT_NODE)
static const struct device *const wdt_dev = DEVICE_DT_GET(WDT_NODE);
#else
static const struct device *const wdt_dev = NULL;
#endif

static int wdt_channel = 0;

int wdt_token_init(uint32_t timeout_ms)
{
	int ret;
	struct wdt_timeout_cfg wdt_config;

	if (timeout_ms == 0U) {
		return -EINVAL;
	}

	if (wdt_dev == NULL || !device_is_ready(wdt_dev)) {
		printk("[WDT] Loi: Watchdog device chua san sang hoac khong ton tai!\n");
		return -ENODEV;
	}

	/* Cấu hình cửa sổ timeout cho IWDG:
	 * STM32 IWDG là independent watchdog (không có window min),
	 * không hỗ trợ interrupt callback trước khi reset SOC.
	 */
	wdt_config.flags = WDT_FLAG_RESET_SOC;
	wdt_config.window.min = 0U;
	wdt_config.window.max = timeout_ms;
	wdt_config.callback = NULL;

	/* Cài đặt timeout và nhận channel ID */
	ret = wdt_install_timeout(wdt_dev, &wdt_config);
	if (ret >= 0) {
		wdt_channel = ret;
	}

	/* Kích hoạt Watchdog (nếu đã kích hoạt từ trước reset thì bỏ qua lỗi -EBUSY / -EALREADY) */
	ret = wdt_setup(wdt_dev, 0);
	if (ret < 0 && ret != -EBUSY && ret != -EALREADY) {
		printk("[WDT] Loi khi setup watchdog: %d\n", ret);
		return ret;
	}

	/* Feed ngay lập tức để nạp lại đầy đủ bộ đếm counter */
	wdt_feed(wdt_dev, wdt_channel);

	printk("[WDT] Khoi tao IWDG thanh cong! Timeout: %u ms, Channel ID: %d\n",
	       timeout_ms, wdt_channel);

	return 0;
}

void wdt_token_feed(void)
{
	if (wdt_dev == NULL) {
		return;
	}

	int ret = wdt_feed(wdt_dev, wdt_channel);
	if (ret != 0) {
		/* Thử feed channel 0 dự phòng */
		wdt_feed(wdt_dev, 0);
	}
}

#if defined(CONFIG_TOKEN_DEV) || defined(WDT_TOKEN_TEST)
void wdt_token_force_hang_for_test(void)
{
	printk("[WDT] !!! TEST: Buoc he thong treo (khong feed) de kich hoat Watchdog Reset !!!\n");
	for (;;) {
		/* Vòng lặp vô tận, chiếm CPU để không cho main loop gọi wdt_token_feed() */
	}
}
#endif