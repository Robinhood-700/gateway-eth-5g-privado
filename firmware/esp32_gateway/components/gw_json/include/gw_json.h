#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

bool gw_json_is_valid(const uint8_t *data, size_t len);

/*
 * Ensures JSON output:
 * - if input is valid JSON, it copies input as-is.
 * - else it wraps binary payload as {"enc":"hex","data":"..."}.
 * Returns output length (without null terminator), or 0 on failure.
 */
size_t gw_json_ensure(const uint8_t *in, size_t in_len, char *out, size_t out_len);

#ifdef __cplusplus
}
#endif
