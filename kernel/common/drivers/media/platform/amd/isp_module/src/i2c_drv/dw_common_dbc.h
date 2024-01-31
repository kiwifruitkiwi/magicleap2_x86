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
#ifndef DW_COMMON_DBC_H
#define DW_COMMON_DBC_H

#define DW_NASSERT	1

/*DW_NASSERT macro disables all contract validations*/
/*(assertions, preconditions, postconditions, and invariants).*/
#ifndef DW_NASSERT

/*callback invoked in case of assertion failure*/
extern void on_assert__(char const *file, unsigned int line);

#define DW_DEFINE_THIS_FILE static const char THIS_FILE__[] = __FILE__

#define DW_ASSERT(test_) (\
		if (test_)	\
				\
		else \
			on_assert__(THIS_FILE__, __LINE__))

#define DW_REQUIRE(test_)		DW_ASSERT(test_)
#define DW_ENSURE(test_)		DW_ASSERT(test_)
#define DW_INVARIANT(test_)		DW_ASSERT(test_)
#define DW_ALLEGE(test_)		DW_ASSERT(test_)

#else				/*DW_NASSERT */

#define DW_DEFINE_THIS_FILE extern const char THIS_FILE__[]
#define DW_ASSERT(test_)
#define DW_REQUIRE(test_)
#define DW_ENSURE(test_)
#define DW_INVARIANT(test_)
#define DW_ALLEGE(test_) (\
	if (test_)	\
			\
	else		\
		)

#endif				/*DW_NASSERT */

#endif				/*DW_COMMON_DBC_H */
