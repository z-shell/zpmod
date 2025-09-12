#pragma once
/* SPDX-License-Identifier: MIT */
/**
 * @file zpmod_vendor_shims.h
 * @brief Local, non-invasive shims to suppress benign vendor header warnings.
 *
 * This header centralizes workaround macros instead of patching vendored
 * sources under vendor/zsh. Include it immediately after the canonical
 * gateway headers ("zpmod.mdh" then "zpmod.pro").
 */
#ifndef ZPMOD_VENDOR_SHIMS_H
#define ZPMOD_VENDOR_SHIMS_H

/* Suppress GCC macro redefinition noise for alloca when both glibc and
 * vendor zsh define it; only apply if standard headers already provided
 * the macro. We purposely do NOT redefine functional behavior. */
#if defined(__GNUC__) && defined(alloca) && !defined(ZPMOD_SUPPRESS_ALLOCA_WARN)
#define ZPMOD_SUPPRESS_ALLOCA_WARN 1
/* Rely on existing definition; nothing to do besides having a named flag
 * that could be used for conditional diagnostic pragmas if desired. */
#endif

#endif /* ZPMOD_VENDOR_SHIMS_H */
