/* Nordstjernen — WebP decode via libwebp.
 * Copyright 2026 Andreas Røsdal
 * SPDX-License-Identifier: LicenseRef-NSL-1.0
 */

#include "image.h"

#include <string.h>
#include <webp/decode.h>

enum {
    NS_WEBP_MAX_DIM    = 16384,
    NS_WEBP_MAX_PIXELS = 64 * 1024 * 1024,
    NS_WEBP_MAX_INPUT  = 64 * 1024 * 1024,
};

gboolean
ns_image_webp_supports_bytes(const guchar *data, gsize len)
{
    return data && len >= 12 &&
           memcmp(data, "RIFF", 4) == 0 &&
           memcmp(data + 8, "WEBP", 4) == 0;
}

static guint8 *
ns_image_webp_decode_premultiplied(const guchar *data, gsize len,
                                   int *out_w, int *out_h, gsize *out_stride)
{
    if (!ns_image_webp_supports_bytes(data, len)) return NULL;
    if (len > NS_WEBP_MAX_INPUT) return NULL;
    int w = 0, h = 0;
    if (!WebPGetInfo(data, len, &w, &h)) return NULL;
    if (w <= 0 || h <= 0 || w > NS_WEBP_MAX_DIM || h > NS_WEBP_MAX_DIM ||
        (guint64)w * (guint64)h > (guint64)NS_WEBP_MAX_PIXELS) return NULL;
    gsize stride = (gsize)w * 4;
    gsize size = stride * (gsize)h;
    guint8 *pix = g_try_malloc(size);
    if (!pix) return NULL;
    if (!WebPDecodeBGRAInto(data, len, pix, size, (int)stride)) {
        g_free(pix);
        return NULL;
    }
    for (gsize i = 0; i < size; i += 4) {
        guint a = pix[i + 3];
        if (a == 255) continue;
        pix[i]     = (guint8)((pix[i]     * a + 127) / 255);
        pix[i + 1] = (guint8)((pix[i + 1] * a + 127) / 255);
        pix[i + 2] = (guint8)((pix[i + 2] * a + 127) / 255);
    }
    if (out_w) *out_w = w;
    if (out_h) *out_h = h;
    if (out_stride) *out_stride = stride;
    return pix;
}

ns_texture *
ns_image_decode_webp(const guchar *data, gsize len, int *out_w, int *out_h)
{
    int w = 0, h = 0;
    gsize stride = 0;
    guint8 *pix = ns_image_webp_decode_premultiplied(data, len, &w, &h,
                                                     &stride);
    if (!pix) return NULL;
    GBytes *bytes = g_bytes_new_take(pix, stride * (gsize)h);
    ns_texture *tex = ns_texture_new(w, h, NS_TEXTURE_BGRA_PREMULTIPLIED,
                                     bytes, stride);
    g_bytes_unref(bytes);
    if (tex) {
        if (out_w) *out_w = w;
        if (out_h) *out_h = h;
    }
    return tex;
}

guint8 *
ns_image_webp_decode_to_bgra(const guchar *data, gsize len,
                             int *out_w, int *out_h,
                             gsize *out_stride, gsize *out_buf_len)
{
    int w = 0, h = 0;
    gsize stride = 0;
    guint8 *pix = ns_image_webp_decode_premultiplied(data, len, &w, &h,
                                                     &stride);
    if (!pix) return NULL;
    if (out_w) *out_w = w;
    if (out_h) *out_h = h;
    if (out_stride) *out_stride = stride;
    if (out_buf_len) *out_buf_len = stride * (gsize)h;
    return pix;
}
