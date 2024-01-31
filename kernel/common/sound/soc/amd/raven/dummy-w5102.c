/*
 * I2S Dummy codec driver
 * Copyright (C) 2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#include <linux/module.h>
#include <sound/soc.h>

#define W5102_RATES	SNDRV_PCM_RATE_8000_96000
#define W5102_FORMATS	(SNDRV_PCM_FMTBIT_S16_LE | \
			SNDRV_PCM_FMTBIT_S32_LE | \
			SNDRV_PCM_FMTBIT_S24_LE)

static const struct snd_soc_dapm_widget w5102_widgets[] = {
	SND_SOC_DAPM_OUTPUT("dummy-w5102-out"),
	SND_SOC_DAPM_INPUT("dummy-w5102-in"),
};

static const struct snd_soc_dapm_route w5102_routes[] = {
	{ "dummy-w5102-out", NULL, "Playback" },
	{ "Capture", NULL, "dummy-w5102-in" },
};

static struct snd_soc_dai_driver w5102_stub_dai = {
	.name		= "dummy_w5102_dai",
	.playback	= {
		.stream_name	= "Playback",
		.channels_min	= 2,
		.channels_max	= 2,
		.rates		= W5102_RATES,
		.formats	= W5102_FORMATS,
	},
	.capture	= {
		.stream_name	= "Capture",
		.channels_min	= 2,
		.channels_max	= 2,
		.rates		= W5102_RATES,
		.formats	= W5102_FORMATS,
	},

};

static const struct snd_soc_component_driver soc_component_wm5102_dummy = {
	.dapm_widgets = w5102_widgets,
	.num_dapm_widgets = ARRAY_SIZE(w5102_widgets),
	.dapm_routes = w5102_routes,
	.num_dapm_routes = ARRAY_SIZE(w5102_routes),
};

static int dummy_w5102_probe(struct platform_device *pdev)
{
	int ret;

	ret = devm_snd_soc_register_component(&pdev->dev,
					      &soc_component_wm5102_dummy,
					      &w5102_stub_dai, 1);

	return ret;
}

static struct platform_driver dummy_w5102_driver = {
	.probe		= dummy_w5102_probe,
	.driver		= {
		.name	= "dummy_w5102",
		.owner	= THIS_MODULE,
	},
};

module_platform_driver(dummy_w5102_driver);

MODULE_DESCRIPTION("dummy-w5102 dummy codec driver");
MODULE_ALIAS("platform: dummy_w5102");
MODULE_LICENSE("GPL v2");
