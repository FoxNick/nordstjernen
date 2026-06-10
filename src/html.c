/* Nordstjernen — HTML helper utilities shared across the lexbor frontend.
 * Copyright 2026 Andreas Røsdal
 * SPDX-License-Identifier: LicenseRef-NSL-1.0
 */

#include "html.h"

#include <string.h>
#include <uchardet.h>

#include <lexbor/core/base.h>

gboolean
ns_html_is_void(const char *tag)
{
    if (!tag) return FALSE;
    static const char *const voids[] = {
        "area", "base", "br", "col", "embed", "hr", "img", "input",
        "link", "meta", "param", "source", "track", "wbr",
        NULL,
    };
    for (int i = 0; voids[i]; i++)
        if (strcmp(tag, voids[i]) == 0)
            return TRUE;
    return FALSE;
}

gboolean
ns_html_is_raw_text(const char *tag)
{
    if (!tag) return FALSE;
    static const char *const raws[] = {
        "script", "style", "xmp", "iframe", "noembed", "noframes",
        "noscript", "plaintext",
        NULL,
    };
    for (int i = 0; raws[i]; i++)
        if (g_ascii_strcasecmp(tag, raws[i]) == 0)
            return TRUE;
    return FALSE;
}

void
ns_html_escape_append(GString *out, const char *s, gboolean escape_quotes)
{
    for (const char *p = s ? s : ""; *p; p++) {
        switch (*p) {
        case '&': g_string_append(out, "&amp;"); break;
        case '<': g_string_append(out, "&lt;");  break;
        case '>': g_string_append(out, "&gt;");  break;
        case '"':
            if (escape_quotes) g_string_append(out, "&quot;");
            else               g_string_append_c(out, '"');
            break;
        case '\xc2':
            if ((unsigned char)p[1] == 0xa0) {
                g_string_append(out, "&nbsp;");
                p++;
            } else {
                g_string_append_c(out, *p);
            }
            break;
        default:  g_string_append_c(out, *p);    break;
        }
    }
}

char *
ns_html_escape_text(const char *s)
{
    GString *g = g_string_new(NULL);
    ns_html_escape_append(g, s, TRUE);
    return g_string_free(g, FALSE);
}

const char *
ns_html_engine_name(void)
{
    return "lexbor";
}

const char *
ns_html_engine_version(void)
{
#ifdef NS_LEXBOR_VERSION
    return NS_LEXBOR_VERSION;
#else
    return LEXBOR_VERSION_STRING;
#endif
}

char *
ns_html_image_document(const char *url)
{
    const char *u = url ? url : "";
    char *esc = ns_html_escape_text(u);
    char *name = g_path_get_basename(u);
    char *query = strchr(name, '?');
    if (query) *query = '\0';
    char *esc_name = ns_html_escape_text(*name ? name : "image");
    char *html = g_strdup_printf(
        "<!DOCTYPE html><html><head><title>%s</title><style>"
        "html,body{margin:0;min-height:100vh}"
        "body{background:#1c1d1e;text-align:center}"
        "img{max-width:100vw;max-height:100vh}"
        "</style></head><body><img src=\"%s\" alt=\"\"></body></html>",
        esc_name, esc);
    g_free(esc);
    g_free(esc_name);
    g_free(name);
    return html;
}

static char *
charset_normalize(const char *name)
{
    static const struct { const char *label; const char *iconv_name; } map[] = {
        { "gb2312",          "GBK" },
        { "gb_2312-80",      "GBK" },
        { "csgb2312",        "GBK" },
        { "iso-8859-1",      "WINDOWS-1252" },
        { "latin1",          "WINDOWS-1252" },
        { "ascii",           "WINDOWS-1252" },
        { "us-ascii",        "WINDOWS-1252" },
        { "utf8",            "UTF-8" },
        { "big5-hkscs",      "BIG5-HKSCS" },
        { "x-cp1251",        "WINDOWS-1251" },
        { "koi8_r",          "KOI8-R" },
        { "koi",             "KOI8-R" },
        { "x-mac-cyrillic",  "MAC-CYRILLIC" },
        { "x-mac-ukrainian", "MAC-CYRILLIC" },
        { "maccyrillic",     "MAC-CYRILLIC" },
        { "x-cp1252",        "WINDOWS-1252" },
        { "x-cp1250",        "WINDOWS-1250" },
        { "shift_jis",       "CP932" },
        { "shift-jis",       "CP932" },
        { "sjis",            "CP932" },
        { "x-sjis",          "CP932" },
        { "ms_kanji",        "CP932" },
        { "csshiftjis",      "CP932" },
        { "windows-31j",     "CP932" },
        { "x-euc-jp",        "EUC-JP" },
        { "eucjp",           "EUC-JP" },
    };
    for (gsize i = 0; i < G_N_ELEMENTS(map); i++)
        if (g_ascii_strcasecmp(name, map[i].label) == 0)
            return g_strdup(map[i].iconv_name);
    return g_ascii_strup(name, -1);
}

static char *
charset_value_in(const char *s, gsize len)
{
    for (gsize i = 0; i + 7 < len; i++) {
        if (g_ascii_strncasecmp(s + i, "charset", 7) != 0) continue;
        gsize p = i + 7;
        while (p < len && g_ascii_isspace(s[p])) p++;
        if (p >= len || s[p] != '=') continue;
        p++;
        while (p < len && g_ascii_isspace(s[p])) p++;
        if (p < len && (s[p] == '"' || s[p] == '\'')) p++;
        gsize start = p;
        while (p < len && (g_ascii_isalnum(s[p]) || s[p] == '-' ||
                           s[p] == '_' || s[p] == ':' || s[p] == '.'))
            p++;
        if (p > start && p - start < 40)
            return g_strndup(s + start, p - start);
    }
    return NULL;
}

static void
charset_report(char **charset_out, const char *name)
{
    if (charset_out && !*charset_out) *charset_out = g_strdup(name);
}

char *
ns_html_decode_body_full(const char *body, gsize len,
                         const char *content_type, char **charset_out)
{
    if (charset_out) *charset_out = NULL;
    if (!body || len == 0) return g_strdup("");

    if (len >= 3 && memcmp(body, "\xef\xbb\xbf", 3) == 0) {
        charset_report(charset_out, "UTF-8");
        return g_utf8_make_valid(body + 3, (gssize)(len - 3));
    }
    if (len >= 2 && memcmp(body, "\xff\xfe", 2) == 0) {
        char *out = g_convert(body, (gssize)len, "UTF-8", "UTF-16LE",
                              NULL, NULL, NULL);
        if (out) {
            charset_report(charset_out, "UTF-16LE");
            return out;
        }
    }
    if (len >= 2 && memcmp(body, "\xfe\xff", 2) == 0) {
        char *out = g_convert(body, (gssize)len, "UTF-8", "UTF-16BE",
                              NULL, NULL, NULL);
        if (out) {
            charset_report(charset_out, "UTF-16BE");
            return out;
        }
    }

    char *declared = content_type
        ? charset_value_in(content_type, strlen(content_type)) : NULL;
    if (!declared)
        declared = charset_value_in(body, len < 1024 ? len : 1024);
    if (declared) {
        char *cs = charset_normalize(declared);
        g_free(declared);
        if (g_ascii_strcasecmp(cs, "UTF-8") != 0) {
            char *out = g_convert(body, (gssize)len, "UTF-8", cs,
                                  NULL, NULL, NULL);
            if (out) {
                charset_report(charset_out, cs);
                g_free(cs);
                return out;
            }
        }
        g_free(cs);
    }

    if (g_utf8_validate(body, (gssize)len, NULL)) {
        charset_report(charset_out, "UTF-8");
        return g_strndup(body, len);
    }

    char *charset = NULL;
    uchardet_t det = uchardet_new();
    if (det) {
        gsize scan = len < (gsize)1024 * 1024 ? len : (gsize)1024 * 1024;
        if (uchardet_handle_data(det, body, scan) == 0) {
            uchardet_data_end(det);
            const char *name = uchardet_get_charset(det);
            if (name && *name
                && g_ascii_strcasecmp(name, "ASCII") != 0
                && g_ascii_strcasecmp(name, "UTF-8") != 0)
                charset = g_strdup(name);
        }
        uchardet_delete(det);
    }

    if (charset) {
        char *out = g_convert(body, (gssize)len, "UTF-8", charset,
                              NULL, NULL, NULL);
        if (out) {
            charset_report(charset_out, charset);
            g_free(charset);
            return out;
        }
        g_free(charset);
    }

    char *latin1 = g_convert(body, (gssize)len, "UTF-8", "WINDOWS-1252",
                             NULL, NULL, NULL);
    if (latin1) {
        charset_report(charset_out, "WINDOWS-1252");
        return latin1;
    }

    charset_report(charset_out, "UTF-8");
    return g_utf8_make_valid(body, (gssize)len);
}

char *
ns_html_decode_body(const char *body, gsize len)
{
    return ns_html_decode_body_full(body, len, NULL, NULL);
}
