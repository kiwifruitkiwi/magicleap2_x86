/*
 * AMD ACP3x Dummy Machine Driver
 * Copyright (C) 2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <linux/module.h>
#include <linux/io.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include "acp3x.h"

static int acp3x_hw_params(struct snd_pcm_substream *substream,
			   struct snd_pcm_hw_params *params)

{
	return 0;
}

static int acp3x_bt_startup(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_card *card = rtd->card;
	struct acp3x_platform_info *machine = snd_soc_card_get_drvdata(card);

	machine->cap_i2s_instance = I2S_BT_INSTANCE;
	machine->play_i2s_instance = I2S_BT_INSTANCE;

	return 0;
}

static int acp3x_i2ssp_startup(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_card *card = rtd->card;
	struct acp3x_platform_info *machine = snd_soc_card_get_drvdata(card);

	machine->cap_i2s_instance = I2S_SP_INSTANCE;
	machine->play_i2s_instance = I2S_SP_INSTANCE;

	return 0;
}

static struct snd_soc_ops acp3x_wm5102_bt_ops = {
	.startup = acp3x_bt_startup,
	.hw_params = acp3x_hw_params,
};

static struct snd_soc_ops acp3x_wm5102_i2ssp_ops = {
	.startup = acp3x_i2ssp_startup,
	.hw_params = acp3x_hw_params,
};

SND_SOC_DAILINK_DEF(acp3x_i2s_play,
		    DAILINK_COMP_ARRAY(COMP_CPU("acp3x_i2s_playcap.0")));
SND_SOC_DAILINK_DEF(acp3x_i2s_capture,
		    DAILINK_COMP_ARRAY(COMP_CPU("acp3x_i2s_playcap.1")));
SND_SOC_DAILINK_DEF(acp3x_bt,
		    DAILINK_COMP_ARRAY(COMP_CPU("acp3x_i2s_playcap.2")));

SND_SOC_DAILINK_DEF(dummy_codec,
		    DAILINK_COMP_ARRAY(COMP_CODEC("dummy_w5102.0",
						  "dummy_w5102_dai")));
SND_SOC_DAILINK_DEF(platform,
		    DAILINK_COMP_ARRAY(COMP_PLATFORM("acp3x_rv_i2s_dma.0")));

static int acp3x_init(struct snd_soc_pcm_runtime *rtd)
{
	return 0;
}

static struct snd_soc_dai_link acp3x_dai_w5102[] = {
	{
		.name = "RV-W5102-PLAY-BT",
		.stream_name = "Playback/Capture BT",
		.dai_fmt = SND_SOC_DAIFMT_I2S  | SND_SOC_DAIFMT_NB_NF |
			   SND_SOC_DAIFMT_CBM_CFM,
		.ops = &acp3x_wm5102_bt_ops,
		.init = acp3x_init,
		SND_SOC_DAILINK_REG(acp3x_bt, dummy_codec, platform),
	},
	{
		.name = "RV-W5102-PLAY-I2S",
		.stream_name = "Play I2SSP",
		.dai_fmt = SND_SOC_DAIFMT_I2S  | SND_SOC_DAIFMT_NB_NF |
			   SND_SOC_DAIFMT_CBM_CFM,
		.ops = &acp3x_wm5102_i2ssp_ops,
		.init = acp3x_init,
		.playback_only = 1,
		SND_SOC_DAILINK_REG(acp3x_i2s_play, dummy_codec, platform),
	},
	{
		.name = "RV-W5102-CAPTURE-I2S",
		.stream_name = "Capture I2SSP",
		.dai_fmt = SND_SOC_DAIFMT_I2S  | SND_SOC_DAIFMT_NB_NF |
			   SND_SOC_DAIFMT_CBM_CFM,
		.ops = &acp3x_wm5102_i2ssp_ops,
		.init = acp3x_init,
		.capture_only = 1,
		SND_SOC_DAILINK_REG(acp3x_i2s_capture, dummy_codec, platform),
	},
};

static const struct snd_soc_dapm_widget acp3x_widgets[] = {
	SND_SOC_DAPM_HP("Headphones", NULL),
	SND_SOC_DAPM_MIC("Analog Mic", NULL),
};

static const struct snd_soc_dapm_route acp3x_audio_route[] = {
	{"Headphones", NULL, "HPO L"},
	{"Headphones", NULL, "HPO R"},
	{"MIC1", NULL, "Analog Mic"},
};

static struct snd_soc_card acp3x_card = {
	.name = "acp3x",
	.owner = THIS_MODULE,
	.dai_link = acp3x_dai_w5102,
	.num_links = 3,
};

static int acp3x_probe(struct platform_device *pdev)
{
	int ret;
	struct acp3x_platform_info *machine;
	struct snd_soc_card *card;

	machine = devm_kzalloc(&pdev->dev, sizeof(struct acp3x_platform_info),
			       GFP_KERNEL);
	if (!machine)
		return -ENOMEM;

	card = &acp3x_card;
	acp3x_card.dev = &pdev->dev;

	platform_set_drvdata(pdev, card);
	snd_soc_card_set_drvdata(card, machine);

	ret = devm_snd_soc_register_card(&pdev->dev, card);
	if (ret) {
		dev_err(&pdev->dev,
			"snd_soc_register_card(%s) failed: %d\n",
			acp3x_card.name, ret);
		return ret;
	}
	return 0;
}

static struct platform_driver acp3x_mach_driver = {
	.driver = {
		.name = "acp3x_w5102_mach",
		.pm = &snd_soc_pm_ops,
	},
	.probe = acp3x_probe,
};

static int __init acp3x_audio_init(void)
{
	platform_driver_register(&acp3x_mach_driver);
	return 0;
}

static void __exit acp3x_audio_exit(void)
{
	platform_driver_unregister(&acp3x_mach_driver);
}

module_init(acp3x_audio_init);
module_exit(acp3x_audio_exit);
MODULE_LICENSE("GPL v2");
