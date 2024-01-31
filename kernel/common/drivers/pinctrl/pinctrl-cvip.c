/*
 * Copyright (C) 2020 Advanced Micro Devices, Inc. All rights reserved.
 *
 */
#include <linux/module.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/gpio/driver.h>
#include <linux/pinctrl/machine.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>

#include "core.h"
#include "pinctrl-utils.h"
#include "pinctrl-amd.h"
#include "pinctrl-cvip.h"

struct cvip_pinctrl_data {
	const struct pinctrl_pin_desc *pins;
	unsigned int npins;
	const struct cvip_function *functions;
	unsigned int nfunctions;
	const struct cvip_pingroup *groups;
	unsigned int ngroups;
};

static const struct cvip_pinctrl_data cvip_pinctrl_data = {
	.pins = cvip_pins,
	.npins = ARRAY_SIZE(cvip_pins),
	.functions = cvip_functions,
	.nfunctions = ARRAY_SIZE(cvip_functions),
	.groups = cvip_groups,
	.ngroups = ARRAY_SIZE(cvip_groups),
};

struct cvip_pinctrl {
	struct device *dev;
	struct pinctrl_dev *pctrl;
	struct pinctrl_desc desc;
	struct pinctrl_gpio_range gpio_range;
	struct gpio_chip gc;
	const struct cvip_pinctrl_data *data;
	void __iomem *mux_regs;
	void __iomem *cvip_mux_regs;
	void __iomem *gpiobase;
	raw_spinlock_t lock;
};

static int cvip_get_groups_count(struct pinctrl_dev *pctldev)
{
	struct cvip_pinctrl *pctrl = pinctrl_dev_get_drvdata(pctldev);

	return pctrl->data->ngroups;
}

static const char *cvip_get_group_name(struct pinctrl_dev *pctldev,
				       unsigned int group)
{
	struct cvip_pinctrl *pctrl = pinctrl_dev_get_drvdata(pctldev);

	return pctrl->data->groups[group].name;
}

static int cvip_get_group_pins(struct pinctrl_dev *pctldev,
			       unsigned int group,
			       const unsigned int **pins,
			       unsigned int *num_pins)
{
	struct cvip_pinctrl *pctrl = pinctrl_dev_get_drvdata(pctldev);

	*pins = pctrl->data->groups[group].pins;
	*num_pins = pctrl->data->groups[group].npins;
	return 0;
}

const struct pinctrl_ops cvip_pinctrl_ops = {
	.get_groups_count	= cvip_get_groups_count,
	.get_group_name		= cvip_get_group_name,
	.get_group_pins		= cvip_get_group_pins,
	.dt_node_to_map		= pinconf_generic_dt_node_to_map_group,
	.dt_free_map		= pinctrl_utils_free_map,
};

static int cvip_pinmux_request(struct pinctrl_dev *pctldev, unsigned int offset)
{
	int i;
	struct cvip_pinctrl *pctrl = pinctrl_dev_get_drvdata(pctldev);

	for (i = 0; i < pctrl->data->npins; i++) {
		if (offset == pctrl->data->pins[i].number)
			return 0;
	};
	return -EINVAL;
}

static int cvip_get_functions_count(struct pinctrl_dev *pctldev)
{
	struct cvip_pinctrl *pctrl = pinctrl_dev_get_drvdata(pctldev);

	return pctrl->data->nfunctions;
}

static const char *cvip_get_function_name(struct pinctrl_dev *pctldev,
					  unsigned int function)
{
	struct cvip_pinctrl *pctrl = pinctrl_dev_get_drvdata(pctldev);

	return pctrl->data->functions[function].name;
}

static int cvip_get_function_groups(struct pinctrl_dev *pctldev,
				    unsigned int function,
				    const char * const **groups,
				    unsigned int * const num_groups)
{
	struct cvip_pinctrl *pctrl = pinctrl_dev_get_drvdata(pctldev);

	*groups = pctrl->data->functions[function].groups;
	*num_groups = pctrl->data->functions[function].ngroups;
	return 0;
}

static int cvip_pinmux_set_mux(struct pinctrl_dev *pctldev,
			       unsigned int function,
			       unsigned int group)
{
	u8 func_id;
	void __iomem *mux_regs;
	unsigned int offset;
	const struct cvip_pingroup *pingroup;
	unsigned long flags;
	struct cvip_pinctrl *pctrl = pinctrl_dev_get_drvdata(pctldev);

	pingroup = &pctrl->data->groups[group];
	mux_regs = pctrl->mux_regs;
	offset = pingroup->offset;
	if (pingroup->cvip_reserved) {
		mux_regs = pctrl->cvip_mux_regs;
		offset -= CVIP_GPIO_BASE;
	}

	for (func_id = 0; func_id < pingroup->nfuncs; func_id++)
		if (pingroup->funcs[func_id] == function)
			break;
	if (func_id >= pingroup->nfuncs) {
		dev_err(pctldev->dev, "Error: invalid function\n");
		return -EINVAL;
	}

	raw_spin_lock_irqsave(&pctrl->lock, flags);
	writeb_relaxed(func_id, mux_regs + offset);
	raw_spin_unlock_irqrestore(&pctrl->lock, flags);
	return 0;
}

const struct pinmux_ops cvip_pinmux_ops = {
	.request		= cvip_pinmux_request,
	.get_functions_count	= cvip_get_functions_count,
	.get_function_name	= cvip_get_function_name,
	.get_function_groups	= cvip_get_function_groups,
	.set_mux		= cvip_pinmux_set_mux,
};

#ifdef CONFIG_GPIOLIB
static int cvip_gpio_get_direction(struct gpio_chip *gc, unsigned int gpio)
{
	unsigned long flags;
	u32 pin_reg;
	struct cvip_pinctrl *pctrl = gpiochip_get_data(gc);

	pr_debug("%s: gpio%d\n", __func__, gpio);
	if (gpio >= CVIP_GPIO_BASE)
		gpio -= CVIP_GPIO_BASE;

	raw_spin_lock_irqsave(&pctrl->lock, flags);
	pin_reg = readl(pctrl->gpiobase + gpio * 4);
	raw_spin_unlock_irqrestore(&pctrl->lock, flags);

	return !(pin_reg & BIT(OUTPUT_ENABLE_OFF));
}

static int cvip_gpio_direction_input(struct gpio_chip *gc, unsigned int gpio)
{
	unsigned long flags;
	u32 pin_reg;
	struct cvip_pinctrl *pctrl = gpiochip_get_data(gc);

	pr_debug("%s: gpio%d\n", __func__, gpio);
	if (gpio >= CVIP_GPIO_BASE)
		gpio -= CVIP_GPIO_BASE;

	raw_spin_lock_irqsave(&pctrl->lock, flags);
	pin_reg = readl(pctrl->gpiobase + gpio * 4);
	pin_reg &= ~BIT(OUTPUT_ENABLE_OFF);
	writel(pin_reg, pctrl->gpiobase + gpio * 4);
	raw_spin_unlock_irqrestore(&pctrl->lock, flags);

	return 0;
}

static int cvip_gpio_direction_output(struct gpio_chip *gc, unsigned int gpio,
				      int value)
{
	unsigned long flags;
	u32 pin_reg;
	struct cvip_pinctrl *pctrl = gpiochip_get_data(gc);

	pr_debug("%s: gpio%d, value=%d\n", __func__, gpio, value);
	if (gpio >= CVIP_GPIO_BASE)
		gpio -= CVIP_GPIO_BASE;

	raw_spin_lock_irqsave(&pctrl->lock, flags);
	pin_reg = readl(pctrl->gpiobase + gpio * 4);
	pin_reg |= BIT(OUTPUT_ENABLE_OFF);
	if (value)
		pin_reg |= BIT(OUTPUT_VALUE_OFF);
	else
		pin_reg &= ~BIT(OUTPUT_VALUE_OFF);
	writel(pin_reg, pctrl->gpiobase + gpio * 4);
	raw_spin_unlock_irqrestore(&pctrl->lock, flags);

	return 0;
}

static int cvip_gpio_get(struct gpio_chip *gc, unsigned int gpio)
{
	unsigned long flags;
	u32 pin_reg;
	struct cvip_pinctrl *pctrl = gpiochip_get_data(gc);

	pr_debug("%s: gpio%d\n", __func__, gpio);
	if (gpio >= CVIP_GPIO_BASE)
		gpio -= CVIP_GPIO_BASE;

	raw_spin_lock_irqsave(&pctrl->lock, flags);
	pin_reg = readl(pctrl->gpiobase + gpio * 4);
	raw_spin_unlock_irqrestore(&pctrl->lock, flags);

	return !!(pin_reg & BIT(PIN_STS_OFF));
}

static void cvip_gpio_set(struct gpio_chip *gc, unsigned int gpio, int value)
{
	unsigned long flags;
	u32 pin_reg;
	struct cvip_pinctrl *pctrl = gpiochip_get_data(gc);

	pr_debug("%s: gpio%d, value=%d\n", __func__, gpio, value);
	if (gpio >= CVIP_GPIO_BASE)
		gpio -= CVIP_GPIO_BASE;

	raw_spin_lock_irqsave(&pctrl->lock, flags);
	pin_reg = readl(pctrl->gpiobase + gpio * 4);
	if (value)
		pin_reg |= BIT(OUTPUT_VALUE_OFF);
	else
		pin_reg &= ~BIT(OUTPUT_VALUE_OFF);
	writel(pin_reg, pctrl->gpiobase + gpio * 4);
	raw_spin_unlock_irqrestore(&pctrl->lock, flags);
}

static int cvip_gpio_set_debounce(struct gpio_chip *gc, unsigned int gpio,
				  unsigned int debounce)
{
	int ret = 0;
	u32 time, pin_reg;
	unsigned long flags;
	struct cvip_pinctrl *pctrl = gpiochip_get_data(gc);

	pr_debug("%s: gpio%d, value=%d\n", __func__, gpio, debounce);
	if (gpio >= CVIP_GPIO_BASE)
		gpio -= CVIP_GPIO_BASE;

	raw_spin_lock_irqsave(&pctrl->lock, flags);
	pin_reg = readl(pctrl->gpiobase + gpio * 4);

	if (debounce) {
		pin_reg |= DB_TYPE_REMOVE_GLITCH << DB_CNTRL_OFF;
		pin_reg &= ~DB_TMR_OUT_MASK;

		/* Debounce	Debounce	Timer	Max Debounce
		 * TmrLarge	TmrOutUnit	Unit	Time
		 * 0	0	61 usec (2 RtcClk)	976 usec
		 * 0	1	244 usec (8 RtcClk)	3.9 msec
		 * 1	0	15.6 msec (512 RtcClk)	250 msec
		 * 1	1	62.5 msec (2048 RtcClk)	1 sec
		 */

		if (debounce < 61) {
			pin_reg |= 1;
			pin_reg &= ~BIT(DB_TMR_OUT_UNIT_OFF);
			pin_reg &= ~BIT(DB_TMR_LARGE_OFF);
		} else if (debounce < 976) {
			time = debounce / 61;
			pin_reg |= time & DB_TMR_OUT_MASK;
			pin_reg &= ~BIT(DB_TMR_OUT_UNIT_OFF);
			pin_reg &= ~BIT(DB_TMR_LARGE_OFF);
		} else if (debounce < 3900) {
			time = debounce / 244;
			pin_reg |= time & DB_TMR_OUT_MASK;
			pin_reg |= BIT(DB_TMR_OUT_UNIT_OFF);
			pin_reg &= ~BIT(DB_TMR_LARGE_OFF);
		} else if (debounce < 250000) {
			time = debounce / 15600;
			pin_reg |= time & DB_TMR_OUT_MASK;
			pin_reg &= ~BIT(DB_TMR_OUT_UNIT_OFF);
			pin_reg |= BIT(DB_TMR_LARGE_OFF);
		} else if (debounce < 1000000) {
			time = debounce / 62500;
			pin_reg |= time & DB_TMR_OUT_MASK;
			pin_reg |= BIT(DB_TMR_OUT_UNIT_OFF);
			pin_reg |= BIT(DB_TMR_LARGE_OFF);
		} else {
			pin_reg &= ~DB_CNTRL_MASK;
			ret = -EINVAL;
		}
	} else {
		pin_reg &= ~BIT(DB_TMR_OUT_UNIT_OFF);
		pin_reg &= ~BIT(DB_TMR_LARGE_OFF);
		pin_reg &= ~DB_TMR_OUT_MASK;
		pin_reg &= ~DB_CNTRL_MASK;
	}
	writel(pin_reg, pctrl->gpiobase + gpio * 4);
	raw_spin_unlock_irqrestore(&pctrl->lock, flags);

	return ret;
}

static int cvip_gpio_set_config(struct gpio_chip *gc, unsigned int gpio,
				unsigned long config)
{
	u32 debounce;

	if (pinconf_to_config_param(config) != PIN_CONFIG_INPUT_DEBOUNCE)
		return -ENOTSUPP;

	debounce = pinconf_to_config_argument(config);
	return cvip_gpio_set_debounce(gc, gpio, debounce);
}

static int cvip_gpiochip_add(struct platform_device *pdev,
			     struct cvip_pinctrl *pctrl)
{
	struct gpio_chip *gc = &pctrl->gc;
	struct pinctrl_gpio_range *grange = &pctrl->gpio_range;
	int ret;

	gc->label = devm_kasprintf(&pdev->dev, GFP_KERNEL, "%pOFn",
				   pdev->dev.of_node);
	gc->owner		= THIS_MODULE;
	gc->parent		= &pdev->dev;
	gc->names		= cvip_range_pins_name;
	gc->request		= gpiochip_generic_request;
	gc->free		= gpiochip_generic_free;
	gc->get_direction	= cvip_gpio_get_direction;
	gc->direction_input	= cvip_gpio_direction_input;
	gc->direction_output	= cvip_gpio_direction_output;
	gc->get			= cvip_gpio_get;
	gc->set			= cvip_gpio_set;
	gc->set_config		= cvip_gpio_set_config;
	gc->base		= 0;
	gc->ngpio		= ARRAY_SIZE(cvip_range_pins);
#if defined(CONFIG_OF_GPIO)
	gc->of_node		= pdev->dev.of_node;
	gc->of_gpio_n_cells	= 2;
#endif

	grange->id = 0;
	grange->pin_base = 0;
	grange->base = 0;
	grange->pins = cvip_range_pins;
	grange->npins = ARRAY_SIZE(cvip_range_pins),
	grange->name = gc->label;
	grange->gc = gc;

	ret = devm_gpiochip_add_data(&pdev->dev, gc, pctrl);
	if (ret) {
		dev_err(&pdev->dev, "%s: Failed adding gpiochip\n", pdev->name);
		return ret;
	}

	pinctrl_add_gpio_range(pctrl->pctrl, grange);

	dev_info(&pdev->dev, "register gpio controller\n");
	return 0;
}
#endif

static const struct of_device_id cvip_pinctrl_of_match[] = {
	{ .compatible = "amd,cvip-pinctrl", },
	{ },
};

static int cvip_pinctrl_probe(struct platform_device *pdev)
{
	struct cvip_pinctrl *pctrl;
	struct resource *res;
	int ret;

	pctrl = devm_kzalloc(&pdev->dev, sizeof(*pctrl), GFP_KERNEL);
	if (!pctrl)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "failed to get mux registers base\n");
		return -EINVAL;
	}
	pctrl->mux_regs = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(pctrl->mux_regs)) {
		dev_err(&pdev->dev, "failed to map mux registers\n");
		return PTR_ERR(pctrl->mux_regs);
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (!res) {
		dev_err(&pdev->dev, "failed to get cvip mux registers base\n");
		return -EINVAL;
	}
	pctrl->cvip_mux_regs = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(pctrl->cvip_mux_regs)) {
		dev_err(&pdev->dev, "failed to map cvip mux registers\n");
		return PTR_ERR(pctrl->cvip_mux_regs);
	}

#ifdef CONFIG_GPIOLIB
	res = platform_get_resource(pdev, IORESOURCE_MEM, 2);
	if (!res) {
		dev_err(&pdev->dev, "failed to get cvip gpio registers base\n");
		return -EINVAL;
	}
	pctrl->gpiobase = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(pctrl->gpiobase)) {
		dev_err(&pdev->dev, "failed to map cvip gpio registers\n");
		return PTR_ERR(pctrl->gpiobase);
	}
#endif
	platform_set_drvdata(pdev, pctrl);

	pctrl->dev = &pdev->dev;
	pctrl->data = &cvip_pinctrl_data;
	pctrl->desc.owner = THIS_MODULE;
	pctrl->desc.pctlops = &cvip_pinctrl_ops;
	pctrl->desc.pmxops = &cvip_pinmux_ops;
	pctrl->desc.name = dev_name(&pdev->dev);
	pctrl->desc.pins = pctrl->data->pins;
	pctrl->desc.npins = pctrl->data->npins;
	ret = devm_pinctrl_register_and_init(&pdev->dev, &pctrl->desc,
					     pctrl, &pctrl->pctrl);
	if (ret) {
		dev_err(&pdev->dev, "couldn't register pinctrl driver\n");
		return ret;
	}

	ret = pinctrl_enable(pctrl->pctrl);
	if (ret) {
		dev_err(&pdev->dev, "pinctrl enable failed\n");
		return ret;
	}
#ifdef CONFIG_GPIOLIB
	ret = cvip_gpiochip_add(pdev, pctrl);
	if (ret)
		return ret;
#endif
	dev_info(&pdev->dev, "pinctrl init successful\n");
	return 0;
}

int cvip_pinctrl_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver cvip_pinctrl_driver = {
	.driver = {
		.name = "cvip-pinctrl",
		.of_match_table = cvip_pinctrl_of_match,
	},
	.probe = cvip_pinctrl_probe,
	.remove = cvip_pinctrl_remove,
};

static int __init cvip_pinctrl_init(void)
{
	return platform_driver_register(&cvip_pinctrl_driver);
}
arch_initcall(cvip_pinctrl_init);

static void __exit cvip_pinctrl_exit(void)
{
	platform_driver_unregister(&cvip_pinctrl_driver);
}
module_exit(cvip_pinctrl_exit);
MODULE_DESCRIPTION("cvip pinctrl driver");
