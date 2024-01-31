/**************************************************************************
 *copyright 2014~2015 advanced micro devices, inc.
 *
 *permission is hereby granted, free of charge, to any person obtaining a
 *copy of this software and associated documentation files (the "software"),
 *to deal in the software without restriction, including without limitation
 *the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *and/or sell copies of the software, and to permit persons to whom the
 *software is furnished to do so, subject to the following conditions:
 *
 *the above copyright notice and this permission notice shall be included in
 *all copies or substantial portions of the software.
 *
 *THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL
 *THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 *OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 *ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 *OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#ifndef DW_COMMON_ERRNO_H
#define DW_COMMON_ERRNO_H

#define DW_EPERM	1	/*operation not permitted */
#define DW_EIO		5	/*I/O error */
#define DW_ENXIO	6	/*no such device or address */
#define DW_ENOMEM	12	/*out of memory */
#define DW_EACCES	13	/*permission denied */
#define DW_EBUSY	16	/*device or resource busy */
#define DW_ENODEV	19	/*no such device */
#define DW_EINVAL	22	/*invalid argument */
#define DW_ENOSPC	28	/*no space left on device */
#define DW_ENOSYS	38	/*function not implemented/supported */
#define DW_ECHRNG	44	/*channel number out of range */
#define DW_ENODATA	61	/*no data available */
#define DW_ETIME	62	/*timer expired */
#define DW_EPROTO	71	/*protocol error */

#endif				/*DW_COMMON_ERRNO_H */
