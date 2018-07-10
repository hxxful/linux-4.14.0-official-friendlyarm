/*
 * A V4L2 driver for IMX219 cameras.
 *
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/videodev2.h>
#include <linux/clk.h>
#include <media/v4l2-device.h>
#include <media/v4l2-mediabus.h>
#include <linux/io.h>
#include "camera.h"
#include "sensor_helper.h"

MODULE_AUTHOR("Chomoly");
MODULE_DESCRIPTION("A low-level driver for IMX219 sensors");
MODULE_LICENSE("GPL");

/* for internal driver debug */
#define DEV_DBG_EN      0
#if (DEV_DBG_EN == 1)
#define vfe_dev_dbg(x, arg...) pr_debug("[IMX219]"x, ##arg)
#else
#define vfe_dev_dbg(x, arg...)
#endif
#define vfe_dev_err(x, arg...) pr_err("[IMX219]"x, ##arg)
#define vfe_dev_print(x, arg...) pr_info("[IMX219]"x, ##arg)

/*
 *#define LOG_ERR_RET(x) { \
 *			  int ret; \
 *			  ret = x; \
 *			  if (ret < 0) { \
 *			    vfe_dev_err("error at %s\n", __func__); \
 *			    return ret; \
 *			  } \
 *			}
 */

/* define module timing */
#define MCLK              (24*1000*1000)
#define VREF_POL          V4L2_MBUS_VSYNC_ACTIVE_LOW
#define HREF_POL          V4L2_MBUS_HSYNC_ACTIVE_HIGH
#define CLK_POL           V4L2_MBUS_PCLK_SAMPLE_RISING
#define V4L2_IDENT_SENSOR 0x0219


/*
 * Our nominal (default) frame rate.
 */
#ifdef FPGA
#define SENSOR_FRAME_RATE 15
#else
#define SENSOR_FRAME_RATE 30
#endif

/*
 * The IMX219 sits on i2c with ID 0x6c
 */
#define I2C_ADDR 0x20
#define SENSOR_NAME "imx219"
int imx219_sensor_vts;
#define ES_GAIN(a, b, c) ((unsigned short)(a*160) < (c*10) && (c*10) <= (unsigned short)(b*160))



/* static struct delayed_work sensor_s_ae_ratio_work; */
static struct v4l2_subdev *glb_sd;

/*
 * Information we maintain about a known sensor.
 */
struct sensor_format_struct;  /* coming later */

struct cfg_array { /* coming later */
	struct regval_list *regs;
	int size;
};

static int LOG_ERR_RET(int x)
{
	int ret;

	ret = x;
	if (ret < 0)
		vfe_dev_err("error at %s\n", __func__);
	return ret;
}

static inline struct sensor_info *to_state(struct v4l2_subdev *sd)
{
	return container_of(sd, struct sensor_info, sd);
}


/*
 * The default register settings
 *
 */

/*3280 x 2464_20fps 4lanes 720Mbps/lane*/
static struct regval_list sensor_default_regs[] = {

};

/* for capture */
static struct regval_list sensor_hxga_regs[] = { /* 3280 * 2464 20fps 4lane */

	{0x30EB, 0x05},
	{0x30EB, 0x0C},
	{0x300A, 0xFF},
	{0x300B, 0xFF},
	{0x30EB, 0x05},
	{0x30EB, 0x09},
	{0x0114, 0x03},
	{0x0128, 0x00},
	{0x012A, 0x18},
	{0x012B, 0x00},


	{0x0160, 0x0F},
	{0x0161, 0xC5},
	{0x0162, 0x0D},
	{0x0163, 0x78},

	{0x0164, 0x00},
	{0x0165, 0x00},
	{0x0166, 0x0C},
	{0x0167, 0xCF},
	{0x0168, 0x00},
	{0x0169, 0x00},
	{0x016A, 0x09},
	{0x016B, 0x9F},
	{0x016C, 0x0C},
	{0x016D, 0xD0},
	{0x016E, 0x09},
	{0x016F, 0xA0},
	{0x0170, 0x01},
	{0x0171, 0x01},
	{0x0174, 0x00},
	{0x0175, 0x00},
	{0x018C, 0x0A},
	{0x018D, 0x0A},
	{0x0301, 0x05},
	{0x0303, 0x01},
	{0x0304, 0x03},
	{0x0305, 0x03},
	{0x0306, 0x00},
	{0x0307, 0x57},
	{0x0309, 0x0A},
	{0x030B, 0x01},
	{0x030C, 0x00},
	{0x030D, 0x5A},
	{0x4767, 0x0F},
	{0x4750, 0x14},
	{0x47B4, 0x14},

	{0x0100, 0x01},

};


static struct regval_list sensor_sxga_regs[] = { /* SXGA: 1280*960@30fps */
	/* 720Mbps 4lane 20fps */
	{0x30EB, 0x05},
	{0x30EB, 0x0C},
	{0x300A, 0xFF},
	{0x300B, 0xFF},
	{0x30EB, 0x05},
	{0x30EB, 0x09},
	{0x0114, 0x03},
	{0x0128, 0x00},
	{0x012A, 0x18},
	{0x012B, 0x00},
	{0x0160, 0x0a},
	{0x0161, 0x2f},
	{0x0162, 0x0d},
	{0x0163, 0xe8},
	{0x0164, 0x03},
	{0x0165, 0xe8},
	{0x0166, 0x08},
	{0x0167, 0xe7},
	{0x0168, 0x02},
	{0x0169, 0xf0},
	{0x016A, 0x06},
	{0x016B, 0xaF},
	{0x016C, 0x05},
	{0x016D, 0x00},
	{0x016E, 0x03},
	{0x016F, 0xc0},

	/* {0x016C, 0x0c}, */
	/* {0x016D, 0xd0}, */
	/* {0x016E, 0x09}, */
	/* {0x016F, 0xa0}, */
	{0x0170, 0x01},
	{0x0171, 0x01},
	{0x0174, 0x00},
	{0x0175, 0x00},
	{0x018C, 0x0A},
	{0x018D, 0x0A},
	{0x0301, 0x05},
	{0x0303, 0x01},
	{0x0304, 0x03},
	{0x0305, 0x03},
	{0x0306, 0x00},
	{0x0307, 0x57},
	{0x0309, 0x0A},
	{0x030B, 0x01},
	{0x030C, 0x00},
	{0x030D, 0x5A},
	{0x4767, 0x0F},
	{0x4750, 0x14},
	{0x47B4, 0x14},
	{0x0100, 0x01},
};

/* for video */
static struct regval_list sensor_1080p_regs[] = { /* 1080: 1920*1080@30fps */
	/* MIPI=720Mbps */
	/* 1920x1080 30fps */
	{0x30EB, 0x05},
	{0x30EB, 0x0C},
	{0x300A, 0xFF},
	{0x300B, 0xFF},
	{0x30EB, 0x05},
	{0x30EB, 0x09},
	/* {0x  , 0x   }, */
	{0x0114, 0x03},
	{0x0128, 0x00},
	{0x012A, 0x18},
	{0x012B, 0x00},
	/* {0x0157, 0x  }, */
	/* {0x015A, 0x  }, */
	/* {0x015B, 0x  }, */
	{0x0160, 0x0A},
	{0x0161, 0x2F},
	{0x0162, 0x0D},
	{0x0163, 0xE8},
	{0x0164, 0x02},
	{0x0165, 0xA8},
	{0x0166, 0x0A},
	{0x0167, 0x27},
	{0x0168, 0x02},
	{0x0169, 0xB4},
	{0x016A, 0x06},
	{0x016B, 0xEB},
	{0x016C, 0x07},
	{0x016D, 0x80},
	{0x016E, 0x04},
	{0x016F, 0x38},
	{0x0170, 0x01},
	{0x0171, 0x01},
	{0x0174, 0x00},
	{0x0175, 0x00},
	{0x018C, 0x0A},
	{0x018D, 0x0A},
	{0x0301, 0x05},
	{0x0303, 0x01},
	{0x0304, 0x03},
	{0x0305, 0x03},
	{0x0306, 0x00},
	{0x0307, 0x57},
	{0x0309, 0x0A},
	{0x030B, 0x01},
	{0x030C, 0x00},
	{0x030D, 0x5A},
	{0x4767, 0x0F},
	{0x4750, 0x14},
	{0x47B4, 0x14},
	{0x0100, 0x01},
};



static struct regval_list sensor_720p_regs[] = { /* 720: 1280*720@30fps */
	/* MIPI=720Mbps, */
	/* 1280x720 60fps //perhaps reach to 100~120fps */
	{0x30EB, 0x05},
	{0x30EB, 0x0C},
	{0x300A, 0xFF},
	{0x300B, 0xFF},
	{0x30EB, 0x05},
	{0x30EB, 0x09},
	{0x0114, 0x03},
	{0x0128, 0x00},
	{0x012A, 0x18},
	{0x012B, 0x00},
	{0x0160, 0x02}, /* 0x05 for 60fps */
	{0x0161, 0x00}, /* 0x17 for 60fps */
	{0x0162, 0x0d}, /*0D*/
	{0x0163, 0xE8},
	{0x0164, 0x03},
	{0x0165, 0xE8},
	{0x0166, 0x08},
	{0x0167, 0xE7},
	{0x0168, 0x03},
	{0x0169, 0x68},
	{0x016A, 0x06},
	{0x016B, 0x37},
	{0x016C, 0x05},
	{0x016D, 0x00},
	{0x016E, 0x02},
	{0x016F, 0xD0},
	{0x0170, 0x01},
	{0x0171, 0x01},
	{0x0174, 0x00},
	{0x0175, 0x00},
	{0x018C, 0x0A},
	{0x018D, 0x0A},
	{0x0301, 0x05},
	{0x0303, 0x01},
	{0x0304, 0x03},
	{0x0305, 0x03},
	{0x0306, 0x00},
	{0x0307, 0x57},
	{0x0309, 0x05}, /* 0A */
	{0x030B, 0x01},
	{0x030C, 0x00},
	{0x030D, 0x5A},
	{0x4767, 0x0F},
	{0x4750, 0x14},
	{0x47B4, 0x14},
	{0x0100, 0x01},
};

/*
 *static struct regval_list sensor_vga_regs[] = { //VGA:  640*480
 *};
 */

/* misc */
/*
 *static struct regval_list sensor_oe_disable_regs[] = {
 *};
 */

/*
 *static struct regval_list sensor_oe_enable_regs[] = {
 *};
 */


/*
 * Here we'll try to encapsulate the changes for just the output
 * video format.
 *
 */

static struct regval_list sensor_fmt_raw[] = {

};


static int sensor_s_exp_gain(struct v4l2_subdev *sd,
				struct sensor_exp_gain *exp_gain)
{ /* return -1; */
	int exp_val, gain_val, frame_length, shutter;
	unsigned char explow = 0, exphigh = 0; /* expmid = 0; */
	/* unsigned char gainlow = 0, gainhigh = 0; */
	struct sensor_info *info = to_state(sd);

	exp_val = exp_gain->exp_val;
	gain_val = exp_gain->gain_val;

	/* if((info->exp == exp_val)&&(info->gain == gain_val)) */
	/* return 0; */
	if (gain_val < 1*16)
		gain_val = 16;
	if (gain_val > 10*16-1)
		gain_val = 10*16-1;

	if (exp_val > 0xfffff)
		exp_val = 0xfffff;

	exp_val >>= 4;
	exphigh  = (unsigned char) ((0x00ff00&exp_val)>>8);
	explow   = (unsigned char) ((0x0000ff&exp_val));

	sensor_write(sd, 0x015b, explow);
	sensor_write(sd, 0x015a, exphigh);

	shutter = exp_val;

	if (shutter  > imx219_sensor_vts - 4)
		frame_length = shutter + 4;
	else
		frame_length = imx219_sensor_vts;
	sensor_write(sd, 0x0161, frame_length & 0xff);

	sensor_write(sd, 0x0160, frame_length >> 8);
	if (gain_val == 16)
		sensor_write(sd, 0x0157, 0x01);
	if (ES_GAIN(1.0, 1.1, gain_val))
		sensor_write(sd, 0x0157, 24);
	else if (ES_GAIN(1.1, 1.2, gain_val))
		sensor_write(sd, 0x0157, 42);
	else if (ES_GAIN(1.2, 1.3, gain_val))
		sensor_write(sd, 0x0157, 60);
	else if (ES_GAIN(1.3, 1.4, gain_val))
		sensor_write(sd, 0x0157, 73);
	else if (ES_GAIN(1.4, 1.5, gain_val))
		sensor_write(sd, 0x0157, 85);
	else if (ES_GAIN(1.5, 1.6, gain_val))
		sensor_write(sd, 0x0157, 96);
	else if (ES_GAIN(1.6, 1.7, gain_val))
		sensor_write(sd, 0x0157, 105);
	else if (ES_GAIN(1.7, 1.8, gain_val))
		sensor_write(sd, 0x0157, 114);
	else if (ES_GAIN(1.8, 1.9, gain_val))
		sensor_write(sd, 0x0157, 122);
	else if (ES_GAIN(1.9, 2.0, gain_val))
		sensor_write(sd, 0x0157, 0x80);
	else if (ES_GAIN(2.0, 2.1, gain_val))
		sensor_write(sd, 0x0157, 134);
	else if (ES_GAIN(2.1, 2.2, gain_val))
		sensor_write(sd, 0x0157, 140);
	else if (ES_GAIN(2.2, 2.3, gain_val))
		sensor_write(sd, 0x0157, 145);
	else if (ES_GAIN(2.3, 2.4, gain_val))
		sensor_write(sd, 0x0157, 150);
	else if (ES_GAIN(2.4, 2.5, gain_val))
		sensor_write(sd, 0x0157, 154);
	else if (ES_GAIN(2.5, 2.6, gain_val))
		sensor_write(sd, 0x0157, 158);
	else if (ES_GAIN(2.6, 2.7, gain_val))
		sensor_write(sd, 0x0157, 162);
	else if (ES_GAIN(2.7, 2.8, gain_val))
		sensor_write(sd, 0x0157, 165);

	else if (ES_GAIN(2.8, 2.9, gain_val))
		sensor_write(sd, 0x0157, 168);
	else if (ES_GAIN(2.9, 3.0, gain_val))
		sensor_write(sd, 0x0157, 0xab);
	else if (ES_GAIN(3.0, 3.1, gain_val))
		sensor_write(sd, 0x0157, 174);
	else if (ES_GAIN(3.1, 3.2, gain_val))
		sensor_write(sd, 0x0157, 176);
	else if (ES_GAIN(3.2, 3.3, gain_val))
		sensor_write(sd, 0x0157, 179);
	else if (ES_GAIN(3.3, 3.4, gain_val))
		sensor_write(sd, 0x0157, 181);
	else if (ES_GAIN(3.4, 3.5, gain_val))
		sensor_write(sd, 0x0157, 183);
	else if (ES_GAIN(3.5, 3.6, gain_val))
		sensor_write(sd, 0x0157, 185);
	else if (ES_GAIN(3.6, 3.7, gain_val))
		sensor_write(sd, 0x0157, 187);
	else if (ES_GAIN(3.7, 3.8, gain_val))
		sensor_write(sd, 0x0157, 189);
	else if (ES_GAIN(3.8, 3.9, gain_val))
		sensor_write(sd, 0x0157, 191);
	else if (ES_GAIN(3.9, 4.0, gain_val))
		sensor_write(sd, 0x0157, 192);
	else if (ES_GAIN(4.0, 4.1, gain_val))
		sensor_write(sd, 0x0157, 194);
	else if (ES_GAIN(4.1, 4.2, gain_val))
		sensor_write(sd, 0x0157, 195);
	else if (ES_GAIN(4.2, 4.3, gain_val))
		sensor_write(sd, 0x0157, 197);
	else if (ES_GAIN(4.3, 4.4, gain_val))
		sensor_write(sd, 0x0157, 198);
	else if (ES_GAIN(4.4, 4.5, gain_val))
		sensor_write(sd, 0x0157, 200);
	else if (ES_GAIN(4.5, 4.6, gain_val))
		sensor_write(sd, 0x0157, 201);
	else if (ES_GAIN(4.6, 4.7, gain_val))
		sensor_write(sd, 0x0157, 202);
	else if (ES_GAIN(4.7, 4.8, gain_val))
		sensor_write(sd, 0x0157, 203);
	else if (ES_GAIN(4.8, 4.9, gain_val))
		sensor_write(sd, 0x0157, 204);
	else if (ES_GAIN(4.9, 5.0, gain_val))
		sensor_write(sd, 0x0157, 205);
	else if (ES_GAIN(5.0, 5.1, gain_val))
		sensor_write(sd, 0x0157, 206);
	else if (ES_GAIN(5.1, 5.2, gain_val))
		sensor_write(sd, 0x0157, 207);


	else if (ES_GAIN(5.2, 5.3, gain_val))
		sensor_write(sd, 0x0157, 208);
	else if (ES_GAIN(5.3, 5.4, gain_val))
		sensor_write(sd, 0x0157, 209);
	else if (ES_GAIN(5.4, 5.5, gain_val))
		sensor_write(sd, 0x0157, 210);
	else if (ES_GAIN(5.5, 5.7, gain_val))
		sensor_write(sd, 0x0157, 211);
	else if (ES_GAIN(5.7, 5.8, gain_val))
		sensor_write(sd, 0x0157, 212);
	else if (ES_GAIN(5.8, 5.9, gain_val))
		sensor_write(sd, 0x0157, 213);
	else if (ES_GAIN(5.9, 6.2, gain_val))
		sensor_write(sd, 0x0157, 215);
	else if (ES_GAIN(6.2, 6.4, gain_val))
		sensor_write(sd, 0x0157, 216);
	else if (ES_GAIN(6.4, 6.5, gain_val))
		sensor_write(sd, 0x0157, 217);
	else if (ES_GAIN(6.5, 6.7, gain_val))
		sensor_write(sd, 0x0157, 218);
	else if (ES_GAIN(6.7, 6.9, gain_val))
		sensor_write(sd, 0x0157, 219);
	else if (ES_GAIN(6.9, 7.1, gain_val))
		sensor_write(sd, 0x0157, 220);
	else if (ES_GAIN(7.1, 7.3, gain_val))
		sensor_write(sd, 0x0157, 221);
	else if (ES_GAIN(7.3, 7.5, gain_val))
		sensor_write(sd, 0x0157, 222);
	else if (ES_GAIN(7.5, 7.7, gain_val))
		sensor_write(sd, 0x0157, 223);
	else if (ES_GAIN(7.7, 8.0, gain_val))
		sensor_write(sd, 0x0157, 224);
	else if (ES_GAIN(8.0, 8.3, gain_val))
		sensor_write(sd, 0x0157, 225);
	else if (ES_GAIN(8.3, 8.5, gain_val))
		sensor_write(sd, 0x0157, 226);
	else if (ES_GAIN(8.5, 8.8, gain_val))
		sensor_write(sd, 0x0157, 227);
	else if (ES_GAIN(8.8, 9.1, gain_val))
		sensor_write(sd, 0x0157, 228);


	else if (ES_GAIN(9.1, 9.4, gain_val))
		sensor_write(sd, 0x0157, 228);
	else if (ES_GAIN(9.4, 9.8, gain_val))
		sensor_write(sd, 0x0157, 230);
	else if (ES_GAIN(9.8, 10.2, gain_val))
		sensor_write(sd, 0x0157, 231);
	else if (ES_GAIN(10.0, 10.6, gain_val))
		sensor_write(sd, 0x0157, 232);

	info->exp = exp_val;
	info->gain = gain_val;
	return 0;
}

static int sensor_g_exp(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);

	*value = info->exp;
	vfe_dev_dbg("sensor_get_exposure = %d\n", info->exp);
	return 0;
}

static int sensor_s_exp(struct v4l2_subdev *sd, unsigned int exp_val)
{
	unsigned char explow, exphigh;
	struct sensor_info *info = to_state(sd);

	if (exp_val > 0xfffff)
		exp_val = 0xfffff;
	exp_val >>= 4;
	exphigh = (unsigned char) ((0x00ff00&exp_val)>>8);
	explow = (unsigned char) ((0x0000ff&exp_val));
	sensor_write(sd, 0x015b, explow);
	sensor_write(sd, 0x015a, exphigh);

	info->exp = exp_val;
	return 0;
}

static int sensor_g_gain(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);

	*value = info->gain;
	vfe_dev_dbg("sensor_get_gain = %d\n", info->gain);
	return 0;
}

static int sensor_s_gain(struct v4l2_subdev *sd, int gain_val)
{
	struct sensor_info *info = to_state(sd);

	if (gain_val < 1*16)
		gain_val = 16;
	if (gain_val > 0x1ff)
		gain_val = 0x1ff;
	if (gain_val == 16)
		sensor_write(sd, 0x0157, 0x01);
	if (ES_GAIN(1.0, 1.1, gain_val))
		sensor_write(sd, 0x0157, 24);
	else if (ES_GAIN(1.1, 1.2, gain_val))
		sensor_write(sd, 0x0157, 42);
	else if (ES_GAIN(1.2, 1.3, gain_val))
		sensor_write(sd, 0x0157, 60);
	else if (ES_GAIN(1.3, 1.4, gain_val))
		sensor_write(sd, 0x0157, 73);
	else if (ES_GAIN(1.4, 1.5, gain_val))
		sensor_write(sd, 0x0157, 85);
	else if (ES_GAIN(1.5, 1.6, gain_val))
		sensor_write(sd, 0x0157, 96);
	else if (ES_GAIN(1.6, 1.7, gain_val))
		sensor_write(sd, 0x0157, 105);
	else if (ES_GAIN(1.7, 1.8, gain_val))
		sensor_write(sd, 0x0157, 114);
	else if (ES_GAIN(1.8, 1.9, gain_val))
		sensor_write(sd, 0x0157, 122);
	else if (ES_GAIN(1.9, 2.0, gain_val))
		sensor_write(sd, 0x0157, 0x80);
	else if (ES_GAIN(2.0, 2.1, gain_val))
		sensor_write(sd, 0x0157, 134);
	else if (ES_GAIN(2.1, 2.2, gain_val))
		sensor_write(sd, 0x0157, 140);
	else if (ES_GAIN(2.2, 2.3, gain_val))
		sensor_write(sd, 0x0157, 145);
	else if (ES_GAIN(2.3, 2.4, gain_val))
		sensor_write(sd, 0x0157, 150);
	else if (ES_GAIN(2.4, 2.5, gain_val))
		sensor_write(sd, 0x0157, 154);
	else if (ES_GAIN(2.5, 2.6, gain_val))
		sensor_write(sd, 0x0157, 158);
	else if (ES_GAIN(2.6, 2.7, gain_val))
		sensor_write(sd, 0x0157, 162);
	else if (ES_GAIN(2.7, 2.8, gain_val))
		sensor_write(sd, 0x0157, 165);

	else if (ES_GAIN(2.8, 2.9, gain_val))
		sensor_write(sd, 0x0157, 168);
	else if (ES_GAIN(2.9, 3.0, gain_val))
		sensor_write(sd, 0x0157, 0xab);
	else if (ES_GAIN(3.0, 3.1, gain_val))
		sensor_write(sd, 0x0157, 174);
	else if (ES_GAIN(3.1, 3.2, gain_val))
		sensor_write(sd, 0x0157, 176);
	else if (ES_GAIN(3.2, 3.3, gain_val))
		sensor_write(sd, 0x0157, 179);
	else if (ES_GAIN(3.3, 3.4, gain_val))
		sensor_write(sd, 0x0157, 181);
	else if (ES_GAIN(3.4, 3.5, gain_val))
		sensor_write(sd, 0x0157, 183);
	else if (ES_GAIN(3.5, 3.6, gain_val))
		sensor_write(sd, 0x0157, 185);
	else if (ES_GAIN(3.6, 3.7, gain_val))
		sensor_write(sd, 0x0157, 187);
	else if (ES_GAIN(3.7, 3.8, gain_val))
		sensor_write(sd, 0x0157, 189);
	else if (ES_GAIN(3.8, 3.9, gain_val))
		sensor_write(sd, 0x0157, 191);
	else if (ES_GAIN(3.9, 4.0, gain_val))
		sensor_write(sd, 0x0157, 192);
	else if (ES_GAIN(4.0, 4.1, gain_val))
		sensor_write(sd, 0x0157, 194);
	else if (ES_GAIN(4.1, 4.2, gain_val))
		sensor_write(sd, 0x0157, 195);
	else if (ES_GAIN(4.2, 4.3, gain_val))
		sensor_write(sd, 0x0157, 197);
	else if (ES_GAIN(4.3, 4.4, gain_val))
		sensor_write(sd, 0x0157, 198);
	else if (ES_GAIN(4.4, 4.5, gain_val))
		sensor_write(sd, 0x0157, 200);
	else if (ES_GAIN(4.5, 4.6, gain_val))
		sensor_write(sd, 0x0157, 201);
	else if (ES_GAIN(4.6, 4.7, gain_val))
		sensor_write(sd, 0x0157, 202);
	else if (ES_GAIN(4.7, 4.8, gain_val))
		sensor_write(sd, 0x0157, 203);
	else if (ES_GAIN(4.8, 4.9, gain_val))
		sensor_write(sd, 0x0157, 204);
	else if (ES_GAIN(4.9, 5.0, gain_val))
		sensor_write(sd, 0x0157, 205);
	else if (ES_GAIN(5.0, 5.1, gain_val))
		sensor_write(sd, 0x0157, 206);
	else if (ES_GAIN(5.1, 5.2, gain_val))
		sensor_write(sd, 0x0157, 207);

	else if (ES_GAIN(5.2, 5.3, gain_val))
		sensor_write(sd, 0x0157, 208);
	else if (ES_GAIN(5.3, 5.4, gain_val))
		sensor_write(sd, 0x0157, 209);
	else if (ES_GAIN(5.4, 5.5, gain_val))
		sensor_write(sd, 0x0157, 210);
	else if (ES_GAIN(5.5, 5.7, gain_val))
		sensor_write(sd, 0x0157, 211);
	else if (ES_GAIN(5.7, 5.8, gain_val))
		sensor_write(sd, 0x0157, 212);
	else if (ES_GAIN(5.8, 5.9, gain_val))
		sensor_write(sd, 0x0157, 213);
	else if (ES_GAIN(5.9, 6.2, gain_val))
		sensor_write(sd, 0x0157, 215);
	else if (ES_GAIN(6.2, 6.4, gain_val))
		sensor_write(sd, 0x0157, 216);
	else if (ES_GAIN(6.4, 6.5, gain_val))
		sensor_write(sd, 0x0157, 217);
	else if (ES_GAIN(6.5, 6.7, gain_val))
		sensor_write(sd, 0x0157, 218);
	else if (ES_GAIN(6.7, 6.9, gain_val))
		sensor_write(sd, 0x0157, 219);
	else if (ES_GAIN(6.9, 7.1, gain_val))
		sensor_write(sd, 0x0157, 220);
	else if (ES_GAIN(7.1, 7.3, gain_val))
		sensor_write(sd, 0x0157, 221);
	else if (ES_GAIN(7.3, 7.5, gain_val))
		sensor_write(sd, 0x0157, 222);
	else if (ES_GAIN(7.5, 7.7, gain_val))
		sensor_write(sd, 0x0157, 223);
	else if (ES_GAIN(7.7, 8.0, gain_val))
		sensor_write(sd, 0x0157, 224);
	else if (ES_GAIN(8.0, 8.3, gain_val))
		sensor_write(sd, 0x0157, 225);
	else if (ES_GAIN(8.3, 8.5, gain_val))
		sensor_write(sd, 0x0157, 226);
	else if (ES_GAIN(8.5, 8.8, gain_val))
		sensor_write(sd, 0x0157, 227);
	else if (ES_GAIN(8.8, 9.1, gain_val))
		sensor_write(sd, 0x0157, 228);


	else if (ES_GAIN(9.1, 9.4, gain_val))
		sensor_write(sd, 0x0157, 228);
	else if (ES_GAIN(9.4, 9.8, gain_val))
		sensor_write(sd, 0x0157, 230);
	else if (ES_GAIN(9.8, 10.2, gain_val))
		sensor_write(sd, 0x0157, 231);
	else if (ES_GAIN(10.0, 10.6, gain_val))
		sensor_write(sd, 0x0157, 232);
	info->gain = gain_val;

	return 0;
}

/*
 *static int sensor_s_sw_stby(struct v4l2_subdev *sd, int on_off)
 *{
 *	int ret;
 *	data_type rdval;
 *
 *	ret = sensor_read(sd, 0x0100, &rdval);
 *	if (ret != 0)
 *		return ret;
 *
 *	if (on_off == CSI_GPIO_LOW) //sw stby on
 *		ret = sensor_write(sd, 0x0100, rdval&0xfe);
 *	else                        //sw stby off
 *		ret = sensor_write(sd, 0x0100, rdval|0x01);
 *	return ret;
 *}
 */


/*
 * Stuff that knows about the sensor.
 */

static int sensor_power(struct v4l2_subdev *sd, int on)
{
	int ret;

	ret = 0;
	switch (on) {
	case CSI_SUBDEV_STBY_ON:
		vfe_dev_dbg("CSI_SUBDEV_STBY_ON!\n");
		cci_lock(sd);
		vfe_gpio_write(sd, RESET, CSI_GPIO_LOW);
		cci_unlock(sd);
		vfe_set_mclk(sd, OFF);
		break;
	case CSI_SUBDEV_STBY_OFF:
		vfe_dev_dbg("CSI_SUBDEV_STBY_OFF!\n");
		cci_lock(sd);
		vfe_set_mclk_freq(sd, MCLK);
		vfe_set_mclk(sd, ON);
		msleep(20);
		cci_unlock(sd);
		vfe_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		break;
	case CSI_SUBDEV_PWR_ON:
		vfe_dev_dbg("CSI_SUBDEV_PWR_ON!\n");
		cci_lock(sd);
		vfe_gpio_set_status(sd, PWDN, 1);  /* set the gpio to output */
		vfe_gpio_set_status(sd, RESET, 1); /* set the gpio to output */
		vfe_gpio_write(sd, PWDN, CSI_GPIO_LOW);
		vfe_gpio_write(sd, RESET, CSI_GPIO_LOW);
		usleep_range(1000, 1200);
		vfe_set_mclk_freq(sd, MCLK);
		vfe_set_mclk(sd, ON);
		usleep_range(10000, 12000);
		vfe_gpio_write(sd, POWER_EN, CSI_GPIO_HIGH);
		vfe_set_pmu_channel(sd, IOVDD, ON);
		vfe_set_pmu_channel(sd, AVDD, ON);
		vfe_set_pmu_channel(sd, DVDD, ON);
		vfe_set_pmu_channel(sd, AFVDD, ON);
		usleep_range(10000, 12000);
		vfe_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		vfe_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		usleep_range(30000, 31000);
		cci_unlock(sd);
		break;
	case CSI_SUBDEV_PWR_OFF:
		vfe_dev_dbg("CSI_SUBDEV_PWR_OFF!\n");
		cci_lock(sd);
		vfe_set_mclk(sd, OFF);
		vfe_gpio_write(sd, POWER_EN, CSI_GPIO_LOW);
		vfe_set_pmu_channel(sd, AFVDD, OFF);
		vfe_set_pmu_channel(sd, DVDD, OFF);
		vfe_set_pmu_channel(sd, AVDD, OFF);
		vfe_set_pmu_channel(sd, IOVDD, OFF);
		usleep_range(10000, 12000);
		vfe_gpio_write(sd, PWDN, CSI_GPIO_LOW);
		vfe_gpio_write(sd, RESET, CSI_GPIO_LOW);

		vfe_gpio_set_status(sd, RESET, 0); /* set the gpio to input */
		vfe_gpio_set_status(sd, PWDN, 0);  /* set the gpio to input */
		cci_unlock(sd);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int sensor_reset(struct v4l2_subdev *sd, u32 val)
{
	switch (val) {
	case 0:
		vfe_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		usleep_range(10000, 12000);
		break;
	case 1:
		vfe_gpio_write(sd, RESET, CSI_GPIO_LOW);
		usleep_range(10000, 12000);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sensor_detect(struct v4l2_subdev *sd)
{
	data_type rdval;

	LOG_ERR_RET(sensor_read(sd, 0x0000, &rdval));
	if ((rdval&0x0f) != 0x02)
		return -ENODEV;

	LOG_ERR_RET(sensor_read(sd, 0x0001, &rdval));
	if (rdval != 0x19)
		return -ENODEV;
	vfe_dev_print("find the sony IMX219 ***********\n");
	return 0;
}

static int sensor_init(struct v4l2_subdev *sd, u32 val)
{
	int ret;
	struct sensor_info *info = to_state(sd);

	vfe_dev_dbg("sensor_init\n");

	/*Make sure it is a target sensor*/
	ret = sensor_detect(sd);
	if (ret) {
		vfe_dev_err("chip found is not an target chip.\n");
		return ret;
	}

	vfe_get_standby_mode(sd, &info->stby_mode);

	if ((info->stby_mode == HW_STBY || info->stby_mode == SW_STBY)
			&& info->init_first_flag == 0) {
		vfe_dev_print("stby_mode and init_first_flag = 0\n");
		return 0;
	}

	info->focus_status = 0;
	info->low_speed = 0;
	info->width = HXGA_WIDTH;
	info->height = HXGA_HEIGHT;
	info->hflip = 0;
	info->vflip = 0;
	info->gain = 0;

	info->tpf.numerator = 1;
	info->tpf.denominator = 30;    /* 30fps */

	ret = sensor_write_array(sd, sensor_default_regs,
				ARRAY_SIZE(sensor_default_regs));
	if (ret < 0) {
		vfe_dev_err("write sensor_default_regs error\n");
		return ret;
	}

	if (info->stby_mode == 0)
		info->init_first_flag = 0;

	info->preview_first_flag = 1;

	return 0;
}

static long sensor_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	int ret = 0;
	struct sensor_info *info = to_state(sd);

	switch (cmd) {
	case GET_CURRENT_WIN_CFG:
		if (info->current_wins != NULL) {
			memcpy(arg,
				info->current_wins,
				sizeof(struct sensor_win_size));
			ret = 0;
		} else {
			vfe_dev_err("empty wins!\n");
			ret = -1;
		}
		break;
	case SET_FPS:
		break;
	case ISP_SET_EXP_GAIN:
		sensor_s_exp_gain(sd, (struct sensor_exp_gain *)arg);
		break;
	default:
		return -EINVAL;
	}
	return ret;
}


/*
 * Store information about the video data format.
 */
static struct sensor_format_struct {
	__u8 *desc;
	/* __u32 pixelformat; */
	u32 mbus_code;
	struct regval_list *regs;
	int regs_size;
	int bpp;   /* Bytes per pixel */
} sensor_formats[] = {
	{
		.desc       = "Raw RGB Bayer",
		.mbus_code  = MEDIA_BUS_FMT_SRGGB10_1X10,
		.regs       = sensor_fmt_raw,
		.regs_size  = ARRAY_SIZE(sensor_fmt_raw),
		.bpp        = 1
	},
};
#define N_FMTS ARRAY_SIZE(sensor_formats)



/*
 * Then there is the issue of window sizes.  Try to capture the info here.
 */


static struct sensor_win_size sensor_win_sizes[] = {
	/* 3280*2464 */
	{
		.width      = 3264, /* 3280, */
		.height     = 2448, /* 2464, */
		.hoffset    = (3280-3264)/2, /* 0, */
		.voffset    = (2464-2448)/2, /* 0, */
		.hts        = 3448, /* //4352,//3448,//4037, */
		.vts        = 4037, /* //2757,//4037, */
		.pclk       = (278*1000*1000), /* 252*1000*1000, */
		.mipi_bps   = 720*1000*1000,
		.fps_fixed  = 1,
		.bin_factor = 1,
		.intg_min   = 1<<4,
		.intg_max   = (4037-4)<<4,
		.gain_min   = 1<<4,
		.gain_max   = 10<<4,
		.regs       = sensor_hxga_regs,
		.regs_size  = ARRAY_SIZE(sensor_hxga_regs),
		.set_size   = NULL,
	},

	/* 1080P */
	{
		.width      = HD1080_WIDTH,
		.height     = HD1080_HEIGHT,
		.hoffset    = 0,
		.voffset    = 0,
		.hts        = 3560,
		.vts        = 2607,
		.pclk       = (278*1000*1000),
		.mipi_bps   = 720*1000*1000,
		.fps_fixed  = 1,
		.bin_factor = 2,
		.intg_min   = 1<<4,
		.intg_max   = (2607-4)<<4,
		.gain_min   = 1<<4,
		.gain_max   = 10<<4,
		.regs       = sensor_1080p_regs,
		.regs_size  = ARRAY_SIZE(sensor_1080p_regs),
		.set_size   = NULL,
	},
	/* SXGA */
	{
		.width      = SXGA_WIDTH,
		.height     = SXGA_HEIGHT,
		.hoffset    = 0,
		.voffset    = 0,
		.hts        = 3560,
		.vts        = 2607,
		.pclk       = (278*1000*1000),
		.mipi_bps   = 720*1000*1000,
		.fps_fixed  = 1,
		.bin_factor = 2,
		.intg_min   = 1<<4,
		.intg_max   = 2607<<4,
		.gain_min   = 1<<4,
		.gain_max   = 10<<4,
		.regs       = sensor_sxga_regs,
		.regs_size  = ARRAY_SIZE(sensor_sxga_regs),
		.set_size   = NULL,
	},
	/* 720p */
	{
		.width      = HD720_WIDTH,
		.height     = HD720_HEIGHT,
		.hoffset    = 0,
		.voffset    = 0,
		.hts        = 2560,
		.vts        = 1303, /* 735 106fps */
		.pclk       = (200*1000*1000),
		.mipi_bps   = 720*1000*1000,
		.fps_fixed  = 1,
		.bin_factor = 2,
		.intg_min   = 1<<4,
		.intg_max   = (1303-4)<<4,
		.gain_min   = 1<<4,
		.gain_max   = 10<<4,
		.regs       = sensor_720p_regs,
		.regs_size  = ARRAY_SIZE(sensor_720p_regs),
		.set_size   = NULL,
	},

};

#define N_WIN_SIZES (ARRAY_SIZE(sensor_win_sizes))

static int sensor_enum_code(struct v4l2_subdev *sd,
				struct v4l2_subdev_pad_config *cfg,
				struct v4l2_subdev_mbus_code_enum *code)
{
	if (code->index >= N_FMTS)
		return -EINVAL;

	code->code = sensor_formats[code->index].mbus_code;
	return 0;
}

static int sensor_enum_frame_size(struct v4l2_subdev *sd,
				struct v4l2_subdev_pad_config *cfg,
				struct v4l2_subdev_frame_size_enum *fse)
{
	if (fse->index > N_WIN_SIZES-1)
		return -EINVAL;

	fse->min_width = sensor_win_sizes[fse->index].width;
	fse->max_width = fse->min_width;
	fse->min_height = sensor_win_sizes[fse->index].height;
	fse->max_height = fse->min_height;

	return 0;
}


static int sensor_try_fmt_internal(struct v4l2_subdev *sd,
				struct v4l2_mbus_framefmt *fmt,
				struct sensor_format_struct **ret_fmt,
				struct sensor_win_size **ret_wsize)
{
	int index;
	struct sensor_win_size *wsize;
	struct sensor_info *info = to_state(sd);

	for (index = 0; index < N_FMTS; index++)
		if (sensor_formats[index].mbus_code == fmt->code)
			break;

	if (index >= N_FMTS)
		return -EINVAL;

	if (ret_fmt != NULL)
		*ret_fmt = sensor_formats + index;

	/*
	 * Fields: the sensor devices claim to be progressive.
	 */
	fmt->field = V4L2_FIELD_NONE;

	/*
	 * Round requested image size down to the nearest
	 * we support, but not below the smallest.
	 */
	for (wsize = sensor_win_sizes; wsize < sensor_win_sizes + N_WIN_SIZES; wsize++)
		if (fmt->width >= wsize->width && fmt->height >= wsize->height)
			break;

	if (wsize >= sensor_win_sizes + N_WIN_SIZES)
		wsize--;   /* Take the smallest one */
	if (ret_wsize != NULL)
		*ret_wsize = wsize;
	/*
	 * Note the size we'll actually handle.
	 */
	fmt->width = wsize->width;
	fmt->height = wsize->height;
	info->current_wins = wsize;
	return 0;
}

static int sensor_get_fmt(struct v4l2_subdev *sd,
				struct v4l2_subdev_pad_config *cfg,
				struct v4l2_subdev_format *fmat)
{
	struct v4l2_mbus_framefmt *fmt = &fmat->format;

	return sensor_try_fmt_internal(sd, fmt, NULL, NULL);
}

static int sensor_g_mbus_config(struct v4l2_subdev *sd,
				struct v4l2_mbus_config *cfg)
{
	cfg->type = V4L2_MBUS_CSI2;
	cfg->flags = 0 | V4L2_MBUS_CSI2_4_LANE | V4L2_MBUS_CSI2_CHANNEL_0;

	return 0;
}


/*
 * Set a format.
 */
static int sensor_set_fmt(struct v4l2_subdev *sd,
				struct v4l2_subdev_pad_config *cfg,
				struct v4l2_subdev_format *fmat)
{
	int ret;
	struct v4l2_mbus_framefmt *fmt = &fmat->format;
	struct sensor_format_struct *sensor_fmt;
	struct sensor_win_size *wsize;
	struct sensor_info *info = to_state(sd);

	vfe_dev_dbg("sensor_s_fmt\n");

	ret = sensor_try_fmt_internal(sd, fmt, &sensor_fmt, &wsize);
	if (ret)
		return ret;

/*
 *if(info->capture_mode == V4L2_MODE_VIDEO)
 *{
 * //video
 *}
 *else if(info->capture_mode == V4L2_MODE_IMAGE)
 *{
 * //image
 *}
 */

	sensor_write_array(sd, sensor_fmt->regs, sensor_fmt->regs_size);

	ret = 0;
	if (wsize->regs)
		LOG_ERR_RET(sensor_write_array(sd,
				wsize->regs, wsize->regs_size));

	if (wsize->set_size)
		LOG_ERR_RET(wsize->set_size(sd));

	info->fmt = sensor_fmt;
	info->width = wsize->width;
	info->height = wsize->height;
	imx219_sensor_vts = wsize->vts;

	vfe_dev_print("s_fmt set width = %d, height = %d\n",
				wsize->width, wsize->height);

/*
 *if(info->capture_mode == V4L2_MODE_VIDEO)
 *{
 * //video
 *} else {
 * //capture image
 *}
 */

	return 0;
}

/*
 * Implement G/S_PARM.  There is a "high quality" mode we could try
 * to do someday; for now, we just do the frame rate tweak.
 */
static int sensor_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	struct v4l2_captureparm *cp = &parms->parm.capture;
	struct sensor_info *info = to_state(sd);

	if (parms->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;

	memset(cp, 0, sizeof(struct v4l2_captureparm));
	cp->capability = V4L2_CAP_TIMEPERFRAME;
	cp->capturemode = info->capture_mode;

	return 0;
}

static int sensor_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	struct v4l2_captureparm *cp = &parms->parm.capture;
	struct sensor_info *info = to_state(sd);

	vfe_dev_dbg("sensor_s_parm\n");

	if (parms->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;

	if (info->tpf.numerator == 0)
		return -EINVAL;

	info->capture_mode = cp->capturemode;

	return 0;
}

static int sensor_g_ctrl(struct v4l2_ctrl *ctrl)
{
	struct sensor_info *info =
			container_of(ctrl->handler, struct sensor_info, handler);
	struct v4l2_subdev *sd = &info->sd;

	switch (ctrl->id) {
	case V4L2_CID_GAIN:
		return sensor_g_gain(sd, &ctrl->val);
	case V4L2_CID_EXPOSURE:
		return sensor_g_exp(sd, &ctrl->val);
	}
	return -EINVAL;
}

static int sensor_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct sensor_info *info =
			container_of(ctrl->handler, struct sensor_info, handler);
	struct v4l2_subdev *sd = &info->sd;

	switch (ctrl->id) {
	case V4L2_CID_GAIN:
		return sensor_s_gain(sd, ctrl->val);
	case V4L2_CID_EXPOSURE:
		return sensor_s_exp(sd, ctrl->val);
	}
	return -EINVAL;
}


/* ----------------------------------------------------------------------- */

static const struct v4l2_ctrl_ops sensor_ctrl_ops = {
	.g_volatile_ctrl = sensor_g_ctrl,
	.s_ctrl = sensor_s_ctrl,
};

static const struct v4l2_subdev_core_ops sensor_core_ops = {
	.reset = sensor_reset,
	.init = sensor_init,
	.s_power = sensor_power,
	.ioctl = sensor_ioctl,
};

static const struct v4l2_subdev_video_ops sensor_video_ops = {
	.s_parm = sensor_s_parm,
	.g_parm = sensor_g_parm,
	.g_mbus_config = sensor_g_mbus_config,
};

static const struct v4l2_subdev_pad_ops sensor_pad_ops = {
	.enum_mbus_code = sensor_enum_code,
	.enum_frame_size = sensor_enum_frame_size,
	.get_fmt = sensor_get_fmt,
	.set_fmt = sensor_set_fmt,
};

static const struct v4l2_subdev_ops sensor_ops = {
	.core = &sensor_core_ops,
	.video = &sensor_video_ops,
	.pad = &sensor_pad_ops,
};

/* ----------------------------------------------------------------------- */
static struct cci_driver cci_drv = {
	.name = SENSOR_NAME,
	.addr_width = CCI_BITS_16,
	.data_width = CCI_BITS_8,
};

static const struct v4l2_ctrl_config sensor_custom_ctrls[] = {
	{
		.ops = &sensor_ctrl_ops,
		.id = V4L2_CID_FRAME_RATE,
		.name = "frame rate",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 15,
		.max = 120,
		.step = 1,
		.def = 120,
	},
};

static int sensor_init_controls(struct v4l2_subdev *sd, const struct v4l2_ctrl_ops *ops)
{
	struct sensor_info *info = to_state(sd);
	struct v4l2_ctrl_handler *handler = &info->handler;
	struct v4l2_ctrl *ctrl;
	int ret = 0;
	int i;

	v4l2_ctrl_handler_init(handler, 2 +  ARRAY_SIZE(sensor_custom_ctrls));

	ctrl = v4l2_ctrl_new_std(handler, ops, V4L2_CID_GAIN, 1*16, 64*16-1, 1, 1*16);
	if (ctrl != NULL)
		ctrl->flags |= V4L2_CTRL_FLAG_VOLATILE;
	ctrl = v4l2_ctrl_new_std(handler, ops, V4L2_CID_EXPOSURE, 0, 65536*16, 1, 0);
	if (ctrl != NULL)
		ctrl->flags |= V4L2_CTRL_FLAG_VOLATILE;
	for (i = 0; i < ARRAY_SIZE(sensor_custom_ctrls); i++)
		v4l2_ctrl_new_custom(handler, &sensor_custom_ctrls[i], NULL);

	if (handler->error) {
		ret = handler->error;
		v4l2_ctrl_handler_free(handler);
	}

	sd->ctrl_handler = handler;

	return ret;
}

static int sensor_probe(struct i2c_client *client,
				const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd;
	struct sensor_info *info;

	info = kzalloc(sizeof(struct sensor_info), GFP_KERNEL);
	if (info == NULL)
		return -ENOMEM;
	sd = &info->sd;
	glb_sd = sd;
	sensor_init_controls(sd, &sensor_ctrl_ops);
	cci_dev_probe_helper(sd, client, &sensor_ops, &cci_drv);

	info->fmt = &sensor_formats[0];
	info->af_first_flag = 1;
	info->init_first_flag = 1;

	return 0;
}
static int sensor_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd;

	sd = cci_dev_remove_helper(client, &cci_drv);
	kfree(to_state(sd));
	return 0;
}

static const struct i2c_device_id sensor_id[] = {
	{SENSOR_NAME, 0},
	{ }
};
MODULE_DEVICE_TABLE(i2c, sensor_id);


static struct i2c_driver sensor_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = SENSOR_NAME,
	},
	.probe = sensor_probe,
	.remove = sensor_remove,
	.id_table = sensor_id,
};
static __init int init_sensor(void)
{
	return cci_dev_init_helper(&sensor_driver);
}

static __exit void exit_sensor(void)
{
	cci_dev_exit_helper(&sensor_driver);
}

module_init(init_sensor);
module_exit(exit_sensor);

