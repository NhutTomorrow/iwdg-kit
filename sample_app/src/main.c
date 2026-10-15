#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>

#if defined(CONFIG_USB_DEVICE_STACK)
#include <zephyr/usb/usb_device.h>
#include <zephyr/drivers/uart.h>
#endif

#include "wdt_token.h"

/* Cấu hình thời gian theo yêu cầu bài toán:
 * - Timeout: 4000 ms (4 giây, yêu cầu: 3000ms - 5000ms)
 * - Chu kỳ Feed: 1000 ms (1 giây, yêu cầu: < Timeout / 2)
 */
#define WDT_TIMEOUT_MS          4000U
#define WDT_FEED_INTERVAL_MS    1000U

/*
 * Cấu hình kiểm thử:
 * Sau 10 chu kỳ (~10 giây), hệ thống sẽ gọi wdt_token_force_hang_for_test()
 * để chứng minh Watchdog tự động reset vi điều khiển khi bị treo.
 */
#define TEST_HANG_AFTER_CYCLES  10

/*
 * Các chân LED trên các phiên bản board STM32F103RCT6 Mini:
 * - Bản chuẩn STM32F103RC Mini V2.0: LED1 nối PA8, LED2 nối PD2 (Active-Low)
 * - Bản tối giản (Blue/Black style): LED nối PC13 (Active-Low)
 */
static const struct device *const gpioa_dev = DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpioa));
static const struct device *const gpioc_dev = DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpioc));
static const struct device *const gpiod_dev = DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpiod));

#define PIN_LED_D2   6   /* GPIOA Pin 8 */
#define PIN_LED_D3   7   /* GPIOD Pin 2 */
#define PIN_LED_DX  1   /* GPIOC Pin 13 */

int main(void)
{
	uint32_t loop_count = 0;
	int led_state = 1;

	/* 1. Khởi tạo tất cả các chân LED ứng viên trên board F103RC */
	if (gpioa_dev && device_is_ready(gpioa_dev)) {
		gpio_pin_configure(gpioa_dev, PIN_LED_D2, GPIO_OUTPUT_ACTIVE | GPIO_ACTIVE_LOW);
	}
	if (gpiod_dev && device_is_ready(gpiod_dev)) {
		gpio_pin_configure(gpioa_dev, PIN_LED_D3, GPIO_OUTPUT_ACTIVE | GPIO_ACTIVE_LOW);
	}

	/* 2. Khởi tạo Watchdog và Feed ngay */
	wdt_token_init(WDT_TIMEOUT_MS);
	wdt_token_feed();

#if defined(CONFIG_USB_DEVICE_STACK)
	/* 3. Kích hoạt cổng USB Serial ảo trên Type-C */
	const struct device *const usb_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
	uint32_t dtr = 0;
	int wait_time = 0;

	if (usb_enable(NULL) == 0) {
		/* Đợi người dùng mở terminal (tối đa 2.5s) và feed watchdog liên tục */
		while (!dtr && wait_time < 25) {
			uart_line_ctrl_get(usb_dev, UART_LINE_CTRL_DTR, &dtr);
			wdt_token_feed();
			k_msleep(100);
			wait_time++;
		}
	}
#endif

	printk("\n");
	printk("==================================================\n");
	printk("***   STM32 IWDG Sample Application Started   ***\n");
	printk("==================================================\n");
	printk("Watchdog da khoi tao thanh cong:\n");
	printk(" - Timeout: %u ms\n", WDT_TIMEOUT_MS);
	printk(" - Chu ky Feed: %u ms\n", WDT_FEED_INTERVAL_MS);
#if (TEST_HANG_AFTER_CYCLES > 0) && (defined(CONFIG_TOKEN_DEV) || defined(WDT_TOKEN_TEST))
	printk(" - Che do test: Buoc treo sau %d chu ky (~%d giay)\n",
	       TEST_HANG_AFTER_CYCLES, (TEST_HANG_AFTER_CYCLES * WDT_FEED_INTERVAL_MS) / 1000);
#else
	printk(" - Che do test: Chay binh thuong khong treo\n");
#endif
	printk("--------------------------------------------------\n");

	/* 4. Vòng lặp chính */
	while (1) {
		loop_count++;

		/* Feed Watchdog mỗi chu kỳ lặp */
		wdt_token_feed();

		/* Đảo trạng thái đèn LED */
		led_state = !led_state;
		if (gpioa_dev && device_is_ready(gpioa_dev)) {
			gpio_pin_set(gpioa_dev, PIN_LED_D2, led_state);
		}
		if (gpiod_dev && device_is_ready(gpiod_dev)) {
			gpio_pin_set(gpioa_dev, PIN_LED_D3, led_state);
		}
		

		printk("[APP] Loop #%u: Da feed Watchdog thanh cong (LED: %s)\n",
		       loop_count, led_state ? "SANG" : "TAT");

#if (TEST_HANG_AFTER_CYCLES > 0) && (defined(CONFIG_TOKEN_DEV) || defined(WDT_TOKEN_TEST))
		if (loop_count >= TEST_HANG_AFTER_CYCLES) {
			printk("\n[APP] !!! BUOC TREO HE THONG DE TEST WATCHDOG RESET !!!\n");
			printk("[APP] Dung feed, MCU se tu Reset sau dung ~%u ms...\n", WDT_TIMEOUT_MS);

			/* Treo hệ thống để kiểm chứng Watchdog Reset */
			wdt_token_force_hang_for_test();
		}
#endif

		k_msleep(WDT_FEED_INTERVAL_MS);
	}

	return 0;
}