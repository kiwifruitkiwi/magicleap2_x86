/*
 * Copyright (C) 2019-2020 Advanced Micro Devices, Inc. All rights reserved.
 */

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
