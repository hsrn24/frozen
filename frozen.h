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

#ifndef FROZEN_HEADER_INCLUDED
#define FROZEN_HEADER_INCLUDED

#include <stdarg.h>
#include <functional>

typedef char (*frozen_fun_t)(void*);
typedef void (*frozen_emit_fun_t)(void*, char);


class JsonEmitter {
public:
	typedef std::function<void(char c)> PutCharHandler;

	PutCharHandler putCharHandler;

	JsonEmitter(const char* format, va_list inArg);
	~JsonEmitter();

	int getSize();
	int doEmit();

private:
	const char* format;
	va_list arg;
	int size;
	int mode;

	void process(va_list tmpList);
	void emitChar(char c);

	int json_emit_long(long int value);
	void json_emit_quoted_str(const char *str, int len);
	void json_emit_quoted_base64(const char *str, int len);
	void json_emit_quoted_base64_callback(frozen_fun_t fun, void* user, int len);
	void json_emit_quoted_hex(const char *str, int len);
};

#endif /* FROZEN_HEADER_INCLUDED */

