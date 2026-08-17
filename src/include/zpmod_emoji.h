/* SPDX-License-Identifier: MIT */
#pragma once

/**
 * @file zpmod_emoji.h
 * @brief Optional terminal/locale detection for emoji support in messages.
 */

/**
 * @brief Determine if emoji/icons should be printed.
 *
 * Checks environment and locale; cached after first call.
 * @return 1 if enabled, 0 otherwise.
 */
int zp_icons_enabled(void);

/**
 * @brief Return the given icon string if icons are enabled, else empty string.
 *
 * @param s UTF-8 icon string (e.g., "⏱️ ").
 * @return s or "" depending on `zp_icons_enabled()`.
 */
const char *zp_icon(const char *s);
