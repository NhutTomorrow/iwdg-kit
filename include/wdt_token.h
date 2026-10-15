#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Arm IWDG after system init. Feature does not exist in tree yet.
 * @param timeout_ms  Target timeout (product target: 3000–5000 ms).
 * @return 0 or negative errno.
 */
int wdt_token_init(uint32_t timeout_ms);

/** Feed IWDG. Must be called from the main loop (and any long wait paths). */
void wdt_token_feed(void);

#if defined(CONFIG_TOKEN_DEV) || defined(WDT_TOKEN_TEST)
void wdt_token_force_hang_for_test(void);
#endif

#ifdef __cplusplus
}
#endif
