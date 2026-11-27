/* Minimal types/macros for syntax-only compilation with real LCD register defs.
 * This is NOT a vendor SDK or hardware simulation.
 */
#ifndef DISPLAY_TEST_SYNTAX_SDK_H
#define DISPLAY_TEST_SYNTAX_SDK_H
#include <stdint.h>
#include <stddef.h>
#define __IO volatile
#define __I volatile const
#define __O volatile
#define __STATIC_INLINE static inline
#define __inline inline
#define assert_param(x) ((void)0)
#define DISPLAY_CTRL_BASE 0x40017000u
#define DISPLAY_CTRL_REG_BASE DISPLAY_CTRL_BASE
#define ENABLE 1
#define DISABLE 0
#define SET 1
#define RESET 0
typedef int FunctionalState;
typedef int ITStatus;
typedef int FlagStatus;
#define BIT(x) (1u << (x))
#define __REV(x) __builtin_bswap32((uint32_t)(x))
