/*
 * copyright 2014~2015 advanced micro devices, inc.
 *
 * permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "software"),
 * to deal in the software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the software, and to permit persons to whom the
 * software is furnished to do so, subject to the following conditions:
 *
 * the above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef _FSL_STRING_H_
#define _FSL_STRING_H_

#include "fsl_interinstr.h"

#define END_OF_STREAM (1073741824)

typedef struct _string_stream_t {
	char *buf;
	int len;
	int read_pos;
} string_stream_t;

int init_string_stream(parser_context_t *context,
		string_stream_t *string_stream, char *buf, int len);
int get_line(parser_context_t *context, string_stream_t *string_stream,
		char *buf, int len);
int split(parser_context_t *context, char *line, int len, char *words,
		int max_words, int max_word_len);
int is_digital(const char *string, int en_hex, int *is_hex);

/*add following by bdu*/
typedef struct _SvmStringStream_t {
	char *buf;
	int len;
	int readPos;
} SvmStringStream_t;

int InitStringStream(SvmParserContext_t *context,
		SvmStringStream_t *stringStream, char *buf, int len);
int GetLine(SvmParserContext_t *context,
		SvmStringStream_t *stringStream, char *buf, int len);
int Split(SvmParserContext_t *context, char *line, int len,
		char *words, int maxWords, int maxWordLen);
int IsDigital(const char *string, int enHex, int *isHex);

#endif
