/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * RTL8305 switch register access layer.
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/mdio.h>
#include <zephyr/sys/printk.h>

#include "rtl8305.h"

int rtl8305_read(const struct device *dev, uint8_t phy, uint8_t reg,
		 uint16_t *val)
{
	int ret;

	if (dev == NULL || val == NULL) {
		return -EINVAL;
	}

	ret = mdio_read(dev, phy, reg, val);
	if (ret < 0) {
		return ret;
	}

	/* 0xFFFF indicates no device present on the bus. */
	if (*val == UINT16_MAX) {
		return -ENODEV;
	}

	return 0;
}

int rtl8305_write(const struct device *dev, uint8_t phy, uint8_t reg,
		  uint16_t val)
{
	if (dev == NULL) {
		return -EINVAL;
	}

	return mdio_write(dev, phy, reg, val);
}

int rtl8305_get_chip_id(const struct device *dev, uint16_t *id)
{
	return rtl8305_read(dev, 0, RTL8305_CHIP_ID_REG, id);
}

int rtl8305_get_link_state(const struct device *dev, uint8_t phy, bool *link)
{
	uint16_t val;
	int ret;

	if (link == NULL) {
		return -EINVAL;
	}

	ret = rtl8305_read(dev, phy, RTL8305_MII_STATUS_REG, &val);
	if (ret < 0) {
		return ret;
	}

	*link = (val & RTL8305_MII_LINK_BIT) != 0;

	return 0;
}

int rtl8305_dump(const struct device *dev, uint8_t phy, uint8_t start,
		 uint8_t count)
{
	uint16_t val;
	int ret;

	if (dev == NULL) {
		return -EINVAL;
	}

	for (uint8_t i = 0; i < count; i++) {
		ret = rtl8305_read(dev, phy, start + i, &val);
		if (ret < 0) {
			printk("reg 0x%02x: read failed (%d)\n", start + i, ret);
			return ret;
		}
		printk("reg 0x%02x: 0x%04x\n", start + i, val);
	}

	return 0;
}