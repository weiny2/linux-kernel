// SPDX-License-Identifier: GPL-2.0
//
// CZ.NIC's Turris Omnia LEDs driver
//
// 2019 by Marek Behun <marek.behun@nic.cz>

#include <linux/i2c.h>
#include <linux/leds.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <uapi/linux/uleds.h>

#define OMNIA_BOARD_LEDS	12

#define CMD_LED_MODE		3
#define CMD_LED_MODE_LED(l)	((l) & 0x0f)
#define CMD_LED_MODE_USER	0x10

#define CMD_LED_STATE		4
#define CMD_LED_STATE_LED(l)	((l) & 0x0f)
#define CMD_LED_STATE_ON	0x10

#define CMD_LED_COLOR		5
#define CMD_LED_SET_BRIGHTNESS	7
#define CMD_LED_GET_BRIGHTNESS	8

struct omnia_led {
	char name[LED_MAX_NAME_SIZE];
	struct led_classdev cdev;
};

#define to_omnia_led(l)	container_of(l, struct omnia_led, cdev)

struct omnia_leds {
	struct i2c_client *client;
	struct mutex lock;
	struct omnia_led leds[OMNIA_BOARD_LEDS];
};

static int omnia_led_idx(struct omnia_leds *leds, struct led_classdev *led)
{
	int idx = to_omnia_led(led) - &leds->leds[0];

	if (idx < 0 || idx >= OMNIA_BOARD_LEDS)
		return -ENXIO;

	return idx;
}

static int omnia_led_brightness_set_blocking(struct led_classdev *led,
					     enum led_brightness brightness)
{
	struct omnia_leds *leds = dev_get_drvdata(led->dev->parent);
	int idx = omnia_led_idx(leds, led);
	int ret;
	u8 state;

	if (idx < 0)
		return idx;

	state = CMD_LED_STATE_LED(idx);
	if (brightness)
		state |= CMD_LED_STATE_ON;

	mutex_lock(&leds->lock);
	ret = i2c_smbus_write_byte_data(leds->client, CMD_LED_STATE, state);
	mutex_unlock(&leds->lock);

	return ret;
}

static int omnia_led_register(struct omnia_leds *leds,
			      struct fwnode_handle *node)
{
	struct i2c_client *client = leds->client;
	struct device *dev = &client->dev;
	struct device_node *np = to_of_node(node);
	struct omnia_led *led;
	const char *str;
	int ret;
	u32 reg;

	ret = fwnode_property_read_u32(node, "reg", &reg);
	if (ret) {
		dev_err(dev, "Failed to read LED 'reg' property: %i\n", ret);
		return ret;
	}

	if (reg >= OMNIA_BOARD_LEDS) {
		dev_warn(dev, "Invalid Turris Omnia LED index %u\n", reg);
		return 0;
	}

	led = &leds->leds[reg];

	ret = fwnode_property_read_string(node, "label", &str);
	snprintf(led->name, sizeof(led->name), "omnia::%s", ret ? "" : str);

	led->cdev.max_brightness = 1;
	led->cdev.brightness_set_blocking = omnia_led_brightness_set_blocking;
	led->cdev.name = led->name;

	fwnode_property_read_string(node, "linux,default-trigger",
				    &led->cdev.default_trigger);

	/* put the LED into software mode */
	ret = i2c_smbus_write_byte_data(client, CMD_LED_MODE,
					CMD_LED_MODE_LED(reg) |
					CMD_LED_MODE_USER);
	if (ret < 0) {
		dev_err(dev, "Cannot set LED %u to software mode: %i\n", reg,
			ret);
		return ret;
	}

	/* disable the LED */
	ret = i2c_smbus_write_byte_data(client, CMD_LED_STATE,
						CMD_LED_STATE_LED(reg));
	if (ret < 0) {
		dev_err(dev, "Cannot set LED %u brightness: %i\n", reg, ret);
		return ret;
	}

	ret = devm_of_led_classdev_register(dev, np, &led->cdev);
	if (ret < 0) {
		dev_err(dev, "Cannot register LED %i: %i\n", reg, ret);
		return ret;
	}

	led->cdev.dev->of_node = np;

	return 0;
}

static int omnia_leds_probe(struct i2c_client *client,
			    const struct i2c_device_id *id)
{
	struct device *dev = &client->dev;
	struct fwnode_handle *child;
	struct omnia_leds *leds;
	int ret;

	if (!device_get_child_node_count(dev)) {
		dev_err(dev, "LEDs are not defined in device tree!\n");
		return -ENODEV;
	}

	leds = devm_kzalloc(dev, sizeof(*leds), GFP_KERNEL);
	if (!leds)
		return -ENOMEM;

	leds->client = client;
	i2c_set_clientdata(client, leds);

	mutex_init(&leds->lock);

	device_for_each_child_node(dev, child) {
		ret = omnia_led_register(leds, child);
		if (ret < 0)
			return ret;
	}

	return 0;
}

static int omnia_leds_remove(struct i2c_client *client)
{
	/* put all LEDs into default (HW triggered) mode */
	i2c_smbus_write_byte_data(client, CMD_LED_MODE,
				  CMD_LED_MODE_LED(OMNIA_BOARD_LEDS));

	return 0;
}

static const struct of_device_id of_omnia_leds_match[] = {
	{ .compatible = "cznic,turris-omnia-leds", },
	{},
};

static const struct i2c_device_id omnia_id[] = {
	{ "omnia", 0 },
	{ }
};

static struct i2c_driver omnia_leds_driver = {
	.probe		= omnia_leds_probe,
	.remove		= omnia_leds_remove,
	.id_table	= omnia_id,
	.driver		= {
		.name	= "leds-turris-omnia",
		.of_match_table = of_omnia_leds_match,
	},
};

module_i2c_driver(omnia_leds_driver);

MODULE_AUTHOR("Marek Behun <marek.behun@nic.cz>");
MODULE_DESCRIPTION("CZ.NIC's Turris Omnia LEDs");
MODULE_LICENSE("GPL v2");
