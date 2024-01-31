// SPDX-License-Identifier: BSD-3-Clause-Clear
/**
 * Copyright (c) 2019 The Linux Foundation. All rights reserved.
 */

#include <linux/kernel.h>
#include <linux/string.h>
#include "diag_msg.h"

/**
 * Flags used during conversion.
 */
#define ALT         0x001	/* alternate form */
#define LADJUST     0x004	/* left adjustment */
#define LONGDBL     0x008	/* long double */
#define LONGINT     0x010	/* long integer */
#define LLONGINT    0x020	/* long long integer */
#define SHORTINT    0x040	/* short integer */
#define ZEROPAD     0x080	/* zero (as opposed to blank) pad */
#define FPT         0x100	/* Floating point number */
#define GROUPING    0x200	/* use grouping ("'" flag) */
/*C99 additional size modifiers: */
#define SIZET       0x400	/* size_t */
#define PTRDIFFT    0x800	/* ptrdiff_t */
#define INTMAXT     0x1000	/* intmax_t */
#define CHARINT     0x2000	/* print char using int format */

/**
 * Macros for converting digits to letters and vice versa
 */
#define to_digit(c)	((c) - '0')
#define is_digit(c)	((unsigned int)to_digit(c) <= 9)

#define _GET_LE32(a) ( \
	(((uint32_t)(a)[3]) << 24) | \
	(((uint32_t)(a)[2]) << 16) | \
	(((uint32_t)(a)[1]) << 8)  | \
	((uint32_t)(a)[0]))

#define GET_LE32(v, msg, msg_len) do {      \
	if ((msg_len) < sizeof(uint32_t)) {   \
		goto msg_error;             \
	}                                   \
	v = _GET_LE32(msg);                 \
	msg += sizeof(uint32_t);            \
	(msg_len) -= sizeof(uint32_t);        \
} while (0)

#define to_char(n)	((n) + '0')

/* Return successive characters in a format string. */
static char fmt_next_char(const char **fmtptr)
{
	char ch;

	ch = **fmtptr;
	if (ch != '\0')
		(*fmtptr)++;

	return ch;
}

/* Return current characters in a format string. */
static char fmt_cur_char(const char **fmtptr)
{
	char ch;

	ch = **fmtptr;
	return ch;
}

static long long get_value_from_msg(char pack, uint8_t **msg, uint32_t *msg_len)
{
	s32 val = 0;

	GET_LE32(val, *msg, *msg_len);

	return val;

msg_error:
	pr_err("Fatal error to fetch param\n");
	return 0;
}

/**
 * Convert an unsigned long to ASCII for printf purposes, returning
 * a pointer to the first character of the string representation.
 * Octal numbers can be forced to have a leading zero; hex numbers
 * use the given digits.
 */
static char *
__ultoa(unsigned long long val, char *endp, int base, int octzero,
	const char *xdigs)
{
	char *cp = endp;

	if (!endp || !xdigs) {
		pr_err("%s: Input parameter invalid", __func__);
		return endp;
	}

	/**
	 * Handle the three cases separately, in the hope of getting
	 * better/faster code.
	 */
	switch (base) {
	case 10:
		if (val < 10) {	/* many numbers are 1 digit */
			*--cp = to_char(val);
			return cp;
		}
		do {
			*--cp = to_char(val % 10);
			val /= 10;
		} while (val != 0);
		break;

	case 8:
		do {
			*--cp = to_char(val & 7);
			val >>= 3;
		} while (val);
		if (octzero && *cp != '0')
			*--cp = '0';
		break;

	case 16:
		do {
			*--cp = xdigs[val & 15];
			val >>= 4;
		} while (val);
		break;

	case 2:
		do {
			*--cp = to_char(val & 1);
			val >>= 1;
		} while (val);
		break;
	default:			/* oops */
		break;
	}
	return cp;
}

/**
 * The size of the buffer we use as scratch space for integer
 * conversions, among other things.  We need enough space to
 * write a uintmax_t in octal (plus one byte).
 */
#define	BUF	(sizeof(long long) * 8)

static int pack_printf(void (*write_char)(char **pbs, char *be, char c),
		       char **pbuf_start,
		       char *buf_end,
		       const char *fmt0,
		       const char *pack,
		       u8 *msg,
		       u32 msg_len)
{
	const char *fmt;     /* format string */
	int ch;              /* character from fmt */
	int n;               /* handy integer (short term usage) */
	char *cp;            /* handy char pointer (short term usage) */
	int flags;           /* flags as above */
	int ret;             /* return value accumulator */
	int width;           /* width from format (%8d), or 0 */
	int prec;            /* precision from format; <0 for N/A */
	char sign;           /* sign prefix (' ', '+', '-', or \0) */

	long long ulval;     /* integer arguments %[diouxX] */
	int base;            /* base for [diouxX] conversion */
	int dprec;           /* a copy of prec if [diouxX], 0 otherwise */
	int realsz;          /* field size expanded by dprec, sign, etc */
	int size;            /* size of converted field or string */
	int prsize;          /* max size of printed field */
	const char *xdigs;   /* digits for %[xX] conversion */
	char buf[BUF];       /* buffer with space for digits of uintmax_t */
	char ox[2];          /* space for 0x; ox[1] is either x, X, or \0 */
	int pad;             /* pad */

	static const char xdigs_lower[] = "0123456789abcdef";
	static const char xdigs_upper[] = "0123456789ABCDEF";

	fmt = (char *)fmt0;
	ret = 0;
	size = 0;
	xdigs = xdigs_lower;

	/**
	 * Scan the format for conversions (`%' character).
	 */
	for (;;) {
		for (cp = (char *)fmt;
		     (ch = fmt_cur_char(&fmt)) != '\0' && ch != '%';
		     ch = fmt_next_char(&fmt)) {
			write_char(pbuf_start, buf_end, ch);
			ret++;
		}

		if (ch == '\0')
			goto done;
		fmt_next_char(&fmt);		/* skip over '%' */

		flags = 0;
		dprec = 0;
		width = 0;
		prec = -1;
		sign = '\0';
		ox[1] = '\0';

rflag:		ch = fmt_next_char(&fmt);
reswitch:
		switch (ch) {
		case ' ':
			/*-
			 * ``If the space and + flags both appear, the space
			 * flag will be ignored.''
			 *	-- ANSI X3J11
			 */
			if (!sign)
				sign = ' ';
			goto rflag;
		case '-':
			flags |= LADJUST;
			goto rflag;
		case '+':
			sign = '+';
			goto rflag;
		case '.':
			ch = fmt_next_char(&fmt);
			prec = 0;
			while (is_digit(ch)) {
				prec = 10 * prec + to_digit(ch);
				ch = fmt_next_char(&fmt);
			}
			goto reswitch;
		case '0':
			/*-
			 * ``Note that 0 is taken as a flag, not as the
			 * beginning of a field width.''
			 *	-- ANSI X3J11
			 */
			flags |= ZEROPAD;
			goto rflag;
		case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			n = 0;
			do {
				n = 10 * n + to_digit(ch);
				ch = fmt_next_char(&fmt);
			} while (is_digit(ch));
			width = n;
			goto reswitch;
		case 'l':
			if (flags & LONGINT) {
				flags &= ~LONGINT;
				flags |= LLONGINT;
			} else {
				flags |= LONGINT;
			}
			goto rflag;
		case 'C':
			flags |= LONGINT; /* Do not support WCHAR */
			/*FALLTHROUGH*/
		case 'c':
			if (msg_len) {
				cp = (char *)msg++;
				msg_len--;
				size = 1;
				sign = '\0';
			}
			break;
		case 'D':
			flags |= LONGINT;
			/*FALLTHROUGH*/
		case 'd':
		case 'i':
			ulval = get_value_from_msg(fmt_next_char(&pack),
						   &msg,
						   &msg_len);
			if (ulval < 0) {
				ulval = -ulval;
				sign = '-';
			}
			base = 10;
			goto number;
		case 'p':
			/*-
			 * ``The argument shall be a pointer to void.  The
			 * value of the pointer is converted to a sequence
			 * of printable characters, in an implementation-
			 * defined manner.''
			 *	-- ANSI X3J11
			 */
			ulval = (int)get_value_from_msg(fmt_next_char(&pack),
							&msg,
							&msg_len);
			base = 16;
			xdigs = xdigs_lower;
			ox[1] = 'x';
			goto nosign;
		case 'S':
		case 's':
			/* Not Supported */
			cp = "<null>";
			size = 0;
			while (cp[size] != '\0')
				size++;
			sign = '\0';
			break;
		case 'O':
			flags |= LONGINT;
			/*FALLTHROUGH*/
		case 'o':
			ulval = get_value_from_msg(fmt_next_char(&pack),
						   &msg,
						   &msg_len);
			base = 8;
			goto nosign;
		case 'B':
			flags |= LONGINT;
			/*FALLTHROUGH*/
		case 'b':
			ulval = get_value_from_msg(fmt_next_char(&pack),
						   &msg,
						   &msg_len);
			base = 2;
			goto nosign;
		case 'U':
			flags |= LONGINT;
			/*FALLTHROUGH*/
		case 'u':
			ulval = get_value_from_msg(fmt_next_char(&pack),
						   &msg,
						   &msg_len);
			base = 10;
			goto nosign;
		case 'X':
			xdigs = xdigs_upper;
			goto hex;
		case 'x':
			xdigs = xdigs_lower;
hex:
			ulval = get_value_from_msg(fmt_next_char(&pack),
						   &msg,
						   &msg_len);
			base = 16;
			/* leading 0x/X only if non-zero */
			if (flags & ALT && ulval != 0)
				ox[1] = ch;

			/* unsigned conversions */
nosign:
			sign = '\0';
			/*-
			 * ``... diouXx conversions ... if a precision is
			 * specified, the 0 flag will be ignored.''
			 *	-- ANSI X3J11
			 */
number:
			dprec = prec;
			if (dprec >= 0)
				flags &= ~ZEROPAD;

			/*-
			 * ``The result of converting a zero value with an
			 * explicit precision of zero is no characters.''
			 *	-- ANSI X3J11
			 *
			 * ``The C Standard is clear enough as is.  The call
			 * printf("%#.0o", 0) should print 0.''
			 *	-- Defect Report #151
			 */
			cp = buf + BUF;
			if (ulval != 0 || prec != 0 ||
			    (flags & ALT && base == 8)) {
				cp = __ultoa(ulval, cp, base,
					     flags & ALT, xdigs);
			}
			size = buf + BUF - cp;

			break;
		default:	/* "%?" prints ?, unless ? is NUL */
			if (ch == '\0')
				goto done;
			/* pretend it was %c with argument ch */
			cp = buf;
			*cp = ch;
			size = 1;
			sign = '\0';
			break;
		}

		/**
		 * All reasonable formats wind up here.  At this point, `cp'
		 * points to a string which (if not flags&LADJUST) should be
		 * padded out to `width' places.  If flags&ZEROPAD, it should
		 * first be prefixed by any sign or other prefix; otherwise,
		 * it should be blank padded before the prefix is emitted.
		 * After any left-hand padding and prefixing, emit zeroes
		 * required by a decimal [diouxX] precision, then print the
		 * string proper, then emit zeroes required by any leftover
		 * floating precision; finally, if LADJUST, pad with blanks.
		 *
		 * Compute actual size, so we know how much to pad.
		 * size excludes decimal prec; realsz includes it.
		 */
		realsz = dprec > size ? dprec : size;
		if (sign)
			realsz++;
		if (ox[1])
			realsz += 2;

		prsize = width > realsz ? width : realsz;
		pad =  width - realsz;

		/* right-adjusting blank padding */
		if ((flags & (LADJUST | ZEROPAD)) == 0) {
			while (pad-- > 0)
				write_char(pbuf_start, buf_end, ' ');
		}

		/* prefix */
		if (sign) {
			write_char(pbuf_start, buf_end, sign);
			pad--;
		}

		if (ox[1]) {	/* ox[1] is either x, X, or \0 */
			ox[0] = '0';
			write_char(pbuf_start, buf_end, ox[0]);
			pad--;
			write_char(pbuf_start, buf_end, ox[1]);
			pad--;
		}

		/* right-adjusting zero padding */
		if ((flags & (LADJUST | ZEROPAD)) == ZEROPAD) {
			while (pad-- > 0)
				write_char(pbuf_start, buf_end, '0');
		}

		while (size-- > 0) {
			ch = *cp++;
			(*write_char)(pbuf_start, buf_end, ch);
		}

		/* left-adjusting padding (always blank) */
		if (flags & LADJUST) {
			while (pad-- > 0)
				write_char(pbuf_start, buf_end, ' ');
		}

		/* finally, adjust ret */
		ret += prsize;
	}
done:

	return (ret);
	/* NOTREACHED */
}

/* user  supply their own function to build string in temporary
 * buffer
 */
static void dbg_write_char(char **pbuf_start, char *buf_end, char c)
{
	if (*pbuf_start < buf_end) {
		*(*pbuf_start) = c;
		++(*pbuf_start);
	}
}

void diag_local_ext_msg_report_handler(u8 *buf, int len)
{
	char msgBuf[DIAG_PRINT_MAX_SIZE] = {0}, *start;
	u8 cmd;
	struct msg_ext_type *msg = NULL;
	char *fmt_text = NULL;

	if (!buf || len <= 0) {
		pr_err("%s: Invalid parameter\n", __func__);
		return;
	}

	cmd = buf[DIAG_MSG_CMD_CODE_OFFSET];

	if (cmd != DIAG_CMD_EXT_MSG) {
		pr_err("%s: Invalid cmd_code %d\n", __func__, cmd);
		return;
	}

	msg = (struct msg_ext_type *)buf;
	fmt_text = (char *)msg->args + msg->hdr.num_args * sizeof(u_int32_t);

	if (msg->hdr.num_args != 0) {
		memset(msgBuf, 0, DIAG_PRINT_MAX_SIZE);
		start = msgBuf;
		pack_printf(dbg_write_char,
			    &start,
			    start + sizeof(msgBuf),
			    fmt_text,
			    fmt_text,
			    (u_int8_t *)&msg->args[0],
			    msg->hdr.num_args * sizeof(msg->args[0]));
	}
}

