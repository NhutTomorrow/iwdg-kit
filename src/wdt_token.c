#include "wdt_token.h"

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/sys/printk.h>

/* Determine Watchdog node from Devicetree for STM32 */
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
		printk("[WDT] Error: Watchdog device is not ready or does not exist!\n");
		return -ENODEV;
	}

	/* Configure IWDG timeout window:
	 * STM32 IWDG is an independent watchdog (no minimum window),
	 * and does not support interrupt callbacks prior to SOC reset.
	 */
	wdt_config.flags = WDT_FLAG_RESET_SOC;
	wdt_config.window.min = 0U;
	wdt_config.window.max = timeout_ms;
	wdt_config.callback = NULL;

	/* Install timeout configuration and retrieve channel ID */
	ret = wdt_install_timeout(wdt_dev, &wdt_config);
	if (ret < 0) {
		printk("[WDT] Error installing timeout: %d\n", ret);
		return ret;
	}
	wdt_channel = ret;

	/* Start Watchdog (ignore -EBUSY / -EALREADY if previously started prior to reset) */
	ret = wdt_setup(wdt_dev, 0);
	if (ret < 0 && ret != -EBUSY && ret != -EALREADY) {
		printk("[WDT] Error setting up watchdog: %d\n", ret);
		return ret;
	}

	/* Feed immediately to fully reload the down-counter */
	wdt_feed(wdt_dev, wdt_channel);

	printk("[WDT] IWDG initialized successfully! Timeout: %u ms, Channel ID: %d\n",
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
		/* Fallback: try feeding channel 0 */
		wdt_feed(wdt_dev, 0);
	}
}

#if defined(CONFIG_TOKEN_DEV) || defined(WDT_TOKEN_TEST)
void wdt_token_force_hang_for_test(void)
{
	printk("[WDT] !!! TEST: Entering infinite loop to trigger Watchdog Reset !!!\n");
	for (;;) {
		
	}
}
#endif