/* arch/arm/mach-msm/qdsp5/snd.c
 *
 * interface to "snd" service on the baseband cpu
 *
 * Copyright (C) 2008 HTC Corporation
 * Copyright (c) 2009, Code Aurora Forum. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <mach/debug_audio_mm.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/msm_audio.h>
#include <linux/seq_file.h>
#include <asm/atomic.h>
#include <asm/ioctls.h>
#include <mach/board.h>
#include <mach/msm_rpcrouter.h>
/* LGE_CHANGE [dojip.kim@lge.com] 2010-03-18, from cupcake */
#if defined (CONFIG_MACH_EVE)		
#include <mach/gpio.h>
#endif

struct snd_ctxt {
	struct mutex lock;
	int opened;
	struct msm_rpc_endpoint *ept;
	struct msm_snd_endpoints *snd_epts;
};

struct snd_sys_ctxt {
	struct mutex lock;
	struct msm_rpc_endpoint *ept;
};

static struct snd_sys_ctxt the_snd_sys;

static struct snd_ctxt the_snd;

#define RPC_SND_PROG	0x30000002
#define RPC_SND_CB_PROG	0x31000002

#define RPC_SND_VERS                    0x00020001

#define SND_SET_DEVICE_PROC 2
#define SND_SET_VOLUME_PROC 3
#define SND_AVC_CTL_PROC 29
#define SND_AGC_CTL_PROC 30

/* LGE_CHANGE [dojip.kim@lge.com] 2010-03-18, from cupcake */
#if defined (CONFIG_MACH_EVE)		
#define SND_SET_VOCCAL_PARAM_PROC 34
#define SND_SET_VOCCAL_IIR_PARAM_PROC 35
#define SND_SET_NEXT_EC_PARAM_PROC 36
#define SND_SET_RX_VOLUME_PROC 37
#define SND_SET_DTMF_VOLUME_PROC 38
#define SND_SET_PAD_VALUE_PROC 39
#define SND_SET_LOOPBACK_MODE_PROC 40
#define SND_WRITE_EFS_PROC 41
#define SND_SET_MICAMP_GAIN_PROC 42
#define SND_SET_AMP_GAIN_PROC 43
#define SND_WRITE_MEM_PROC 44
#define SND_SET_FM_RADIO_VOLUME_PROC 45
#define SND_SET_VOICE_CLARITY_PROC 46
#define SND_SET_POWER_OFF_PROC 47
#define SND_SET_FM_RADIO_MULTI_SOUND_PROC 48


#define GPIO_HS_MIC_BIAS_EN		26

#define DEBUG_SND	1

#if DEBUG_SND
#define D(fmt, args...) printk(fmt, ##args)
#else
#define D(fmt, args...) do () while(0)
#endif
#endif /* CONFIG_MACH_EVE */

struct rpc_snd_set_device_args {
	uint32_t device;
	uint32_t ear_mute;
	uint32_t mic_mute;

	uint32_t cb_func;
	uint32_t client_data;
};

struct rpc_snd_set_volume_args {
	uint32_t device;
	uint32_t method;
	uint32_t volume;

	uint32_t cb_func;
	uint32_t client_data;
};

struct rpc_snd_avc_ctl_args {
	uint32_t avc_ctl;
	uint32_t cb_func;
	uint32_t client_data;
};

struct rpc_snd_agc_ctl_args {
	uint32_t agc_ctl;
	uint32_t cb_func;
	uint32_t client_data;
};

struct snd_set_device_msg {
	struct rpc_request_hdr hdr;
	struct rpc_snd_set_device_args args;
};

struct snd_set_volume_msg {
	struct rpc_request_hdr hdr;
	struct rpc_snd_set_volume_args args;
};

struct snd_avc_ctl_msg {
	struct rpc_request_hdr hdr;
	struct rpc_snd_avc_ctl_args args;
};

struct snd_agc_ctl_msg {
	struct rpc_request_hdr hdr;
	struct rpc_snd_agc_ctl_args args;
};

struct snd_endpoint *get_snd_endpoints(int *size);

/* LGE_CHANGE [dojip.kim@lge.com] 2010-03-18, from cupcake */
#if defined (CONFIG_MACH_EVE)		
struct rpc_snd_set_voccal_param_args {
	voc_codec_type voc_codec;
	voccal_property_enum_type voccal_param_type;
	int get_flag;  //get_flag = 0 for set, get_flag = 1 for get
    uint32_t param_val;

	uint32_t cb_func;
	uint32_t client_data;
};

struct snd_set_voccal_param_msg {
	struct rpc_request_hdr hdr;
	struct rpc_snd_set_voccal_param_args args;
};

struct rpc_snd_set_voccal_iir_param_args {
     voc_codec_type voc_codec;
     voccal_iir_filter_type voccal_iir_param_type;
	 int get_flag;  //get_flag = 0 for set, get_flag = 1 for get
     int32_t param_val;
 
     uint32_t cb_func;
     uint32_t client_data;
};
 
struct snd_set_voccal_iir_param_msg {
    struct rpc_request_hdr hdr;
    struct rpc_snd_set_voccal_iir_param_args args;
};

struct rpc_snd_set_next_ec_param_args {
     voc_ec_type ec_mode;
     nextgen_ec_param_enum_type ec_param_type;
	 int get_flag;  //get_flag = 0 for set, get_flag = 1 for get
     int32_t param_val;
 
     uint32_t cb_func;
     uint32_t client_data;
};
 
struct snd_set_next_ec_param_msg {
    struct rpc_request_hdr hdr;
    struct rpc_snd_set_next_ec_param_args args;
};

struct rpc_snd_set_rx_volume_param_args {
     uint32_t device;
     uint32_t method;
     uint32_t idx;
	 int get_flag;  //get_flag = 0 for set, get_flag = 1 for get
     int32_t param_val;
 
     uint32_t cb_func;
     uint32_t client_data;
};
 
struct snd_set_rx_volume_param_msg {
    struct rpc_request_hdr hdr;
    struct rpc_snd_set_rx_volume_param_args args;
};

struct rpc_snd_set_dtmf_volume_param_args {
     uint32_t device;
     uint32_t method;
     uint32_t idx;
	 int get_flag;  //get_flag = 0 for set, get_flag = 1 for get
     int32_t param_val;
 
     uint32_t cb_func;
     uint32_t client_data;
};
 
struct snd_set_dtmf_volume_param_msg {
    struct rpc_request_hdr hdr;
    struct rpc_snd_set_dtmf_volume_param_args args;
};

struct rpc_snd_set_pad_value_param_args {
     uint32_t device;
     uint32_t method;
     uint32_t idx;
	 int get_flag;  //get_flag = 0 for set, get_flag = 1 for get
     int32_t param_val;
 
     uint32_t cb_func;
     uint32_t client_data;
};
 
struct snd_set_pad_value_param_msg {
    struct rpc_request_hdr hdr;
    struct rpc_snd_set_pad_value_param_args args;
};

struct rpc_snd_set_amp_gain_param_args {
     voc_codec_type voc_codec;
     amp_gain_type gain_type;
     int get_flag;  //get_flag = 0 for set, get_flag = 1 for get
	 int get_param;
     uint32_t cb_func;
     uint32_t client_data;
};

struct snd_set_set_amp_gain_param_msg {
    struct rpc_request_hdr hdr;
    struct rpc_snd_set_amp_gain_param_args args;
};


struct rpc_snd_set_loopback_mode_args {
     uint32_t mode;
//	 int get_flag;  //get_flag = 0 for set, get_flag = 1 for get
 
     uint32_t cb_func;
     uint32_t client_data;
};
 
struct snd_set_loopback_mode_msg {
    struct rpc_request_hdr hdr;
    struct rpc_snd_set_loopback_mode_args args;
};

struct rpc_snd_write_efs_args {
     uint32_t cb_func;
     uint32_t client_data;
};
 
struct snd_write_efs_msg {
    struct rpc_request_hdr hdr;
    struct rpc_snd_write_efs_args args;
};
struct rpc_snd_set_micamp_gain_param_args {
     int32_t voc_codec;
     int32_t mic_channel;
	 int get_flag;  //get_flag = 0 for set, get_flag = 1 for get
     int32_t get_param;
     uint32_t cb_func;
     uint32_t client_data;
};

struct snd_set_micamp_gain_param_msg {
    struct rpc_request_hdr hdr;
    struct rpc_snd_set_micamp_gain_param_args args;
};

struct rpc_snd_set_fm_radio_vol_args {
     uint32_t volume;
     uint32_t cb_func;
     uint32_t client_data;
};
 
struct snd_set_fm_radio_vol_msg {
    struct rpc_request_hdr hdr;
    struct rpc_snd_set_fm_radio_vol_args args;
};

struct rpc_snd_set_voice_clarity_args {
     uint32_t mode;
     uint32_t cb_func;
     uint32_t client_data;
};

struct snd_set_voice_clarity_msg {
    struct rpc_request_hdr hdr;
    struct rpc_snd_set_voice_clarity_args args;
};


#if 0
enum audio_path {
	RCV_AMP = 0,
	HS_AMP = 2,
	HS_STEREO_AMP = 3,
	SPK_MEDIA_AMP = 5,
	SPK_AMP = 6,
	AUD_PWROFF = 0xFF,
};
#endif
#define AUD_PWROFF 0xFF

static atomic_t current_snd_device;
//static atomic_t amp_ctl_mode;
//static int loopback_at=false;
//static int last_device_at=-1;
static int fm_radio_flag = 0;


extern int amp_write_register(char reg, char val);
extern int amp_read_register(char reg, char *ret);

#if 0 //unused
static void set_amp_gain(voc_codec_type voc_codec, amp_gain_type gain_type, int value)
{
	switch(voc_codec) {
		case  VOC_CODEC_ON_CHIP_0:
			D("voc_codec %d does not use the amp\n", voc_codec);
			break;
		case  VOC_CODEC_STEREO_HEADSET:
			if ( gain_type == HPH ){
				amp_write_register(0x00, 0x20);
				amp_write_register(0x04, 0x82);
				msleep(10);
				amp_write_register(0x02, value);
				amp_write_register(0x03, value);
			} else {
				D("voc_codec %d does not use gain_type[%d]\n",voc_codec, gain_type);
			}
			break;
		case  VOC_CODEC_STEREO_HEADSET_MEDIA:
			if ( gain_type == HPH) {
				amp_write_register(0x00, 0x00);
				amp_write_register(0x04, 0x82);
				msleep(10);
				amp_write_register(0x02, value);
				amp_write_register(0x03, value);
			} else {
				D("voc_codec %d does not use gain_type[%d]\n",voc_codec, gain_type);
			}
			break;
		case  VOC_CODEC_SPEAKER_MEDIA:
			if ( gain_type == SPK) {
				amp_write_register(0x00, 0x11); //9db
				amp_write_register(0x04, 0x84);
				msleep(10);
				amp_write_register(0x01, value);
			} else {
				D("voc_codec %d does not use gain_type[%d]\n",voc_codec, gain_type);
			}
			break;
		case  VOC_CODEC_SPEAKER_PHONE:
			if ( gain_type == SPK) {
				amp_write_register(0x00, 0x10);
				amp_write_register(0x04, 0x84);
				msleep(10);
				amp_write_register(0x01, value);
			} else {
				D("voc_codec %d does not use gain_type[%d]\n",voc_codec, gain_type);
			}
			break;
		case  VOC_CODEC_SPEAKER_RING:
			if ( gain_type == HPH) {
				amp_write_register(0x00, 0x00);
				amp_write_register(0x04, 0x83);
				msleep(10);
				amp_write_register(0x02, value);
				amp_write_register(0x03, value);
			} else if ( gain_type == SPK) {
				amp_write_register(0x00, 0x00);
				amp_write_register(0x04, 0x83);
				msleep(10);
				amp_write_register(0x01, value);
			} else {
				D("voc_codec %d does not use gain_type[%d]\n",voc_codec, gain_type);
			}
			break;
		default :
			printk("GW620 : voc_codec %d does not support AMP cal tool\n", voc_codec);
	}
}


static int get_amp_gain(voc_codec_type voc_codec, amp_gain_type gain_type)
{
	char ret = 0;

	switch(voc_codec) {
		case  VOC_CODEC_ON_CHIP_0:
			D("voc_codec %d does not use the amp\n", voc_codec);
			break;
		case  VOC_CODEC_STEREO_HEADSET:
		case  VOC_CODEC_STEREO_HEADSET_MEDIA:
			if ( gain_type == HPH ){
				amp_read_register(0x02, &ret);
				D("amp_read : 0x02 => %x\n", ret);
			} else {
				D("voc_codec %d does not use gain_type[%d]\n",voc_codec, gain_type);
			}
			break;
		case  VOC_CODEC_SPEAKER_MEDIA:
		case  VOC_CODEC_SPEAKER_PHONE:
			if ( gain_type == SPK ){
				amp_read_register(0x01, &ret);
				D("amp_read : 0x01 => %x\n", ret);
			} else {
				D("voc_codec %d does not use gain_type[%d]\n",voc_codec, gain_type);
			}
			break;
		case  VOC_CODEC_SPEAKER_RING:
			if ( gain_type == HPH ){
				amp_read_register(0x02, &ret);
				D("amp_read : 0x02 => %x\n", ret);
			} else if ( gain_type == SPK ) {
				amp_read_register(0x01, &ret);
				D("amp_read : 0x01 => %x\n", ret);
			} else {
				D("voc_codec %d does not use gain_type[%d]\n",voc_codec, gain_type);
			}
			break;
		default :
			printk("GW620 : voc_codec %d does not support AMP cal tool\n", voc_codec);
			return -EINVAL;
	}
	return ret;
}

static int amp_control(uint32_t path)
{
	//int value;
	//char i;
	//unsigned long amp_status;
	//unsigned char ret;
	
	switch (path){
		case VOC_CODEC_SPEAKER_PHONE:
			printk("GW620 : SPK AMP ON\n");
			set_amp_gain(VOC_CODEC_SPEAKER_PHONE, SPK, 0x1F);
			gpio_set_value(GPIO_HS_MIC_BIAS_EN, 0);
			break;
		case VOC_CODEC_SPEAKER_MEDIA:
		case VOC_CODEC_FM_RADIO_SPEAKER_MEDIA:
			printk("GW620 : SPK MEDIA AMP ON\n");
			set_amp_gain(VOC_CODEC_SPEAKER_MEDIA, SPK, 0x1F);
			gpio_set_value(GPIO_HS_MIC_BIAS_EN, 0);
#if 0 //for debugging
			amp_read_register(0x00, &ret);
			printk("amp_read : 0x00 => %2x\n", ret);
			amp_read_register(0x04, &ret);
			printk("amp_read : 0x04 => %2x\n", ret);
			amp_read_register(0x01, &ret);
			printk("amp_read : 0x01 => %2x\n", ret);
#endif
			break;
		case VOC_CODEC_ON_CHIP_0:
			printk("GW620 : RCV AMP ON\n");
			amp_write_register(0x01, 0x00);
			amp_write_register(0x02, 0x00);
			amp_write_register(0x03, 0x00);
			amp_write_register(0x04, 0x40);
			gpio_set_value(GPIO_HS_MIC_BIAS_EN, 0);
			break;
		case VOC_CODEC_STEREO_HEADSET:
			printk("GW620 : HS AMP ON\n");
			set_amp_gain(VOC_CODEC_STEREO_HEADSET, HPH, 0x1F);
			gpio_set_value(GPIO_HS_MIC_BIAS_EN, 1);
			break;
		case VOC_CODEC_STEREO_HEADSET_MEDIA:
		case VOC_CODEC_FM_RADIO_HEADSET_MEDIA:
			printk("GW620 : HS STEREO AMP ON\n");
			set_amp_gain(VOC_CODEC_STEREO_HEADSET_MEDIA, HPH, 0x1F);
			gpio_set_value(GPIO_HS_MIC_BIAS_EN, 1);
			break;
		case VOC_CODEC_SPEAKER_RING:
			printk("GW620 : SPEAKER RING AMP ON\n");
			set_amp_gain(VOC_CODEC_SPEAKER_RING, HPH, 0x1F);
			gpio_set_value(GPIO_HS_MIC_BIAS_EN, 1);
			break;
		case AUD_PWROFF:
			printk("GW620 : AMP OFF\n");
			amp_write_register(0x01, 0x00); 
			amp_write_register(0x02, 0x00);
			amp_write_register(0x03, 0x00);
			amp_write_register(0x04, 0x00);
			gpio_set_value(GPIO_HS_MIC_BIAS_EN, 0);
			break;
		default :
			printk("GW620 : path[%d] does not support AMP\n", path);
			return -EINVAL;
	}
}
#endif //0
#endif //defined(EVE

static inline int check_mute(int mute)
{
	return (mute == SND_MUTE_MUTED ||
		mute == SND_MUTE_UNMUTED) ? 0 : -EINVAL;
}

static int get_endpoint(struct snd_ctxt *snd, unsigned long arg)
{
	int rc = 0, index;
	struct msm_snd_endpoint ept;

	if (copy_from_user(&ept, (void __user *)arg, sizeof(ept))) {
		MM_ERR("snd_ioctl get endpoint: invalid read pointer\n");
		return -EFAULT;
	}

	index = ept.id;
	if (index < 0 || index >= snd->snd_epts->num) {
		MM_ERR("snd_ioctl get endpoint: invalid index!\n");
		return -EINVAL;
	}

	ept.id = snd->snd_epts->endpoints[index].id;
	strncpy(ept.name,
		snd->snd_epts->endpoints[index].name,
		sizeof(ept.name));

	if (copy_to_user((void __user *)arg, &ept, sizeof(ept))) {
		MM_ERR("snd_ioctl get endpoint: invalid write pointer\n");
		rc = -EFAULT;
	}

	return rc;
}

static long snd_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct snd_set_device_msg dmsg;
	struct snd_set_volume_msg vmsg;
	struct snd_avc_ctl_msg avc_msg;
	struct snd_agc_ctl_msg agc_msg;

	struct msm_snd_device_config dev;
	struct msm_snd_volume_config vol;
	struct snd_ctxt *snd = file->private_data;
	/* LGE_CHANGE [dojip.kim@lge.com] 2010-03-18, from cupcake */
#if defined (CONFIG_MACH_EVE)		
	struct msm_snd_set_rx_volume_param rxvol;
	struct snd_set_rx_volume_param_msg rmsg;
	struct msm_snd_set_pad_value_param padvalue;
	struct snd_set_pad_value_param_msg pmsg;
	struct snd_write_efs_msg wmsg;
	struct msm_snd_set_fm_radio_vol_param fmradiovol;
	struct snd_set_fm_radio_vol_msg fmrmsg;
	struct msm_snd_set_voice_clarity_param voiceclarity;
	struct snd_set_voice_clarity_msg vcmsg;
	

	//int fm_radio_vol;
	int wefs;
	struct snd_set_voccal_param_rep {
		struct rpc_reply_hdr hdr;
		uint32_t get_voccal;
	}crep;
	struct snd_set_voccal_iir_param_rep {
		struct rpc_reply_hdr hdr;
		uint32_t get_voccal_iir;
	}cirep;
	struct snd_set_nextec_param_rep {
		struct rpc_reply_hdr hdr;
		uint32_t get_nextec;
	}nrep;
	struct snd_set_rxvol_param_rep {
		struct rpc_reply_hdr hdr;
		uint32_t get_rxvol;
	}rrep;
	struct snd_set_dtmfvol_param_rep {
		struct rpc_reply_hdr hdr;
		uint32_t get_dtmfvol;
	}frep;
	struct snd_set_padvalue_param_rep {
		struct rpc_reply_hdr hdr;
		uint32_t get_padvalue;
	}prep;
	struct snd_set_amp_gain_param_rep {
		struct rpc_reply_hdr hdr;
		uint32_t get_gainvalue;
	}arep;
	struct snd_set_loopback_param_rep {
		struct rpc_reply_hdr hdr;
		uint32_t get_mode;
	}lrep;
	struct snd_write_efs_rep {
		struct rpc_reply_hdr hdr;
		uint32_t result;
	}wrep;
	struct snd_set_micamp_item_param_rep {
		struct rpc_reply_hdr hdr;
		uint32_t get_gainvalue;
	}mrep;
	struct snd_set_voice_clarity_param_rep {
		struct rpc_reply_hdr hdr;
		uint32_t get_mode;
	}vcrep;
	int get_device;
#endif	
	int rc = 0;

	uint32_t avc, agc;

	mutex_lock(&snd->lock);
	switch (cmd) {
	case SND_SET_DEVICE:
		if (copy_from_user(&dev, (void __user *) arg, sizeof(dev))) {
			MM_ERR("set device: invalid pointer\n");
			rc = -EFAULT;
			break;
		}

		dmsg.args.device = cpu_to_be32(dev.device);
		dmsg.args.ear_mute = cpu_to_be32(dev.ear_mute);
		dmsg.args.mic_mute = cpu_to_be32(dev.mic_mute);
		if (check_mute(dev.ear_mute) < 0 ||
				check_mute(dev.mic_mute) < 0) {
			MM_ERR("set device: invalid mute status\n");
			rc = -EINVAL;
			break;
		}
		dmsg.args.cb_func = -1;
		dmsg.args.client_data = 0;

		MM_INFO("snd_set_device %d %d %d\n", dev.device,
				dev.ear_mute, dev.mic_mute);

		rc = msm_rpc_call(snd->ept,
			SND_SET_DEVICE_PROC,
			&dmsg, sizeof(dmsg), 5 * HZ);
		/* LGE_CHANGE_S, [antosky@lge.com] , 2009-09-06, To keep fm radio from changing device */
#if defined (CONFIG_MACH_EVE)
		if (dev.device == VOC_CODEC_FM_RADIO_HEADSET_MEDIA
			|| dev.device == VOC_CODEC_FM_RADIO_SPEAKER_MEDIA){
				fm_radio_flag =1;
			}
		else if((dev.device == VOC_CODEC_STEREO_HEADSET_MEDIA
			|| dev.device== VOC_CODEC_SPEAKER_MEDIA
			|| dev.device== VOC_CODEC_VOICE_RECORDER
			|| dev.device== VOC_CODEC_A2DP_BT_HEADSET)
			&& (fm_radio_flag == 1)){
			fm_radio_flag = 0;
		}

		if (dev.device != 25){
			if(fm_radio_flag == 0){
				pr_info("snd_set_device - not fm radio\n");
				atomic_set(&current_snd_device, dev.device);
			}
			else{
				pr_info("snd_set_device - fm radio\n");
				atomic_set(&current_snd_device, VOC_CODEC_FM_RADIO_HEADSET_MEDIA);
			}
		}
#endif		
		/* LGE_CHANGE_E, [antosky@lge.com] , 2009-09-06, To keep fm radio from changing device */
		break;

	case SND_SET_VOLUME:
		if (copy_from_user(&vol, (void __user *) arg, sizeof(vol))) {
			MM_ERR("set volume: invalid pointer\n");
			rc = -EFAULT;
			break;
		}

		vmsg.args.device = cpu_to_be32(vol.device);
		vmsg.args.method = cpu_to_be32(vol.method);
/* LGE_CHANGE [dojip.kim@lge.com] 2010-03-18, from cupcake */
#if defined (CONFIG_MACH_EVE)		
#else		
		if (vol.method != SND_METHOD_VOICE) {
			MM_ERR("set volume: invalid method\n");
			rc = -EINVAL;
			break;
		}
#endif
		vmsg.args.volume = cpu_to_be32(vol.volume);
		vmsg.args.cb_func = -1;
		vmsg.args.client_data = 0;

		MM_INFO("snd_set_volume %d %d %d\n", vol.device,
				vol.method, vol.volume);

		rc = msm_rpc_call(snd->ept,
			SND_SET_VOLUME_PROC,
			&vmsg, sizeof(vmsg), 5 * HZ);
		break;

	case SND_AVC_CTL:
		if (get_user(avc, (uint32_t __user *) arg)) {
			rc = -EFAULT;
			break;
		} else if ((avc != 1) && (avc != 0)) {
			rc = -EINVAL;
			break;
		}

		avc_msg.args.avc_ctl = cpu_to_be32(avc);
		avc_msg.args.cb_func = -1;
		avc_msg.args.client_data = 0;

		MM_INFO("snd_avc_ctl %d\n", avc);

		rc = msm_rpc_call(snd->ept,
			SND_AVC_CTL_PROC,
			&avc_msg, sizeof(avc_msg), 5 * HZ);
		break;

	case SND_AGC_CTL:
		if (get_user(agc, (uint32_t __user *) arg)) {
			rc = -EFAULT;
			break;
		} else if ((agc != 1) && (agc != 0)) {
			rc = -EINVAL;
			break;
		}
		agc_msg.args.agc_ctl = cpu_to_be32(agc);
		agc_msg.args.cb_func = -1;
		agc_msg.args.client_data = 0;

		MM_INFO("snd_agc_ctl %d\n", agc);

		rc = msm_rpc_call(snd->ept,
			SND_AGC_CTL_PROC,
			&agc_msg, sizeof(agc_msg), 5 * HZ);
		break;

	case SND_GET_NUM_ENDPOINTS:
		if (copy_to_user((void __user *)arg,
				&snd->snd_epts->num, sizeof(unsigned))) {
			MM_ERR("get endpoint: invalid pointer\n");
			rc = -EFAULT;
		}
		break;

	case SND_GET_ENDPOINT:
		rc = get_endpoint(snd, arg);
		break;

/* LGE_CHANGE [dojip.kim@lge.com] 2010-03-18, from cupcake */
#if defined (CONFIG_MACH_EVE)		
	case SND_SET_VOCCAL_PARAM:
    {
        struct msm_snd_set_voccal_param voccal;
        struct snd_set_voccal_param_msg cmsg;

		if (copy_from_user(&voccal, (void __user *) arg, sizeof(voccal))) {
			pr_err("snd_ioctl set voccal_param: invalid pointer.\n");
			rc = -EFAULT;
			break;
		}
		cmsg.args.voc_codec = cpu_to_be32(voccal.voc_codec);
		cmsg.args.voccal_param_type = cpu_to_be32(voccal.voccal_param_type);
		cmsg.args.get_flag = cpu_to_be32(voccal.get_flag);
		cmsg.args.param_val = cpu_to_be32(voccal.param_val);
		cmsg.args.cb_func = -1;
		cmsg.args.client_data = 0;
		pr_info("snd_set_voccal_param %d %d %d %d\n", voccal.voc_codec,
						 voccal.voccal_param_type, voccal.get_flag,  voccal.param_val);

		rc = msm_rpc_call_reply(snd->ept,
			SND_SET_VOCCAL_PARAM_PROC,
			&cmsg, sizeof(cmsg),&crep, sizeof(crep), 5 * HZ);
		if (rc < 0){
			printk(KERN_ERR "%s:rpc err because of %d\n", __func__, rc);
		}
		else
		{
			voccal.get_param = be32_to_cpu(crep.get_voccal);
			printk(KERN_INFO "%s:voccal ->%d\n", __func__, voccal.get_param);
			if (copy_to_user((void __user *)arg, &voccal, sizeof(voccal))) {
				pr_err("snd_ioctl get voccal: invalid write pointer.\n");
				rc = -EFAULT;
			}
		}
		break;
    }
	case SND_SET_VOCCAL_IIR_PARAM:
    {
        struct msm_snd_set_voccal_iir_param voccaliir;
        struct snd_set_voccal_iir_param_msg cimsg;

		if (copy_from_user(&voccaliir, (void __user *) arg, sizeof(voccaliir))) {
			pr_err("snd_ioctl set_voccal_iir_param: invalid pointer.\n");
			rc = -EFAULT;
			break;
		}
		cimsg.args.voc_codec = cpu_to_be32(voccaliir.voc_codec);
		cimsg.args.voccal_iir_param_type = cpu_to_be32(voccaliir.voccal_iir_param_type);
		cimsg.args.get_flag = cpu_to_be32(voccaliir.get_flag);
		cimsg.args.param_val = cpu_to_be32(voccaliir.param_val);
		cimsg.args.cb_func = -1;
		cimsg.args.client_data = 0;
		pr_info("set_voccal_iir_param %d %d %d\n", voccaliir.voc_codec,
						 voccaliir.voccal_iir_param_type, voccaliir.param_val);

		rc = msm_rpc_call_reply(snd->ept,
			SND_SET_VOCCAL_IIR_PARAM_PROC,
			&cimsg, sizeof(cimsg),&cirep, sizeof(cirep), 5 * HZ);
		if (rc < 0){
			printk(KERN_ERR "%s:rpc err because of %d\n", __func__, rc);
		}
		else
		{
			voccaliir.get_param = be32_to_cpu(cirep.get_voccal_iir);
			printk(KERN_INFO "%s:voccal_iir ->%d\n", __func__, voccaliir.get_param);
			if (copy_to_user((void __user *)arg, &voccaliir, sizeof(voccaliir))) {
				pr_err("snd_ioctl get voccal iir: invalid write pointer.\n");
				rc = -EFAULT;
			}
		}
		break;
    }
	case SND_SET_NEXT_EC_PARAM:
    {
        struct msm_snd_set_next_ec_param nextec;
        struct snd_set_next_ec_param_msg nmsg;

		if (copy_from_user(&nextec, (void __user *) arg, sizeof(nextec))) {
			pr_err("snd_ioctl set_next_ec_param: invalid pointer.\n");
			rc = -EFAULT;
			break;
		}
		nmsg.args.ec_mode = cpu_to_be32(nextec.ec_mode);
		nmsg.args.ec_param_type = cpu_to_be32(nextec.ec_param_type);
		nmsg.args.get_flag = cpu_to_be32(nextec.get_flag);
		nmsg.args.param_val = cpu_to_be32(nextec.param_val);
		nmsg.args.cb_func = -1;
		nmsg.args.client_data = 0;
		pr_info("set_next_ec_param %d %d %d\n", nextec.ec_mode,
						 nextec.ec_param_type, nextec.param_val);

		rc = msm_rpc_call_reply(snd->ept,
			SND_SET_NEXT_EC_PARAM_PROC,
			&nmsg, sizeof(nmsg),&nrep, sizeof(nrep), 5 * HZ);
		if (rc < 0){
			printk(KERN_ERR "%s:rpc err because of %d\n", __func__, rc);
		}
		else
		{
			nextec.get_param = be32_to_cpu(nrep.get_nextec);
			printk(KERN_INFO "%s:nextec ->%d\n", __func__, nextec.get_param);
			if (copy_to_user((void __user *)arg, &nextec, sizeof(nextec))) {
				pr_err("snd_ioctl get next ec: invalid write pointer.\n");
				rc = -EFAULT;
			}
		}
		break;
    }
	case SND_SET_RX_VOLUME:
		if (copy_from_user(&rxvol, (void __user *) arg, sizeof(rxvol))) {
			pr_err("snd_ioctl set_rx_volume: invalid pointer.\n");
			rc = -EFAULT;
			break;
		}
		rmsg.args.device = cpu_to_be32(rxvol.device);
		rmsg.args.method = cpu_to_be32(rxvol.method);
		rmsg.args.idx = cpu_to_be32(rxvol.idx);
		rmsg.args.get_flag = cpu_to_be32(rxvol.get_flag);
		rmsg.args.param_val = cpu_to_be32(rxvol.param_val);
		rmsg.args.cb_func = -1;
		rmsg.args.client_data = 0;
		pr_info("set_rx_volume %d %d %d %d\n", rxvol.device,
						 rxvol.method, rxvol.idx, rxvol.param_val);

		rc = msm_rpc_call_reply(snd->ept,
			SND_SET_RX_VOLUME_PROC,
			&rmsg, sizeof(rmsg),&rrep, sizeof(rrep), 5 * HZ);
		if (rc < 0){
			printk(KERN_ERR "%s:rpc err because of %d\n", __func__, rc);
		}
		else
		{
			rxvol.get_param = be32_to_cpu(rrep.get_rxvol);
			printk(KERN_INFO "%s:rx vol ->%d\n", __func__, rxvol.get_param);
			if (copy_to_user((void __user *)arg, &rxvol, sizeof(rxvol))) {
				pr_err("snd_ioctl get rx vol: invalid write pointer.\n");
				rc = -EFAULT;
			}
		}
		break;

	case SND_SET_DTMF_VOLUME:
    {
        struct msm_snd_set_dtmf_volume_param dtmfvol;
        struct snd_set_dtmf_volume_param_msg fmsg;

		if (copy_from_user(&dtmfvol, (void __user *) arg, sizeof(dtmfvol))) {
			pr_err("snd_ioctl set_dtmf_volume: invalid pointer.\n");
			rc = -EFAULT;
			break;
		}
		fmsg.args.device = cpu_to_be32(dtmfvol.device);
		fmsg.args.method = cpu_to_be32(dtmfvol.method);
		fmsg.args.idx = cpu_to_be32(dtmfvol.idx);
		fmsg.args.get_flag = cpu_to_be32(dtmfvol.get_flag);
		fmsg.args.param_val = cpu_to_be32(dtmfvol.param_val);
		fmsg.args.cb_func = -1;
		fmsg.args.client_data = 0;
		pr_info("set_dtmf_volume %d %d %d %d\n", dtmfvol.device,
						 dtmfvol.method, dtmfvol.idx, dtmfvol.param_val);

		rc = msm_rpc_call_reply(snd->ept,
			SND_SET_DTMF_VOLUME_PROC,
			&fmsg, sizeof(fmsg),&frep, sizeof(frep), 5 * HZ);
		if (rc < 0){
			printk(KERN_ERR "%s:rpc err because of %d\n", __func__, rc);
		}
		else
		{
			dtmfvol.get_param = be32_to_cpu(frep.get_dtmfvol);
			printk(KERN_INFO "%s:rx vol ->%d\n", __func__, dtmfvol.get_param);
			if (copy_to_user((void __user *)arg, &dtmfvol, sizeof(dtmfvol))) {
				pr_err("snd_ioctl get dtmf vol: invalid write pointer.\n");
				rc = -EFAULT;
			}
		}
		break;
    }
	case SND_SET_PAD_VALUE:
		if (copy_from_user(&padvalue, (void __user *) arg, sizeof(padvalue))) {
			pr_err("snd_ioctl set_pad_value: invalid pointer.\n");
			rc = -EFAULT;
			break;
		}
		pmsg.args.device = cpu_to_be32(padvalue.device);
		pmsg.args.method = cpu_to_be32(padvalue.method);
		pmsg.args.idx = cpu_to_be32(padvalue.idx);
		pmsg.args.get_flag = cpu_to_be32(padvalue.get_flag);
		pmsg.args.param_val = cpu_to_be32(padvalue.param_val);
		pmsg.args.cb_func = -1;
		pmsg.args.client_data = 0;
		pr_info("set_pad_value %d %d %d %d\n", padvalue.device,
						 padvalue.method, padvalue.idx, padvalue.param_val);

		rc = msm_rpc_call_reply(snd->ept,
			SND_SET_PAD_VALUE_PROC,
			&pmsg, sizeof(pmsg),&prep, sizeof(prep), 5 * HZ);
		if (rc < 0){
			printk(KERN_ERR "%s:rpc err because of %d\n", __func__, rc);
		}
		else
		{
			padvalue.get_param = be32_to_cpu(prep.get_padvalue);
			printk(KERN_INFO "%s:rx vol ->%d\n", __func__, padvalue.get_param);
			if (copy_to_user((void __user *)arg, &padvalue, sizeof(padvalue))) {
				pr_err("snd_ioctl get pad value: invalid write pointer.\n");
				rc = -EFAULT;
			}
		}
		break;

	case SND_SET_LOOPBACK_MODE:
    {
         struct snd_set_loopback_mode_msg lmsg;
         struct msm_snd_set_loopback_mode_param loopback;

		if (copy_from_user(&loopback, (void __user *) arg, sizeof(loopback))) {
			pr_err("snd_ioctl set amp_gain: invalid pointer.\n");
			rc = -EFAULT;
			break;
		}
		lmsg.args.mode = cpu_to_be32(loopback.mode);
		lmsg.args.cb_func = -1;
		lmsg.args.client_data = 0;
		pr_info("set_loopback_mode %d \n", loopback.mode);
					
		rc = msm_rpc_call(snd->ept,
			SND_SET_LOOPBACK_MODE_PROC,
			&lmsg, sizeof(lmsg), 5 * HZ);
		if (rc < 0){
			printk(KERN_ERR "%s:rpc err because of %d\n", __func__, rc);
		}
		else
		{
			loopback.get_param = be32_to_cpu(lrep.get_mode);
			printk(KERN_INFO "%s:loopback mode ->%d\n", __func__, loopback.get_param);
			if (copy_to_user((void __user *)arg, &loopback, sizeof(loopback))) {
				pr_err("snd_ioctl get loopback mode: invalid write pointer.\n");
				rc = -EFAULT;
			}
		}
		break;
    }
	case SND_SET_AMP_GAIN:
    {
        struct msm_snd_set_amp_gain_param ampgain;
        struct snd_set_set_amp_gain_param_msg amsg;

		if (copy_from_user(&ampgain, (void __user *) arg, sizeof(ampgain))) {
			pr_err("snd_ioctl set amp_gain: invalid pointer.\n");
			rc = -EFAULT;
			break;
		}
		amsg.args.voc_codec = cpu_to_be32(ampgain.voc_codec);
		amsg.args.gain_type = cpu_to_be32(ampgain.gain_type);
		amsg.args.get_flag = cpu_to_be32(ampgain.get_flag);
		amsg.args.get_param = cpu_to_be32(ampgain.value);
		amsg.args.cb_func = -1;
		amsg.args.client_data = 0;
		rc = msm_rpc_call_reply(snd->ept,
			SND_SET_AMP_GAIN_PROC,
			&amsg, sizeof(amsg),&arep, sizeof(arep), 5 * HZ);
		if (rc < 0){
			printk(KERN_ERR "%s:rpc err because of %d\n", __func__, rc);
		}
		else
		{
			ampgain.get_param = be32_to_cpu(arep.get_gainvalue);
			printk(KERN_INFO "%s:rx vol ->%d\n", __func__, ampgain.get_param);
			if (copy_to_user((void __user *)arg, &ampgain, sizeof(ampgain))) {
				pr_err("snd_ioctl get pad value: invalid write pointer.\n");
				rc = -EFAULT;
			}
		}
			
		if (copy_to_user((void __user *)arg, &ampgain, sizeof(ampgain))) {
			pr_err("snd_ioctl get amp gain: invalid write pointer.\n");
			rc = -EFAULT;
		}
		break;
    }
	case SND_WRITE_EFS:
		wmsg.args.cb_func = -1;
		wmsg.args.client_data = 0;
		pr_info("set_write_efs \n");

		rc = msm_rpc_call_reply(snd->ept,
			SND_WRITE_EFS_PROC,
			&wmsg, sizeof(wmsg),&wrep, sizeof(wrep), 5 * HZ);
		if (rc < 0){
			printk(KERN_ERR "%s:rpc err because of %d\n", __func__, rc);
		} 
		else
		{
			wefs = be32_to_cpu(wrep.result);
			printk(KERN_INFO "%s:loopback mode ->%d\n", __func__, wefs);
			if (copy_to_user((void __user *)arg, &wefs, sizeof(wefs))) {
				pr_err("snd_ioctl write efs: invalid write pointer.\n");
				rc = -EFAULT;
			}
		}
		break;
	case SND_WRITE_MEM:	
		wmsg.args.cb_func = -1;
		wmsg.args.client_data = 0;
		pr_info("set_write_efs \n");

		rc = msm_rpc_call_reply(snd->ept,
			SND_WRITE_MEM_PROC,
			&wmsg, sizeof(wmsg),&wrep, sizeof(wrep), 5 * HZ);
		if (rc < 0){
			printk(KERN_ERR "%s:rpc err because of %d\n", __func__, rc);
		} 
		else
		{
			wefs = be32_to_cpu(wrep.result);
			printk(KERN_INFO "%s:loopback mode ->%d\n", __func__, wefs);
			if (copy_to_user((void __user *)arg, &wefs, sizeof(wefs))) {
				pr_err("snd_ioctl write efs: invalid write pointer.\n");
				rc = -EFAULT;
			}
		}
		break;
	case SND_SET_MICAMP_GAIN:
    {
        struct msm_snd_set_micamp_gain_param micampgain;
        struct snd_set_micamp_gain_param_msg mamsg;

		if (copy_from_user(&micampgain, (void __user *) arg, sizeof(micampgain))) {
			pr_err("snd_ioctl set_pad_value: invalid pointer.\n");
			rc = -EFAULT;
			break;
		}
		mamsg.args.voc_codec = cpu_to_be32(micampgain.voc_codec);
		mamsg.args.mic_channel = cpu_to_be32(micampgain.mic_channel);
		mamsg.args.get_flag = cpu_to_be32(micampgain.get_flag);
		mamsg.args.get_param = cpu_to_be32(micampgain.value);
		mamsg.args.cb_func = -1;
		mamsg.args.client_data = 0;
		pr_info("SND_SET_MICAMP_GAIN %d %d %d %d\n", micampgain.voc_codec,
						 micampgain.mic_channel, micampgain.get_flag, micampgain.get_param);

		rc = msm_rpc_call_reply(snd->ept,
			SND_SET_MICAMP_GAIN_PROC,
			&mamsg, sizeof(mamsg),&mrep, sizeof(mrep), 5 * HZ);
		if (rc < 0){
			printk(KERN_ERR "%s:rpc err because of %d\n", __func__, rc);
		}
		else
		{
			micampgain.get_param = be32_to_cpu(mrep.get_gainvalue);
			printk(KERN_INFO "%s:rx vol ->%d\n", __func__, micampgain.get_param);
			if (copy_to_user((void __user *)arg, &micampgain, sizeof(micampgain))) {
				pr_err("snd_ioctl get pad value: invalid write pointer.\n");
				rc = -EFAULT;
			}
		}
		break;
    }
	case SND_SET_FM_RADIO_VOLUME:
		if (copy_from_user(&fmradiovol, (void __user *) arg, sizeof(fmradiovol))) {
			pr_err("snd_ioctl set amp_gain: invalid pointer.\n");
			rc = -EFAULT;
			break;
		}
		fmrmsg.args.volume = cpu_to_be32(fmradiovol.volume);
		fmrmsg.args.cb_func = -1;
		fmrmsg.args.client_data = 0;

		pr_info("snd_set_fm_radio_volume %d\n", fmradiovol.volume);

		rc = msm_rpc_call(snd->ept,
			SND_SET_FM_RADIO_VOLUME_PROC,
			&fmrmsg, sizeof(fmrmsg), 5 * HZ);
		break;
	/*case SND_SET_LOOPBACK_MODE_AT:
		if (copy_from_user(&loopback, (void __user *) arg, sizeof(loopback))) {
			pr_err("snd_ioctl set amp_gain: invalid pointer.\n");
			rc = -EFAULT;
			break;
		}
		lmsg.args.mode = cpu_to_be32(loopback.mode);
		lmsg.args.cb_func = -1;
		lmsg.args.client_data = 0;
		pr_info("set_loopback_mode %d \n", loopback.mode);
		if(loopback.mode == 1)
		{
			loopback_at = true;
		}
		else
		{
			loopback_at = false;
		}
		rc = msm_rpc_call(snd->ept,
			SND_SET_LOOPBACK_MODE_PROC,
			&lmsg, sizeof(lmsg), 5 * HZ);
		if (rc < 0){
			printk(KERN_ERR "%s:rpc err because of %d\n", __func__, rc);
		}
		else
		{
			loopback.get_param = be32_to_cpu(lrep.get_mode);
			printk(KERN_INFO "%s:loopback mode ->%d\n", __func__, loopback.get_param);
			if (copy_to_user((void __user *)arg, &loopback, sizeof(loopback))) {
				pr_err("snd_ioctl get loopback mode: invalid write pointer.\n");
				rc = -EFAULT;
			}
		}
		break;*/
	case SND_GET_DEVICE:
		get_device = atomic_read(&current_snd_device);
		printk(KERN_INFO "%s: get_device %x\n", __func__, get_device);
		if (copy_to_user((void __user *)arg, &get_device, sizeof(get_device))) {
			pr_err("snd_ioctl get device.\n");
			rc = -EFAULT;
		}
		break;
	case SND_SET_VOICE_CLARITY:
		if (copy_from_user(&voiceclarity, (void __user *) arg, sizeof(voiceclarity))) {
			pr_err("snd_ioctl set amp_gain: invalid pointer.\n");
			rc = -EFAULT;
			break;
		}
		vcmsg.args.mode = cpu_to_be32(voiceclarity.mode);
		vcmsg.args.cb_func = -1;
		vcmsg.args.client_data = 0;
		pr_info("set_loopback_mode %d \n", voiceclarity.mode);
					
		rc = msm_rpc_call(snd->ept,
			SND_SET_VOICE_CLARITY_PROC,
			&vcmsg, sizeof(vcmsg), 5 * HZ);
		if (rc < 0){
			printk(KERN_ERR "%s:rpc err because of %d\n", __func__, rc);
		}
		else
		{
			voiceclarity.get_param = be32_to_cpu(vcrep.get_mode);
			printk(KERN_INFO "%s:voice clarity mode ->%d\n", __func__, voiceclarity.get_param);
			if (copy_to_user((void __user *)arg, &voiceclarity, sizeof(voiceclarity))) {
				pr_err("snd_ioctl get loopback mode: invalid write pointer.\n");
				rc = -EFAULT;
			}
		}
		break;
	case SND_SET_POWER_OFF:
		wmsg.args.cb_func = -1;
		wmsg.args.client_data = 0;
		pr_info("set_power_off \n");

		rc = msm_rpc_call_reply(snd->ept,
			SND_SET_POWER_OFF_PROC,
			&wmsg, sizeof(wmsg),&wrep, sizeof(wrep), 5 * HZ);
		if (rc < 0){
			printk(KERN_ERR "%s:rpc err because of %d\n", __func__, rc);
		} 
		else
		{
			wefs = be32_to_cpu(wrep.result);
			if (copy_to_user((void __user *)arg, &wefs, sizeof(wefs))) {
				pr_err("snd_ioctl write efs: invalid write pointer.\n");
				rc = -EFAULT;
			}
		}
		break;
		
	case SND_SET_FM_RADIO_MULTI_SOUND:
		wmsg.args.cb_func = -1;
		wmsg.args.client_data = 0;
		pr_info("set_fm_radio_multi_sound \n");

		rc = msm_rpc_call_reply(snd->ept,
			SND_SET_FM_RADIO_MULTI_SOUND_PROC,
			&wmsg, sizeof(wmsg),&wrep, sizeof(wrep), 5 * HZ);
		if (rc < 0){
			printk(KERN_ERR "%s:rpc err because of %d\n", __func__, rc);
		} 
		else
		{
			wefs = be32_to_cpu(wrep.result);
			if (copy_to_user((void __user *)arg, &wefs, sizeof(wefs))) {
				pr_err("snd_ioctl set_fm_radio_multi_sound: invalid write pointer.\n");
				rc = -EFAULT;
			}
		}
		break;
#endif
	default:
		MM_ERR("unknown command\n");
		rc = -EINVAL;
		break;
	}
	mutex_unlock(&snd->lock);

	return rc;
}

static int snd_release(struct inode *inode, struct file *file)
{
	struct snd_ctxt *snd = file->private_data;
	int rc;

	mutex_lock(&snd->lock);
	rc = msm_rpc_close(snd->ept);
	if (rc < 0)
		MM_ERR("msm_rpc_close failed\n");
	snd->ept = NULL;
	snd->opened = 0;
	mutex_unlock(&snd->lock);
	return 0;
}
static int snd_sys_release(void)
{
	struct snd_sys_ctxt *snd_sys = &the_snd_sys;
	int rc = 0;

	mutex_lock(&snd_sys->lock);
	rc = msm_rpc_close(snd_sys->ept);
	if (rc < 0)
		MM_ERR("msm_rpc_close failed\n");
	snd_sys->ept = NULL;
	mutex_unlock(&snd_sys->lock);
	return rc;
}
static int snd_open(struct inode *inode, struct file *file)
{
	struct snd_ctxt *snd = &the_snd;
	int rc = 0;

	mutex_lock(&snd->lock);
	if (snd->opened == 0) {
		if (snd->ept == NULL) {
			snd->ept = msm_rpc_connect_compatible(RPC_SND_PROG,
					RPC_SND_VERS, 0);
			if (IS_ERR(snd->ept)) {
				rc = PTR_ERR(snd->ept);
				snd->ept = NULL;
				MM_ERR("failed to connect snd svc\n");
				goto err;
			}
		}
		file->private_data = snd;
		snd->opened = 1;
	} else {
		MM_ERR("snd already opened\n");
		rc = -EBUSY;
	}

err:
	mutex_unlock(&snd->lock);
	return rc;
}
static int snd_sys_open(void)
{
	struct snd_sys_ctxt *snd_sys = &the_snd_sys;
	int rc = 0;

	mutex_lock(&snd_sys->lock);
	if (snd_sys->ept == NULL) {
		snd_sys->ept = msm_rpc_connect_compatible(RPC_SND_PROG,
			RPC_SND_VERS, 0);
		if (IS_ERR(snd_sys->ept)) {
			rc = PTR_ERR(snd_sys->ept);
			snd_sys->ept = NULL;
			MM_ERR("failed to connect snd svc\n");
			goto err;
		}
	} else
		MM_DBG("snd already opened\n");

err:
	mutex_unlock(&snd_sys->lock);
	return rc;
}

static struct file_operations snd_fops = {
	.owner		= THIS_MODULE,
	.open		= snd_open,
	.release	= snd_release,
	.unlocked_ioctl	= snd_ioctl,
};

struct miscdevice snd_misc = {
	.minor	= MISC_DYNAMIC_MINOR,
	.name	= "msm_snd",
	.fops	= &snd_fops,
};

static long snd_agc_enable(unsigned long arg)
{
	struct snd_sys_ctxt *snd_sys = &the_snd_sys;
	struct snd_agc_ctl_msg agc_msg;
	int rc = 0;

	if ((arg != 1) && (arg != 0))
		return -EINVAL;

	agc_msg.args.agc_ctl = cpu_to_be32(arg);
	agc_msg.args.cb_func = -1;
	agc_msg.args.client_data = 0;

	MM_DBG("snd_agc_ctl %ld,%d\n", arg, agc_msg.args.agc_ctl);

	rc = msm_rpc_call(snd_sys->ept,
		SND_AGC_CTL_PROC,
		&agc_msg, sizeof(agc_msg), 5 * HZ);
	return rc;
}

static long snd_avc_enable(unsigned long arg)
{
	struct snd_sys_ctxt *snd_sys = &the_snd_sys;
	struct snd_avc_ctl_msg avc_msg;
	int rc = 0;

	if ((arg != 1) && (arg != 0))
		return -EINVAL;

	avc_msg.args.avc_ctl = cpu_to_be32(arg);

	avc_msg.args.cb_func = -1;
	avc_msg.args.client_data = 0;

	MM_DBG("snd_avc_ctl %ld,%d\n", arg, avc_msg.args.avc_ctl);

	rc = msm_rpc_call(snd_sys->ept,
		SND_AVC_CTL_PROC,
		&avc_msg, sizeof(avc_msg), 5 * HZ);
	return rc;
}

static ssize_t snd_agc_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	ssize_t status;
	struct snd_sys_ctxt *snd_sys = &the_snd_sys;
	int rc = 0;

	rc = snd_sys_open();
	if (rc)
		return rc;

	mutex_lock(&snd_sys->lock);

	if (sysfs_streq(buf, "enable"))
		status = snd_agc_enable(1);
	else if (sysfs_streq(buf, "disable"))
		status = snd_agc_enable(0);
	else
		status = -EINVAL;

	mutex_unlock(&snd_sys->lock);
	rc = snd_sys_release();
	if (rc)
		return rc;

	return status ? : size;
}

static ssize_t snd_avc_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	ssize_t status;
	struct snd_sys_ctxt *snd_sys = &the_snd_sys;
	int rc = 0;

	rc = snd_sys_open();
	if (rc)
		return rc;

	mutex_lock(&snd_sys->lock);

	if (sysfs_streq(buf, "enable"))
		status = snd_avc_enable(1);
	else if (sysfs_streq(buf, "disable"))
		status = snd_avc_enable(0);
	else
		status = -EINVAL;

	mutex_unlock(&snd_sys->lock);
	rc = snd_sys_release();
	if (rc)
		return rc;

	return status ? : size;
}

static long snd_vol_enable(const char *arg)
{
	struct snd_sys_ctxt *snd_sys = &the_snd_sys;
	struct snd_set_volume_msg vmsg;
	struct msm_snd_volume_config vol;
	int rc = 0;

	rc = sscanf(arg, "%d %d %d", &vol.device, &vol.method, &vol.volume);
	if (rc != 3) {
		MM_ERR("Invalid arguments. Usage: <device> <method> \
				<volume>\n");
		rc = -EINVAL;
		return rc;
	}

	vmsg.args.device = cpu_to_be32(vol.device);
	vmsg.args.method = cpu_to_be32(vol.method);
	if (vol.method != SND_METHOD_VOICE) {
		MM_ERR("snd_ioctl set volume: invalid method\n");
		rc = -EINVAL;
		return rc;
	}

	vmsg.args.volume = cpu_to_be32(vol.volume);
	vmsg.args.cb_func = -1;
	vmsg.args.client_data = 0;

	MM_DBG("snd_set_volume %d %d %d\n", vol.device, vol.method,
			vol.volume);

	rc = msm_rpc_call(snd_sys->ept,
		SND_SET_VOLUME_PROC,
		&vmsg, sizeof(vmsg), 5 * HZ);
	return rc;
}

static long snd_dev_enable(const char *arg)
{
	struct snd_sys_ctxt *snd_sys = &the_snd_sys;
	struct snd_set_device_msg dmsg;
	struct msm_snd_device_config dev;
	int rc = 0;

	rc = sscanf(arg, "%d %d %d", &dev.device, &dev.ear_mute, &dev.mic_mute);
	if (rc != 3) {
		MM_ERR("Invalid arguments. Usage: <device> <ear_mute> \
				<mic_mute>\n");
		rc = -EINVAL;
		return rc;
	}
	dmsg.args.device = cpu_to_be32(dev.device);
	dmsg.args.ear_mute = cpu_to_be32(dev.ear_mute);
	dmsg.args.mic_mute = cpu_to_be32(dev.mic_mute);
	if (check_mute(dev.ear_mute) < 0 ||
			check_mute(dev.mic_mute) < 0) {
		MM_ERR("snd_ioctl set device: invalid mute status\n");
		rc = -EINVAL;
		return rc;
	}
	dmsg.args.cb_func = -1;
	dmsg.args.client_data = 0;

	MM_INFO("snd_set_device %d %d %d\n", dev.device, dev.ear_mute,
			dev.mic_mute);

	rc = msm_rpc_call(snd_sys->ept,
		SND_SET_DEVICE_PROC,
		&dmsg, sizeof(dmsg), 5 * HZ);
	return rc;
}

static ssize_t snd_dev_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	ssize_t status;
	struct snd_sys_ctxt *snd_sys = &the_snd_sys;
	int rc = 0;

	rc = snd_sys_open();
	if (rc)
		return rc;

	mutex_lock(&snd_sys->lock);
	status = snd_dev_enable(buf);
	mutex_unlock(&snd_sys->lock);

	rc = snd_sys_release();
	if (rc)
		return rc;

	return status ? : size;
}

static ssize_t snd_vol_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	ssize_t status;
	struct snd_sys_ctxt *snd_sys = &the_snd_sys;
	int rc = 0;

	rc = snd_sys_open();
	if (rc)
		return rc;

	mutex_lock(&snd_sys->lock);
	status = snd_vol_enable(buf);
	mutex_unlock(&snd_sys->lock);

	rc = snd_sys_release();
	if (rc)
		return rc;

	return status ? : size;
}

static DEVICE_ATTR(agc, S_IWUSR | S_IRUGO,
		NULL, snd_agc_store);

static DEVICE_ATTR(avc, S_IWUSR | S_IRUGO,
		NULL, snd_avc_store);

static DEVICE_ATTR(device, S_IWUSR | S_IRUGO,
		NULL, snd_dev_store);

static DEVICE_ATTR(volume, S_IWUSR | S_IRUGO,
		NULL, snd_vol_store);

static int snd_probe(struct platform_device *pdev)
{
	struct snd_ctxt *snd = &the_snd;
	struct snd_sys_ctxt *snd_sys = &the_snd_sys;
	int rc = 0;

	mutex_init(&snd->lock);
	mutex_init(&snd_sys->lock);
	snd_sys->ept = NULL;
	snd->snd_epts = (struct msm_snd_endpoints *)pdev->dev.platform_data;
	rc = misc_register(&snd_misc);
	if (rc)
		return rc;

	rc = device_create_file(snd_misc.this_device, &dev_attr_agc);
	if (rc) {
		misc_deregister(&snd_misc);
		return rc;
	}

	rc = device_create_file(snd_misc.this_device, &dev_attr_avc);
	if (rc) {
		device_remove_file(snd_misc.this_device,
						&dev_attr_agc);
		misc_deregister(&snd_misc);
		return rc;
	}

	rc = device_create_file(snd_misc.this_device, &dev_attr_device);
	if (rc) {
		device_remove_file(snd_misc.this_device,
						&dev_attr_agc);
		device_remove_file(snd_misc.this_device,
						&dev_attr_avc);
		misc_deregister(&snd_misc);
		return rc;
	}

	rc = device_create_file(snd_misc.this_device, &dev_attr_volume);
	if (rc) {
		device_remove_file(snd_misc.this_device,
						&dev_attr_agc);
		device_remove_file(snd_misc.this_device,
						&dev_attr_avc);
		device_remove_file(snd_misc.this_device,
						&dev_attr_device);
		misc_deregister(&snd_misc);
	}

	return rc;
}

static struct platform_driver snd_plat_driver = {
	.probe = snd_probe,
	.driver = {
		.name = "msm_snd",
		.owner = THIS_MODULE,
	},
};

static int __init snd_init(void)
{
	return platform_driver_register(&snd_plat_driver);
}

module_init(snd_init);
