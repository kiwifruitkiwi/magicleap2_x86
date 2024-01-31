/*
 * Copyright (C) 2019-2021 Advanced Micro Devices, Inc. All rights reserved.
 */

#include "sensor_if.h"
#include "fsl_error.h"
#include "fsl_string.h"

int InitStringStream(struct _SvmParserContext_t *context,
		struct _SvmStringStream_t *stringStream, char *buf, int len)
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

int GetLine(struct _SvmParserContext_t *context,
		struct _SvmStringStream_t *string_stream,
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

int Split(struct _SvmParserContext_t *context, char *line, int len,
		char *words, int maxWords, int maxWordLen)
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
