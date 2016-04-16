/*
 * Copyright (c) 2004-2013 Sergey Lyubka <valenok@gmail.com>
 * Copyright (c) 2013 Cesanta Software Limited
 * All rights reserved
 *
 * This library is dual-licensed: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation. For the terms of this
 * license, see <http: *www.gnu.org/licenses/>.
 *
 * You are free to use this library under the terms of the GNU General
 * Public License, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * Alternatively, you can license this library under a commercial
 * license, as set out in <http://cesanta.com/products.html>.
 */

#define _CRT_SECURE_NO_WARNINGS /* Disable deprecation warning in VS2005+ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "frozen.h"

#define EMIT(x)          \
  do {                   \
    if (s < end) *s = x; \
    s++;                 \
  } while (0)

int json_emit_long(char *buf, int buf_len, long int value) {
  char tmp[20];
  int n = snprintf(tmp, sizeof(tmp), "%ld", value);
  strncpy(buf, tmp, buf_len > 0 ? buf_len : 0);
  return n;
}

int json_emit_double(char *buf, int buf_len, double value) {
  char tmp[20];
  int n = snprintf(tmp, sizeof(tmp), "%g", value);
  strncpy(buf, tmp, buf_len > 0 ? buf_len : 0);
  return n;
}

int json_emit_quoted_str(char *s, int s_len, const char *str, int len) {
  const char *begin = s, *end = s + s_len, *str_end = str + len;
  char ch;

  EMIT('"');
  while (str < str_end) {
    ch = *str++;
    switch (ch) {
      case '"':
        EMIT('\\');
        EMIT('"');
        break;
      case '\\':
        EMIT('\\');
        EMIT('\\');
        break;
      case '\b':
        EMIT('\\');
        EMIT('b');
        break;
      case '\f':
        EMIT('\\');
        EMIT('f');
        break;
      case '\n':
        EMIT('\\');
        EMIT('n');
        break;
      case '\r':
        EMIT('\\');
        EMIT('r');
        break;
      case '\t':
        EMIT('\\');
        EMIT('t');
        break;
      default:
        EMIT(ch);
    }
  }
  EMIT('"');
  if (s < end) {
    *s = '\0';
  }

  return s - begin;
}

int json_emit_unquoted_str(char *buf, int buf_len, const char *str, int len) {
  if (buf_len > 0 && len > 0) {
    int n = len < buf_len ? len : buf_len;
    memcpy(buf, str, n);
    if (n < buf_len) {
      buf[n] = '\0';
    }
  }
  return len;
}

// base64 encoder from Apple QuickTime source
// https://opensource.apple.com/source/QuickTimeStreamingServer/QuickTimeStreamingServer-452/CommonUtilitiesLib/base64.c
static const char basis_64[] =
  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

int json_emit_quoted_base64(char *s, int s_len, const char *str, int len) {
  const char *begin = s, *end = s + s_len;

  EMIT('"');

  int i;

  for (i = 0; i < len - 2; i += 3) {
    EMIT(basis_64[(str[i] >> 2) & 0x3F]);
    EMIT(basis_64[((str[i] & 0x3) << 4) | ((int)(str[i + 1] & 0xF0) >> 4)]);
    EMIT(basis_64[((str[i + 1] & 0xF) << 2) | ((int)(str[i + 2] & 0xC0) >> 6)]);
    EMIT(basis_64[str[i + 2] & 0x3F]);
  }
  if (i < len) {
    EMIT(basis_64[(str[i] >> 2) & 0x3F]);
    if (i == (len - 1)) {
      EMIT(basis_64[((str[i] & 0x3) << 4)]);
      EMIT('=');
    } else {
      EMIT(basis_64[((str[i] & 0x3) << 4) | ((int)(str[i + 1] & 0xF0) >> 4)]);
      EMIT(basis_64[((str[i + 1] & 0xF) << 2)]);
    }
    EMIT('=');
  }

  EMIT('"');
  if (s < end) {
    *s = '\0';
  }

  return s - begin;
}

int json_emit_quoted_base64_callback(char *s, int s_len, frozen_fun_t fun, void* user, int len) {
  const char *begin = s, *end = s + s_len;

  EMIT('"');

  int i;

  for (i = 0; i < len - 2; i += 3) {
    char s1 = fun(user);
    char s2 = fun(user);
    char s3 = fun(user);
    EMIT(basis_64[(s1 >> 2) & 0x3F]);
    EMIT(basis_64[((s1 & 0x3) << 4) | ((int)(s2 & 0xF0) >> 4)]);
    EMIT(basis_64[((s2 & 0xF) << 2) | ((int)(s3 & 0xC0) >> 6)]);
    EMIT(basis_64[s3 & 0x3F]);
  }
  if (i < len) {
    char s1 = fun(user);
    EMIT(basis_64[(s1 >> 2) & 0x3F]);
    if (i == (len - 1)) {
      EMIT(basis_64[((s1 & 0x3) << 4)]);
      EMIT('=');
    } else {
      char s2 = fun(user);
      EMIT(basis_64[((s1 & 0x3) << 4) | ((int)(s2 & 0xF0) >> 4)]);
      EMIT(basis_64[((s2 & 0xF) << 2)]);
    }
    EMIT('=');
  }

  EMIT('"');
  if (s < end) {
    *s = '\0';
  }

  return s - begin;
}


char get_hex_char(char val)
{
  if (val < 10)
    return val + '0';
  else
    return val - 10 + 'a';
}

int json_emit_quoted_hex(char *s, int s_len, const char *str, int len) {
  const char *begin = s, *end = s + s_len;

  EMIT('"');

  int i;

  for (i = 0; i < len; i++) {
    char c = str[i];
    EMIT(get_hex_char((c >> 4) & 0x0f));
    EMIT(get_hex_char(c & 0x0f));
  }

  EMIT('"');
  if (s < end) {
    *s = '\0';
  }

  return s - begin;
}

int json_emit_va(char *s, int s_len, const char *fmt, va_list ap) {
  const char *end = s + s_len, *str, *orig = s;
  size_t len;

  while (*fmt != '\0') {
    switch (*fmt) {
      case ' ':
        break;
      case '[':
      case ']':
      case '{':
      case '}':
      case ',':
      case ':':
      case '\r':
      case '\n':
      case '\t':
        if (s < end) {
          *s = *fmt;
        }
        s++;
        break;
      case 'i':
        s += json_emit_long(s, end - s, va_arg(ap, long) );
        break;
      case 'f':
        s += json_emit_double(s, end - s, va_arg(ap, double) );
        break;
      case 'v':
        str = va_arg(ap, char *);
        len = va_arg(ap, size_t);
        s += json_emit_quoted_str(s, end - s, str, len);
        break;
      case 'V':
        str = va_arg(ap, char *);
        len = va_arg(ap, size_t);
        s += json_emit_unquoted_str(s, end - s, str, len);
        break;
      case 's':
        str = va_arg(ap, char *);
        s += json_emit_quoted_str(s, end - s, str, strlen(str));
        break;
      case 'S':
        str = va_arg(ap, char *);
        s += json_emit_unquoted_str(s, end - s, str, strlen(str));
        break;
      case 'b':
        str = va_arg(ap, char *);
        len = va_arg(ap, size_t);
        s += json_emit_quoted_base64(s, end - s, str, len);
        break;
      case 'n':
        {
          frozen_fun_t fn = va_arg(ap, frozen_fun_t);
          void* user = va_arg(ap, void*);
          len = va_arg(ap, size_t);
          s += json_emit_quoted_base64_callback(s, end - s, fn, user, len);
        }
        break;
      case 'x':
        str = va_arg(ap, char *);
        len = va_arg(ap, size_t);
        s += json_emit_quoted_hex(s, end - s, str, len);
        break;
      case 'T':
        s += json_emit_unquoted_str(s, end - s, "true", 4);
        break;
      case 'F':
        s += json_emit_unquoted_str(s, end - s, "false", 5);
        break;
      case 'N':
        s += json_emit_unquoted_str(s, end - s, "null", 4);
        break;
      default:
        return 0;
    }
    fmt++;
  }

  /* Best-effort to 0-terminate generated string */
  if (s < end) {
    *s = '\0';
  }

  return s - orig;
}

int json_emit(char *buf, int buf_len, const char *fmt, ...) {
  int len;
  va_list ap;

  va_start(ap, fmt);
  len = json_emit_va(buf, buf_len, fmt, ap);
  va_end(ap);

  return len;
}
