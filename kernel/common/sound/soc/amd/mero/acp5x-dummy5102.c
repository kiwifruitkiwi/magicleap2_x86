/*
 * AMD ACP5x Dummy Machine Driver
 * Copyright (C) 2020 Advanced Micro Devices, Inc. All rights reserved.
 */

#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <linux/module.h>
#include <linux/io.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include "acp5x.h"

static int i2s_tdm_mode;
module_param(i2s_tdm_mode, int, 0644);
MODULE_PARM_DESC(i2s_tdm_mode, "enables I2S TDM Mode");
static int i2s_master_mode = 0x01;
module_param(i2s_master_mode, int, 0644);
MODULE_PARM_DESC(i2s_master_mode, "enables I2S Master Mode");

#define DRV_NAME "acp5x_w5102_mach"

static int acp5x_hw_params(struct snd_pcm_substream *substream,
			   struct snd_pcm_hw_params *params)

{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	unsigned int fmt;
	unsigned int dai_fmt;
	unsigned int slot_width;
	unsigned int slots;
	unsigned int channels;
	int ret = 0;

	if (i2s_tdm_mode) {
		fmt = params_format(params);
		switch (fmt) {
		case SNDRV_PCM_FORMAT_S16_LE:
			slot_width = 16;
			break;
		case SNDRV_PCM_FORMAT_S24_LE:
			slot_width = 32;
			break;
		case SNDRV_PCM_FORMAT_S32_LE:
			slot_width = 32;
			break;
		default:
			pr_err("acp5x: unsupported PCM format\n");
			return -EINVAL;
		}

		if (i2s_master_mode)
			dai_fmt = SND_SOC_DAIFMT_DSP_A  | SND_SOC_DAIFMT_NB_NF |
				  SND_SOC_DAIFMT_CBM_CFM;
		else
			dai_fmt = SND_SOC_DAIFMT_DSP_A  | SND_SOC_DAIFMT_NB_NF |
				  SND_SOC_DAIFMT_CBS_CFS;
		ret = snd_soc_dai_set_fmt(cpu_dai, dai_fmt);
		if (ret < 0) {
			pr_err("can't set format to DSP_A mode ret:%d\n", ret);
			return ret;
		}

		channels = params_channels(params);
		if (channels == 0x08)
			slots = 0x00;
		else
			slots = channels;
		ret = snd_soc_dai_set_tdm_slot(cpu_dai, 0x3, 0x3, slots,
					       slot_width);
		if (ret < 0)
			pr_err("can't set I2S TDM mode config ret:%d\n", ret);
	} else {
		if (i2s_master_mode)
			dai_fmt = SND_SOC_DAIFMT_I2S  | SND_SOC_DAIFMT_NB_NF |
				  SND_SOC_DAIFMT_CBM_CFM;
		else
			dai_fmt = SND_SOC_DAIFMT_I2S  | SND_SOC_DAIFMT_NB_NF |
				  SND_SOC_DAIFMT_CBS_CFS;
		ret = snd_soc_dai_set_fmt(cpu_dai, dai_fmt);
		pr_debug("i2s_master_mode:0x%x dai fmt set:0x%x ret:%d\n",
			 i2s_master_mode, dai_fmt, ret);
	}
	return ret;
}

static int acp5x_bt_startup(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_card *card = rtd->card;
	struct acp5x_platform_info *machine = snd_soc_card_get_drvdata(card);

	machine->cap_i2s_instance = I2S_BT_INSTANCE;
	machine->play_i2s_instance = I2S_BT_INSTANCE;
	pr_debug("%s play_i2s_instance:0x%x cap_i2s_instance:0x%x\n", __func__,
		 machine->play_i2s_instance, machine->cap_i2s_instance);
	return 0;
}

static int acp5x_i2ssp_startup(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_card *card = rtd->card;
	struct acp5x_platform_info *machine = snd_soc_card_get_drvdata(card);

	machine->cap_i2s_instance = I2S_SP_INSTANCE;
	machine->play_i2s_instance = I2S_SP_INSTANCE;
	pr_debug("%s play_i2s_instance:0x%x cap_i2s_instance:0x%x\n", __func__,
		 machine->play_i2s_instance, machine->cap_i2s_instance);
	return 0;
}

static struct snd_soc_ops acp5x_wm5102_bt_ops = {
	.startup = acp5x_bt_startup,
	.hw_params = acp5x_hw_params,
};

static struct snd_soc_ops acp5x_wm5102_i2ssp_ops = {
	.startup = acp5x_i2ssp_startup,
	.hw_params = acp5x_hw_params,
};

SND_SOC_DAILINK_DEF(acp5x_i2s,
		    DAILINK_COMP_ARRAY(COMP_CPU("acp5x_i2s_playcap.0")));
SND_SOC_DAILINK_DEF(acp5x_bt,
		    DAILINK_COMP_ARRAY(COMP_CPU("acp5x_i2s_playcap.1")));

SND_SOC_DAILINK_DEF(dummy_codec,
		    DAILINK_COMP_ARRAY(COMP_CODEC("dummy_w5102.0",
						  "dummy_w5102_dai")));
SND_SOC_DAILINK_DEF(platform,
		    DAILINK_COMP_ARRAY(COMP_PLATFORM("acp5x_i2s_dma.0")));

static int acp5x_init(struct snd_soc_pcm_runtime *rtd)
{
	return 0;
}

static struct snd_soc_dai_link acp5x_dai_w5102[] = {
	{
		.name = "ACP5x-W5102-I2S",
		.stream_name = "Playback/Capture SP",
		.ops = &acp5x_wm5102_i2ssp_ops,
		.init = acp5x_init,
		SND_SOC_DAILINK_REG(acp5x_i2s, dummy_codec, platform),
	},
	{
		.name = "ACP5x-W5102-BT",
		.stream_name = "Playback/Capture BT",
		.ops = &acp5x_wm5102_bt_ops,
		.init = acp5x_init,
		SND_SOC_DAILINK_REG(acp5x_bt, dummy_codec, platform),
	},
};

static struct snd_soc_card acp5x_card = {
	.name = "acp5x",
	.owner = THIS_MODULE,
	.dai_link = acp5x_dai_w5102,
	.num_links = 2,
};

static int acp5x_probe(struct platform_device *pdev)
{
	int ret;
	struct acp5x_platform_info *machine;
	struct snd_soc_card *card;

	machine = devm_kzalloc(&pdev->dev, sizeof(struct acp5x_platform_info),
			       GFP_KERNEL);
	if (!machine)
		return -ENOMEM;

	card = &acp5x_card;
	acp5x_card.dev = &pdev->dev;

	platform_set_drvdata(pdev, card);
	snd_soc_card_set_drvdata(card, machine);

	ret = devm_snd_soc_register_card(&pdev->dev, card);
	if (ret) {
		dev_err(&pdev->dev,
			"snd_soc_register_card(%s) failed: %d\n",
			acp5x_card.name, ret);
		return ret;
	}
	return 0;
}

static struct platform_driver acp5x_mach_driver = {
	.driver = {
		.name = "acp5x_w5102_mach",
		.pm = &snd_soc_pm_ops,
	},
	.probe = acp5x_probe,
};

module_platform_driver(acp5x_mach_driver);

MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:" DRV_NAME);
