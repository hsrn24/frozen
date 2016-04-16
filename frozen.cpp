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

// int json_emit_long(char *buf, int buf_len, long int value) {
// char tmp[20];
// int n = snprintf(tmp, sizeof(tmp), "%ld", value);
// strncpy(buf, tmp, buf_len > 0 ? buf_len : 0);
// return n;
// }

// int json_emit_double(char *buf, int buf_len, double value) {
// char tmp[20];
// int n = snprintf(tmp, sizeof(tmp), "%g", value);
// strncpy(buf, tmp, buf_len > 0 ? buf_len : 0);
// return n;
// }

#define EMIT(x) emitChar(x)

char get_hex_char(char val)
{
	if (val < 10)
		return val + '0';
	else
		return val - 10 + 'a';
}

void JsonEmitter::json_emit_quoted_str(const char *str, int len)
{
	const char *str_end = str + len;
	char ch;

	EMIT('"');
	while (str < str_end) {
		ch = *str++;
		switch (ch) {
		case '"':
			EMIT('\\'); EMIT('"');
			break;
		case '\\':
			EMIT('\\'); EMIT('\\');
			break;
		case '\b':
			EMIT('\\'); EMIT('b');
			break;
		case '\f':
			EMIT('\\'); EMIT('f');
			break;
		case '\n':
			EMIT('\\'); EMIT('n');
			break;
		case '\r':
			EMIT('\\'); EMIT('r');
			break;
		case '\t':
			EMIT('\\'); EMIT('t');
			break;
		default:
			EMIT(ch);
		}
	}
	EMIT('"');
}

// void json_emit_unquoted_str(JsonEmitter* emitter, const char *str, int len) {
// if (buf_len > 0 && len > 0) {
// int n = len < buf_len ? len : buf_len;
// memcpy(buf, str, n);
// if (n < buf_len) {
// buf[n] = '\0';
// }
// }
// return len;
// }

// base64 encoder from Apple QuickTime source
// https://opensource.apple.com/source/QuickTimeStreamingServer/QuickTimeStreamingServer-452/CommonUtilitiesLib/base64.c
static const char basis_64[] =
  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

void JsonEmitter::json_emit_quoted_base64(const char *str, int len)
{
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
}

void JsonEmitter::json_emit_quoted_base64_callback(frozen_fun_t fun, void* user, int len)
{
	EMIT('"');

	int i;
	char s1, s2, s3;

	for (i = 0; i < len - 2; i += 3) {
		if (mode == 1) {
			s1 = fun(user);
			s2 = fun(user);
			s3 = fun(user);
		}
		EMIT(basis_64[(s1 >> 2) & 0x3F]);
		EMIT(basis_64[((s1 & 0x3) << 4) | ((int)(s2 & 0xF0) >> 4)]);
		EMIT(basis_64[((s2 & 0xF) << 2) | ((int)(s3 & 0xC0) >> 6)]);
		EMIT(basis_64[s3 & 0x3F]);
	}
	if (i < len) {
		if (mode == 1) {
			s1 = fun(user);
		}
		EMIT(basis_64[(s1 >> 2) & 0x3F]);
		if (i == (len - 1)) {
			EMIT(basis_64[((s1 & 0x3) << 4)]);
			EMIT('=');
		} else {
			if (mode == 1) {
				s2 = fun(user);
			}
			EMIT(basis_64[((s1 & 0x3) << 4) | ((int)(s2 & 0xF0) >> 4)]);
			EMIT(basis_64[((s2 & 0xF) << 2)]);
		}
		EMIT('=');
	}

	EMIT('"');
}

void JsonEmitter::json_emit_quoted_hex(const char *str, int len)
{
	EMIT('"');

	int i;

	for (i = 0; i < len; i++) {
		char c = str[i];
		EMIT(get_hex_char((c >> 4) & 0x0f));
		EMIT(get_hex_char(c & 0x0f));
	}

	EMIT('"');
}

int JsonEmitter::getSize()
{
	va_list tmpList;
	va_copy(tmpList, arg);
	size = 0;
	mode = 0;
	process(tmpList);
	return size;
}
int JsonEmitter::doEmit()
{
	va_list tmpList;
	va_copy(tmpList, arg);
	size = 0;
	mode = 1;
	process(tmpList);
	return size;
}
void JsonEmitter::process(va_list tmpList)
{
	size_t len;
	char* str;
	void* user;
	frozen_fun_t fn;

	const char* fmt = format;
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
			EMIT(*fmt);
			break;
			// case 'i':
			// s += json_emit_long(s, end - s, va_arg(ap, long) );
			// break;
			// case 'f':
			// s += json_emit_double(s, end - s, va_arg(ap, double) );
			break;
		case 'v':
			str = va_arg(tmpList, char*);
			len = va_arg(tmpList, size_t);
			json_emit_quoted_str(str, len);
			break;
		// case 'V':
		// str = va_arg(ap, char *);
		// len = va_arg(ap, size_t);
		// json_emit_unquoted_str(emitter, str, len);
		// break;
		case 's':
			str = va_arg(tmpList, char*);
			json_emit_quoted_str(str, strlen(str));
			break;
		// case 'S':
		// str = va_arg(ap, char *);
		// json_emit_unquoted_str(emitter, str, strlen(str));
		// break;
		case 'b':
			str = va_arg(tmpList, char*);
			len = va_arg(tmpList, size_t);
			json_emit_quoted_base64(str, len);
			break;
		case 'n':
			fn = va_arg(tmpList, frozen_fun_t);
			user = va_arg(tmpList, void*);
			len = va_arg(tmpList, size_t);
			json_emit_quoted_base64_callback(fn, user, len);
			break;
		case 'x':
			str = va_arg(tmpList, char*);
			len = va_arg(tmpList, size_t);
			json_emit_quoted_hex(str, len);
			break;
		// case 'T':
		// s += json_emit_unquoted_str(s, "true", 4);
		// break;
		// case 'F':
		// s += json_emit_unquoted_str(s, "false", 5);
		// break;
		// case 'N':
		// s += json_emit_unquoted_str(s, "null", 4);
		// break;
		default:
			break;
		}
		fmt++;
	}
	va_end(tmpList);
}

JsonEmitter::JsonEmitter(const char* format, va_list inArg)
	: format(format)
{
	va_copy(arg, inArg);
	va_end(inArg);
}
JsonEmitter::~JsonEmitter()
{
	va_end(arg);
}

void JsonEmitter::emitChar(char c)
{
	if (mode == 1 && putCharHandler)
		putCharHandler(c);
	size++;
}
