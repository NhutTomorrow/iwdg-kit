#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>

#if defined(CONFIG_USB_DEVICE_STACK)
#include <zephyr/usb/usb_device.h>
#include <zephyr/drivers/uart.h>
#endif

#include "wdt_token.h"

/* Timing configuration per task requirements:
 * - Timeout: 4000 ms (4 seconds, requirement: 3000ms - 5000ms)
 * - Feed Interval: 1000 ms (1 second, requirement: < Timeout / 2)
 */
#define WDT_TIMEOUT_MS          4000U
#define WDT_FEED_INTERVAL_MS    1000U

/*
 * Test configuration:
 * After 120 cycles (~120 seconds), system calls wdt_token_force_hang_for_test()
 * to verify that the Watchdog automatically resets the MCU when hung.
 */
#define TEST_HANG_AFTER_CYCLES  120

/*
 * LED pin configurations across STM32 board variants:
 * - STM32F103RC Mini V2.0: LED1 on PA8, LED2 on PD2 (Active-Low)
 * - Blue/Black pill style: LED on PC13 (Active-Low)
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

	/* 1. Initialize candidate LED pins on board */
	if (gpioa_dev && device_is_ready(gpioa_dev)) {
		gpio_pin_configure(gpioa_dev, PIN_LED_D2, GPIO_OUTPUT_ACTIVE | GPIO_ACTIVE_LOW);
	}
	if (gpiod_dev && device_is_ready(gpiod_dev)) {
		gpio_pin_configure(gpioa_dev, PIN_LED_D3, GPIO_OUTPUT_ACTIVE | GPIO_ACTIVE_LOW);
	}

	/* 2. Initialize Watchdog */
	wdt_token_init(WDT_TIMEOUT_MS);
	// wdt_token_feed();

#if defined(CONFIG_USB_DEVICE_STACK)
	/* 3. Enable USB CDC-ACM Virtual Serial Port on Type-C */
	const struct device *const usb_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
	uint32_t dtr = 0;
	int wait_time = 0;

	if (usb_enable(NULL) == 0) {
		/* Wait for terminal connection (up to 2.5s) and feed watchdog continuously */
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
	printk("Watchdog initialized successfully:\n");
	printk(" - Timeout: %u ms\n", WDT_TIMEOUT_MS);
	printk(" - Feed Interval: %u ms\n", WDT_FEED_INTERVAL_MS);
#if (TEST_HANG_AFTER_CYCLES > 0) && (defined(CONFIG_TOKEN_DEV) || defined(WDT_TOKEN_TEST))
	printk(" - Test Mode: Force hang after %d cycles (~%d seconds)\n",
	       TEST_HANG_AFTER_CYCLES, (TEST_HANG_AFTER_CYCLES * WDT_FEED_INTERVAL_MS) / 1000);
#else
	printk(" - Test Mode: Normal operation (no forced hang)\n");
#endif
	printk("--------------------------------------------------\n");

	/* 4. Main loop */
	while (1) {
		loop_count++;

		/* Feed Watchdog every loop cycle */
		wdt_token_feed();

		/* Toggle LED state */
		led_state = !led_state;
		if (gpioa_dev && device_is_ready(gpioa_dev)) {
			gpio_pin_set(gpioa_dev, PIN_LED_D2, led_state);
		}
		if (gpiod_dev && device_is_ready(gpiod_dev)) {
			gpio_pin_set(gpioa_dev, PIN_LED_D3, led_state);
		}

		printk("[APP] Loop #%u: Watchdog fed successfully (LED: %s)\n",
		       loop_count, led_state ? "ON" : "OFF");

#if (TEST_HANG_AFTER_CYCLES > 0) && (defined(CONFIG_TOKEN_DEV) || defined(WDT_TOKEN_TEST))
		if (loop_count >= TEST_HANG_AFTER_CYCLES) {
			printk("\n[APP] !!! FORCING SYSTEM HANG TO TEST WATCHDOG RESET !!!\n");
			printk("[APP] Stopping feed, MCU will reset in ~%u ms...\n", WDT_TIMEOUT_MS);

			/* Force infinite loop to verify Watchdog Reset */
			wdt_token_force_hang_for_test();
		}
#endif

		k_msleep(WDT_FEED_INTERVAL_MS);
	}

	return 0;
}