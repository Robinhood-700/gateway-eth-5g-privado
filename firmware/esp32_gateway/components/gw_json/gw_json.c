#include "gw_json.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

static const char *skip_ws(const char *p, const char *end)
{
    while (p < end && isspace((unsigned char)*p)) {
        p++;
    }
    return p;
}

bool gw_json_is_valid(const uint8_t *data, size_t len)
{
    if (data == NULL || len == 0) {
        return false;
    }

    const char *p = (const char *)data;
    const char *end = p + len;
    p = skip_ws(p, end);
    if (p >= end) {
        return false;
    }

    char first = *p;
    if (first != '{' && first != '[') {
        return false;
    }

    int brace = 0;
    int bracket = 0;
    bool in_string = false;
    bool escaped = false;

    for (; p < end; ++p) {
        char c = *p;

        if (in_string) {
            if (escaped) {
                escaped = false;
                continue;
            }
            if (c == '\\') {
                escaped = true;
                continue;
            }
            if (c == '"') {
                in_string = false;
            }
            continue;
        }

        if (c == '"') {
            in_string = true;
            continue;
        }

        if (c == '{') {
            brace++;
        } else if (c == '}') {
            brace--;
            if (brace < 0) {
                return false;
            }
        } else if (c == '[') {
            bracket++;
        } else if (c == ']') {
            bracket--;
            if (bracket < 0) {
                return false;
            }
        }
    }

    if (in_string || brace != 0 || bracket != 0) {
        return false;
    }

    const char *tail = end;
    while (tail > (const char *)data && isspace((unsigned char)tail[-1])) {
        --tail;
    }
    if (tail <= (const char *)data) {
        return false;
    }

    char last = tail[-1];
    return ((first == '{' && last == '}') || (first == '[' && last == ']'));
}

static size_t hex_wrap(const uint8_t *in, size_t in_len, char *out, size_t out_len)
{
    static const char hexdig[] = "0123456789ABCDEF";
    const char *head = "{\"enc\":\"hex\",\"data\":\"";
    const char *tail = "\"}";
    const size_t base = strlen(head) + strlen(tail);

    if (out == NULL || out_len < base + (2 * in_len) + 1) {
        return 0;
    }

    size_t off = 0;
    memcpy(out + off, head, strlen(head));
    off += strlen(head);

    for (size_t i = 0; i < in_len; ++i) {
        uint8_t b = in[i];
        out[off++] = hexdig[(b >> 4) & 0x0F];
        out[off++] = hexdig[b & 0x0F];
    }

    memcpy(out + off, tail, strlen(tail));
    off += strlen(tail);
    out[off] = '\0';
    return off;
}

size_t gw_json_ensure(const uint8_t *in, size_t in_len, char *out, size_t out_len)
{
    if (in == NULL || in_len == 0 || out == NULL || out_len < 2) {
        return 0;
    }

    if (gw_json_is_valid(in, in_len)) {
        if (out_len < (in_len + 1)) {
            return 0;
        }
        memcpy(out, in, in_len);
        out[in_len] = '\0';
        return in_len;
    }

    return hex_wrap(in, in_len, out, out_len);
}
