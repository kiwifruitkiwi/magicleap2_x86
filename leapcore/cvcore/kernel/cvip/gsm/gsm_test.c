#include <kunit/test.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include "gsm.h"
#include "gsm_jrtc.h"
#include "gsm_cvip.h"
#include "gsm_spinlock.h"

#define SECOND_TEST

static void gsm_test_is_up(struct kunit *test)
{
	KUNIT_EXPECT_EQ(test, 1, gsm_core_is_cvip_up());
}

#ifdef SECOND_TEST
static void gsm_test_is_down(struct kunit *test)
{
	KUNIT_EXPECT_EQ(test, 0, gsm_core_is_cvip_up());
}

static void gsm_test_is_middle(struct kunit *test)
{
	KUNIT_EXPECT_EQ(test, 0, gsm_core_is_cvip_up());
}
#endif

static struct kunit_case gsm_test_cases[] = {
#ifdef SECOND_TEST
	KUNIT_CASE(gsm_test_is_down),
	KUNIT_CASE(gsm_test_is_middle),
#endif
	KUNIT_CASE(gsm_test_is_up),
	{}
};

static struct kunit_suite gsm_test_suite = {
	.name = "gsm_test",
	.test_cases = gsm_test_cases,
};
kunit_test_suite(gsm_test_suite);

MODULE_LICENSE("GPL");
