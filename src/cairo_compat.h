/* Nordstjernen — thin Cairo version-compat helpers.
 * Copyright 2026 Andreas Røsdal
 * SPDX-License-Identifier: LicenseRef-NSL-1.0
 */

#ifndef NORDSTJERNEN_CAIRO_COMPAT_H
#define NORDSTJERNEN_CAIRO_COMPAT_H

#include <cairo.h>
#include <glib.h>

/* Turn on Cairo 1.18+ gradient dithering for a gradient/mesh pattern. Smooth
 * CSS and <canvas> gradients otherwise show visible banding in 8-bit colour;
 * dithering (exposed to patterns in Cairo 1.18) hides it for a small, purely
 * visual-quality win. No-op when built against older Cairo (NS_HAVE_CAIRO_DITHER
 * unset), and can be turned off at runtime with NS_GRADIENT_DITHER=0 to A/B the
 * effect without rebuilding. */
static inline void
ns_cairo_gradient_dither(cairo_pattern_t *pat)
{
#ifdef NS_HAVE_CAIRO_DITHER
    static int enabled = -1;
    if (enabled < 0) {
        const char *e = g_getenv("NS_GRADIENT_DITHER");
        enabled = (e && e[0] == '0' && e[1] == '\0') ? 0 : 1;
    }
    if (pat && enabled)
        cairo_pattern_set_dither(pat, CAIRO_DITHER_BEST);
#else
    (void)pat;
#endif
}

#endif /* NORDSTJERNEN_CAIRO_COMPAT_H */
