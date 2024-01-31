/*
 * Copyright (C) 2020 Advanced Micro Devices, Inc. All rights reserved.
 *
 */
#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinmux.h>
#include <linux/pinctrl/pinconf-generic.h>

static const struct pinctrl_pin_desc cvip_pins[] = {
	PINCTRL_PIN(141, "GPIO_141"), /* uart0 rx */
	PINCTRL_PIN(143, "GPIO_143"), /* uart0 tx */
	PINCTRL_PIN(144, "GPIO_144"), /* uart0 int */
	PINCTRL_PIN(320, "GPIO_320"), /* general purpose */
	PINCTRL_PIN(321, "GPIO_321"), /* general purpose */
	PINCTRL_PIN(322, "GPIO_322"), /* general purpose */
	PINCTRL_PIN(323, "GPIO_323"), /* general purpose */
	PINCTRL_PIN(324, "GPIO_324"), /* general purpose */
	PINCTRL_PIN(325, "GPIO_325"), /* general purpose */
	PINCTRL_PIN(326, "GPIO_326"), /* general purpose */
	PINCTRL_PIN(327, "GPIO_327"), /* general purpose */
	PINCTRL_PIN(328, "GPIO_328"), /* pcie hotplug */
	PINCTRL_PIN(340, "GPIO_340"), /* i2c0 clk */
	PINCTRL_PIN(341, "GPIO_341"), /* i2c0 data */
	PINCTRL_PIN(342, "GPIO_342"), /* i2c1 clk */
	PINCTRL_PIN(343, "GPIO_343"), /* i2c1 data */
	PINCTRL_PIN(345, "GPIO_345"), /* spi clk */
	PINCTRL_PIN(346, "GPIO_346"), /* spi cs2 */
	PINCTRL_PIN(347, "GPIO_347"), /* spi cs3 */
	PINCTRL_PIN(348, "GPIO_348"), /* spi di */
	PINCTRL_PIN(349, "GPIO_349"), /* spi do */
	PINCTRL_PIN(350, "GPIO_350"), /* uart1 tx */
	PINCTRL_PIN(351, "GPIO_351"), /* uart1 rx */
};

#define CVIP_GPIO_PINS(pin) \
static const unsigned int gpio##pin##_pins[] = { pin }
CVIP_GPIO_PINS(141);
CVIP_GPIO_PINS(143);
CVIP_GPIO_PINS(144);
CVIP_GPIO_PINS(320);
CVIP_GPIO_PINS(321);
CVIP_GPIO_PINS(322);
CVIP_GPIO_PINS(323);
CVIP_GPIO_PINS(324);
CVIP_GPIO_PINS(325);
CVIP_GPIO_PINS(326);
CVIP_GPIO_PINS(327);
CVIP_GPIO_PINS(328);
CVIP_GPIO_PINS(340);
CVIP_GPIO_PINS(341);
CVIP_GPIO_PINS(342);
CVIP_GPIO_PINS(343);
CVIP_GPIO_PINS(345);
CVIP_GPIO_PINS(346);
CVIP_GPIO_PINS(347);
CVIP_GPIO_PINS(348);
CVIP_GPIO_PINS(349);
CVIP_GPIO_PINS(350);
CVIP_GPIO_PINS(351);

static const unsigned int cvip_range_pins[] = {
	320, 321, 322, 323, 324, 325, 326, 327, 328,
};

static const char * const cvip_range_pins_name[] = {
	"gpio320", "gpio321", "gpio322", "gpio323", "gpio324",
	"gpio325", "gpio326", "gpio327", "gpio328",
};

enum cvip_functions {
	mux_gpio,
	mux_uart0,
	mux_uart1,
	mux_i2c0,
	mux_i2c1,
	mux_spi,
	mux_NA
};

static const char * const gpio_groups[] = {
	"gpio141", "gpio143", "gpio144",
	"gpio320", "gpio321", "gpio322", "gpio323",
	"gpio324", "gpio325", "gpio326", "gpio327", "gpio328",
	"gpio340", "gpio341", "gpio342", "gpio343",
	"gpio345", "gpio346", "gpio347", "gpio348",
	"gpio349", "gpio350", "gpio351",
};

static const char * const uart0_groups[] = { "gpio141", "gpio143", "gpio144" };
static const char * const uart1_groups[] = { "gpio350", "gpio351" };
static const char * const i2c0_groups[] = { "gpio340", "gpio341" };
static const char * const i2c1_groups[] = { "gpio342", "gpio343" };
static const char * const spi_groups[] = { "gpio345", "gpio346", "gpio347",
					   "gpio348", "gpio349" };

/**
 * struct cvip_function - a pinmux function
 * @name:    Name of the pinmux function.
 * @groups:  List of pingroups for this function.
 * @ngroups: Number of entries in @groups.
 */
struct cvip_function {
	const char *name;
	const char * const *groups;
	unsigned int ngroups;
};

#define FUNCTION(fname)					\
	[mux_##fname] = {				\
		.name = #fname,				\
		.groups = fname##_groups,		\
		.ngroups = ARRAY_SIZE(fname##_groups),	\
	}

static const struct cvip_function cvip_functions[] = {
	FUNCTION(gpio),
	FUNCTION(uart0),
	FUNCTION(uart1),
	FUNCTION(i2c0),
	FUNCTION(i2c1),
	FUNCTION(spi),
};

/**
 * struct cvip_pingroup - a pinmux group
 * @name:  Name of the pinmux group.
 * @pins:  List of pins for this group.
 * @npins: Number of entries in @pins.
 * @funcs: List of functions belongs to this group.
 * @nfuncs: Number of entries in @funcs.
 * @offset: Group offset in cvip pinmux groups.
 * @cvip_reserved: Cvip flag.
 */
struct cvip_pingroup {
	const char *name;
	const unsigned int *pins;
	unsigned int npins;
	unsigned int *funcs;
	unsigned int nfuncs;
	unsigned int offset;
	bool cvip_reserved; /* cvip reserved pin */
};

#define CVIP_GPIO_BASE 320

#define PINGROUP(id, f0, f1, f2, f3, reserved)			\
	{							\
		.name = "gpio" #id,				\
		.pins = gpio##id##_pins,			\
		.npins = ARRAY_SIZE(gpio##id##_pins),		\
		.funcs = (int[]){				\
			mux_##f0,				\
			mux_##f1,				\
			mux_##f2,				\
			mux_##f3,				\
		},						\
		.nfuncs = 4,					\
		.offset = id,					\
		.cvip_reserved = reserved,			\
	}

static const struct cvip_pingroup cvip_groups[] = {
	PINGROUP(141, gpio, uart0, gpio, gpio, false),
	PINGROUP(143, gpio, uart0, gpio, gpio, false),
	PINGROUP(144, gpio, NA,    uart0, gpio, false),
	PINGROUP(350, gpio, uart0, uart1, NA, false),
	PINGROUP(351, gpio, uart0, uart1, NA, false),
	PINGROUP(320, gpio, NA, NA, NA, true),
	PINGROUP(321, gpio, NA, NA, NA, true),
	PINGROUP(322, gpio, NA, NA, NA, true),
	PINGROUP(323, gpio, NA, NA, NA, true),
	PINGROUP(324, gpio, NA, NA, NA, true),
	PINGROUP(325, gpio, NA, NA, NA, true),
	PINGROUP(326, gpio, NA, NA, NA, true),
	PINGROUP(327, gpio, NA, NA, NA, true),
	PINGROUP(328, gpio, NA, NA, NA, true),
	PINGROUP(340, i2c0, gpio, gpio, gpio, true),
	PINGROUP(341, i2c0, gpio, gpio, gpio, true),
	PINGROUP(342, i2c1, gpio, gpio, gpio, true),
	PINGROUP(343, i2c1, gpio, gpio, gpio, true),
	PINGROUP(345, spi, gpio, gpio, gpio, true),
	PINGROUP(346, spi, gpio, gpio, gpio, true),
	PINGROUP(347, spi, gpio, gpio, gpio, true),
	PINGROUP(348, spi, gpio, gpio, gpio, true),
	PINGROUP(349, spi, gpio, gpio, gpio, true),
};
