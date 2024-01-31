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

#include "sensor_if.h"
#include "fsl_error.h"
#include "fsl_string.h"

int init_string_stream(parser_context_t *context,
		string_stream_t *string_stream, char *buf, int len)
{
	if ((!buf) || (!string_stream)) {
		set_error_info(context, "script stream buf is NULL\n");
		return -1;
	}

	string_stream->buf = buf;
	string_stream->len = len;
	string_stream->read_pos = 0;
	return 0;
}

int get_line(parser_context_t *context, string_stream_t *string_stream,
		char *buf, int len)
{
	int bytes;
	char data;
	int loop = 1;

	if ((!string_stream) || (!buf)) {
		set_error_info(context, "stream buf is NULL\n");
		return -1;
	}

	if (len < 1) {
		set_error_info(context, "line buf len is 0\n");
		return -1;
	}

	bytes = 0;

	if (string_stream->read_pos >= (string_stream->len))
		return END_OF_STREAM;

	while (loop) {
		if (string_stream->read_pos >= string_stream->len) {
			/*the end of the stream */
			buf[bytes] = 0;
			break;
		}

		data = string_stream->buf[string_stream->read_pos];
		string_stream->read_pos++;

		/*'\r' only exist on windows platform,
		 *and it will always after with '\n'
		 */
		if (data == '\r') {
			continue;
		} else if (data == '\n') {
			buf[bytes] = 0;	/*string should be end of zero. */
			break;
		}
		buf[bytes++] = data;

		if (bytes >= (len - 1))
			break;

		continue;
	}

	return bytes;
}

int split(parser_context_t *context, char *line, int len, char *words,
		int max_words, int max_word_len)
{
	int i, j;
	int bytes;
	char data;

	j = 0;
	bytes = 0;

	for (i = 0; i < len; i++) {
		data = line[i];

		if ((data != ' ') && (data != '\t') && (data != ',')) {
			if (data == ';') {	/*the end of the instruction */
				if (bytes > 0) {
					/*word string should be end of zero. */
					(words + j * max_word_len)[bytes] = 0;
					/*return the number of words */
					return j + 1;
				} else
					return j;
			} else {
				if (j >= max_words) {
					set_error_info(context,
					"too many words in one line, max:%d\n",
						max_words);
					return -1;
				}

				if (bytes >= max_word_len) {
					set_error_info(context,
						"word len too long, max:%d\n",
						max_word_len);
					return -1;
				}

				(words + j * max_word_len)[bytes++] = data;
			}
		} else {
			if (bytes > 0) {	/*the end of word */
				/*word string should be end of zero. */
				(words + j * max_word_len)[bytes] = 0;
				j++;
				bytes = 0;
			} else
				continue;
		}
	}

	/*no ';' at the end of the line */
	if (bytes > 0) {
		/*word string should be end of zero. */
		(words + j * max_word_len)[bytes] = 0;
		return j + 1;
	} else
		return j;
}

int is_digital(const char *string, int en_hex, int *is_hex)
{
	int len;
	int i;
	char c;

	len = (int)strlen(string);

	if (en_hex) {
		if (len > 2) {	/*hex data prefix with 0x or 0X */
			if ((string[0] == '0')
				&& ((string[1] == 'x') ||
				(string[1] == 'X'))) {
				/*it is an hex number */
				for (i = 2; i < len; i++) {
					c = string[i];

					if (((c >= '0') && (c <= '9'))
						|| ((c >= 'a') && (c <= 'f'))
						|| ((c >= 'A') && (c <= 'F')))
						continue;
					else
						return 0;
				}

				if (is_hex)
					*is_hex = 1;

				return 1;
			}
		}
	}

	/*it is an dec data */
	if (len >= 1) {
		for (i = 0; i < len; i++) {
			c = string[i];

			if ((c >= '0') && (c <= '9'))
				continue;
			else
				return 0;
		}

		if (is_hex)
			*is_hex = 0;

		return 1;
	} else
		return 0;
}

/*add following by bdu*/
int InitStringStream(SvmParserContext_t *context,
		SvmStringStream_t *stringStream, char *buf, int len)
{
	if ((!buf) || (!stringStream)) {
		SetErrorInfo(context, "script stream buf is NULL\n");
		return -1;
	}

	stringStream->buf = buf;
	stringStream->len = len;
	stringStream->readPos = 0;
	return 0;
}

int GetLine(SvmParserContext_t *context, SvmStringStream_t *string_stream,
		char *buf, int len)
{
	int bytes;
	char data;
	int loop = 1;

	if ((!string_stream) || (!buf)) {
		SetErrorInfo(context, "stream buf is NULL\n");
		return -1;
	}

	if (len < 1) {
		SetErrorInfo(context, "line buf len is 0\n");
		return -1;
	}

	bytes = 0;

	if (string_stream->readPos >= (string_stream->len))
		return END_OF_STREAM;

	while (loop) {
		if (string_stream->readPos >= string_stream->len) {
			/*the end of the stream */
			buf[bytes] = 0;
			break;
		}

		data = string_stream->buf[string_stream->readPos];
		string_stream->readPos++;

		//'\r' only exist on windows platform,
		//and it will always after with '\n'
		if (data == '\r') {
			continue;
		} else if (data == '\n') {
			buf[bytes] = 0;	/*string should be end of zero. */
			break;
		}
		buf[bytes++] = data;

		if (bytes >= (len - 1))
			break;

		continue;
	}

	return bytes;
}

int Split(SvmParserContext_t *context, char *line, int len, char *words,
		int maxWords, int maxWordLen)
{
	int i, j;
	int bytes;
	char data;

	j = 0;
	bytes = 0;
	for (i = 0; i < len; i++) {
		data = line[i];
		if ((data != ' ') && (data != '\t') && (data != ',')) {
			//The end of the instruction
			if (data == ';') {
				if (bytes > 0) {
					//Word string should be end of zero.
					(words + j * maxWordLen)[bytes] = 0;
					//Return the number of words
					return (j + 1);
				} else
					return j;
			} else {
				if (j >= maxWords) {
					SetErrorInfo(context,
					"Too many words in one line, max:%d\n",
						maxWords);
					return -1;
				}

				if (bytes >= maxWordLen) {
					SetErrorInfo(context,
						"Word len too long, max:%d\n",
						maxWordLen);
					return -1;
				}

				(words + j * maxWordLen)[bytes++] = data;
			}
		} else {
			//The end of word
			if (bytes > 0)	{
				//Word string should be end of zero.
				(words + j * maxWordLen)[bytes] = 0;
				j++;
				bytes = 0;
			} else
				continue;
		}
	}
	//No ';' at the end of the line
	if (bytes > 0) {
		//Word string should be end of zero.
		(words + j * maxWordLen)[bytes] = 0;
		return (j + 1);
	} else
		return j;
}

int IsDigital(const char *string, int enHex, int *isHex)
{
	int len;
	int i;
	char c;

	len = (int)strlen(string);

	if (enHex) {
		//Hex data prefix with 0x or 0X
		if (len > 2) {
			if ((string[0] == '0')
			&& ((string[1] == 'x') || (string[1] == 'X'))) {
				//It is an hex number
				for (i = 2; i < len; i++) {
					c = string[i];

					if (((c >= '0') && (c <= '9'))
						|| ((c >= 'a') && (c <= 'f'))
						|| ((c >= 'A') && (c <= 'F')))
						continue;
					else
						return 0;
				}
				if (isHex)
					*isHex = 1;

				return 1;
			}
		}
	}
	//It is an dec data
	if (len >= 1) {
		for (i = 0; i < len; i++) {
			c = string[i];

			if ((c >= '0') && (c <= '9'))
				continue;
			else
				return 0;
		}
		if (isHex)
			*isHex = 0;

		return 1;
	} else
		return 0;
}
