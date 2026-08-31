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

/* ------------------------------------------------------------------ */
/* VLAN helpers                                                       */
/* ------------------------------------------------------------------ */

static int vlan_port_index_reg(uint8_t port, uint8_t *reg, uint8_t *shift)
{
	switch (port) {
	case 0:
		*reg = RTL8305_REG_PORT_INDEX_0_3;
		*shift = RTL8305_P0VLANINDEX_SHIFT;
		break;
	case 1:
		*reg = RTL8305_REG_PORT_INDEX_0_3;
		*shift = RTL8305_P1VLANINDEX_SHIFT;
		break;
	case 2:
		*reg = RTL8305_REG_PORT_INDEX_0_3;
		*shift = RTL8305_P2VLANINDEX_SHIFT;
		break;
	case 3:
		*reg = RTL8305_REG_PORT_INDEX_0_3;
		*shift = RTL8305_P3VLANINDEX_SHIFT;
		break;
	case 4:
		*reg = RTL8305_REG_PORT_INDEX_4;
		*shift = RTL8305_P4VLANINDEX_SHIFT;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

int rtl8305_vlan_get_port_index(const struct device *dev, uint8_t port,
				uint8_t *index)
{
	uint8_t reg, shift;
	uint16_t val;
	int ret;

	if (dev == NULL || index == NULL) {
		return -EINVAL;
	}

	ret = vlan_port_index_reg(port, &reg, &shift);
	if (ret < 0) {
		return ret;
	}

	ret = rtl8305_read(dev, RTL8305_PHY_PORT_INDEX, reg, &val);
	if (ret < 0) {
		return ret;
	}

	*index = (val >> shift) & RTL8305_VLANINDEX_MASK;

	return 0;
}

int rtl8305_vlan_set_port_index(const struct device *dev, uint8_t port,
				uint8_t index)
{
	uint8_t reg, shift;
	uint16_t val;
	int ret;

	if (dev == NULL || index > RTL8305_VLANINDEX_MASK) {
		return -EINVAL;
	}

	ret = vlan_port_index_reg(port, &reg, &shift);
	if (ret < 0) {
		return ret;
	}

	ret = rtl8305_read(dev, RTL8305_PHY_PORT_INDEX, reg, &val);
	if (ret < 0) {
		return ret;
	}

	val &= ~(RTL8305_VLANINDEX_MASK << shift);
	val |= (uint16_t)(index & RTL8305_VLANINDEX_MASK) << shift;

	return rtl8305_write(dev, RTL8305_PHY_PORT_INDEX, reg, val);
}

int rtl8305_vlan_get_vid(const struct device *dev, uint8_t group,
			 uint16_t *vid)
{
	uint16_t hi, lo;
	int ret;

	if (dev == NULL || vid == NULL || group >= RTL8305_NUM_VLAN_GROUPS) {
		return -EINVAL;
	}

	switch (group) {
	case RTL8305_VLAN_A:
		return rtl8305_read(dev, RTL8305_PHY_VLAN, RTL8305_REG_VIDA, vid);
	case RTL8305_VLAN_B:
		ret = rtl8305_read(dev, RTL8305_PHY_VLAN, RTL8305_REG_VIDB_HI, &hi);
		if (ret < 0) {
			return ret;
		}
		ret = rtl8305_read(dev, RTL8305_PHY_VLAN, RTL8305_REG_VIDB_LO, &lo);
		if (ret < 0) {
			return ret;
		}
		*vid = ((hi >> 8) << 4) | (lo & 0x0f);
		return 0;
	case RTL8305_VLAN_C:
		return rtl8305_read(dev, RTL8305_PHY_VLAN, RTL8305_REG_VIDC, vid);
	case RTL8305_VLAN_D:
		ret = rtl8305_read(dev, RTL8305_PHY_VLAN, RTL8305_REG_VIDD_HI, &hi);
		if (ret < 0) {
			return ret;
		}
		ret = rtl8305_read(dev, RTL8305_PHY_VLAN, RTL8305_REG_VIDD_LO, &lo);
		if (ret < 0) {
			return ret;
		}
		*vid = ((hi >> 8) << 4) | (lo & 0x0f);
		return 0;
	case RTL8305_VLAN_E:
		return rtl8305_read(dev, RTL8305_PHY_VLAN, RTL8305_REG_VIDE, vid);
	default:
		return -EINVAL;
	}
}

int rtl8305_vlan_set_vid(const struct device *dev, uint8_t group,
			 uint16_t vid)
{
	uint16_t hi, lo;
	int ret;

	if (dev == NULL || group >= RTL8305_NUM_VLAN_GROUPS || vid > 0xfff) {
		return -EINVAL;
	}

	switch (group) {
	case RTL8305_VLAN_A:
		return rtl8305_write(dev, RTL8305_PHY_VLAN, RTL8305_REG_VIDA, vid);
	case RTL8305_VLAN_B:
		ret = rtl8305_read(dev, RTL8305_PHY_VLAN, RTL8305_REG_VIDB_HI, &hi);
		if (ret < 0) {
			return ret;
		}
		ret = rtl8305_read(dev, RTL8305_PHY_VLAN, RTL8305_REG_VIDB_LO, &lo);
		if (ret < 0) {
			return ret;
		}
		hi = (hi & 0x00ff) | ((uint16_t)(vid >> 4) << 8);
		lo = (lo & 0xfff0) | (vid & 0x0f);
		ret = rtl8305_write(dev, RTL8305_PHY_VLAN, RTL8305_REG_VIDB_HI, hi);
		if (ret < 0) {
			return ret;
		}
		return rtl8305_write(dev, RTL8305_PHY_VLAN, RTL8305_REG_VIDB_LO, lo);
	case RTL8305_VLAN_C:
		return rtl8305_write(dev, RTL8305_PHY_VLAN, RTL8305_REG_VIDC, vid);
	case RTL8305_VLAN_D:
		ret = rtl8305_read(dev, RTL8305_PHY_VLAN, RTL8305_REG_VIDD_HI, &hi);
		if (ret < 0) {
			return ret;
		}
		ret = rtl8305_read(dev, RTL8305_PHY_VLAN, RTL8305_REG_VIDD_LO, &lo);
		if (ret < 0) {
			return ret;
		}
		hi = (hi & 0x00ff) | ((uint16_t)(vid >> 4) << 8);
		lo = (lo & 0xfff0) | (vid & 0x0f);
		ret = rtl8305_write(dev, RTL8305_PHY_VLAN, RTL8305_REG_VIDD_HI, hi);
		if (ret < 0) {
			return ret;
		}
		return rtl8305_write(dev, RTL8305_PHY_VLAN, RTL8305_REG_VIDD_LO, lo);
	case RTL8305_VLAN_E:
		return rtl8305_write(dev, RTL8305_PHY_VLAN, RTL8305_REG_VIDE, vid);
	default:
		return -EINVAL;
	}
}

int rtl8305_vlan_get_member(const struct device *dev, uint8_t group,
			    uint8_t *member)
{
	uint8_t reg;
	uint16_t val;
	int ret;

	if (dev == NULL || member == NULL || group >= RTL8305_NUM_VLAN_GROUPS) {
		return -EINVAL;
	}

	switch (group) {
	case RTL8305_VLAN_A:
		reg = RTL8305_REG_MEMBERA;
		break;
	case RTL8305_VLAN_B:
		reg = RTL8305_REG_MEMBERB;
		break;
	case RTL8305_VLAN_C:
		reg = RTL8305_REG_MEMBERC;
		break;
	case RTL8305_VLAN_D:
		reg = RTL8305_REG_MEMBERD;
		break;
	case RTL8305_VLAN_E:
		reg = RTL8305_REG_MEMBERE;
		break;
	default:
		return -EINVAL;
	}

	ret = rtl8305_read(dev, RTL8305_PHY_VLAN, reg, &val);
	if (ret < 0) {
		return ret;
	}

	*member = (val >> RTL8305_MEMBER_SHIFT) & RTL8305_MEMBER_MASK;

	return 0;
}

int rtl8305_vlan_set_member(const struct device *dev, uint8_t group,
			    uint8_t member)
{
	uint8_t reg;
	uint16_t val;
	int ret;

	if (dev == NULL || group >= RTL8305_NUM_VLAN_GROUPS ||
	    member > RTL8305_MEMBER_MASK) {
		return -EINVAL;
	}

	switch (group) {
	case RTL8305_VLAN_A:
		reg = RTL8305_REG_MEMBERA;
		break;
	case RTL8305_VLAN_B:
		reg = RTL8305_REG_MEMBERB;
		break;
	case RTL8305_VLAN_C:
		reg = RTL8305_REG_MEMBERC;
		break;
	case RTL8305_VLAN_D:
		reg = RTL8305_REG_MEMBERD;
		break;
	case RTL8305_VLAN_E:
		reg = RTL8305_REG_MEMBERE;
		break;
	default:
		return -EINVAL;
	}

	ret = rtl8305_read(dev, RTL8305_PHY_VLAN, reg, &val);
	if (ret < 0) {
		return ret;
	}

	val &= ~(RTL8305_MEMBER_MASK << RTL8305_MEMBER_SHIFT);
	val |= (uint16_t)(member & RTL8305_MEMBER_MASK) << RTL8305_MEMBER_SHIFT;

	return rtl8305_write(dev, RTL8305_PHY_VLAN, reg, val);
}

int rtl8305_vlan_get_ctrl(const struct device *dev, uint16_t *ctrl)
{
	if (dev == NULL || ctrl == NULL) {
		return -EINVAL;
	}

	return rtl8305_read(dev, RTL8305_PHY_VLAN_CTRL, RTL8305_REG_VLAN_CTRL,
			    ctrl);
}

int rtl8305_vlan_set_ctrl(const struct device *dev, uint16_t ctrl)
{
	if (dev == NULL) {
		return -EINVAL;
	}

	return rtl8305_write(dev, RTL8305_PHY_VLAN_CTRL, RTL8305_REG_VLAN_CTRL,
			     ctrl);
}

int rtl8305_soft_reset(const struct device *dev)
{
	if (dev == NULL) {
		return -EINVAL;
	}

	return rtl8305_write(dev, RTL8305_PHY_PORT_CTRL, RTL8305_REG_PORT_CTRL,
			     RTL8305_PORT_CTRL_SOFTRESET);
}

int rtl8305_vlan_lan(const struct device *dev, uint8_t port, uint16_t vid)
{
	uint8_t group;
	uint8_t member[RTL8305_NUM_VLAN_GROUPS];
	uint16_t group_vid;
	int ret;

	if (dev == NULL || port >= RTL8305_NUM_PORTS || vid > 0xfff) {
		return -EINVAL;
	}

	/* Port N maps to group N. */
	group = port;

	/* Set the group VID and assign the port to that group. */
	ret = rtl8305_vlan_set_vid(dev, group, vid);
	if (ret < 0) {
		return ret;
	}

	ret = rtl8305_vlan_set_port_index(dev, port, group);
	if (ret < 0) {
		return ret;
	}

	/*
	 * Recompute every group's member set so that ports sharing the
	 * same VID can communicate. A group's member set is the union of
	 * all ports whose group currently has that group's VID.
	 */
	for (uint8_t g = 0; g < RTL8305_NUM_VLAN_GROUPS; g++) {
		member[g] = 0;
	}

	for (uint8_t p = 0; p < RTL8305_NUM_PORTS; p++) {
		uint8_t p_index;

		ret = rtl8305_vlan_get_port_index(dev, p, &p_index);
		if (ret < 0) {
			return ret;
		}

		ret = rtl8305_vlan_get_vid(dev, p_index, &group_vid);
		if (ret < 0) {
			return ret;
		}

		/* Port p joins every group whose VID matches its own. */
		for (uint8_t g = 0; g < RTL8305_NUM_VLAN_GROUPS; g++) {
			uint16_t g_vid;

			ret = rtl8305_vlan_get_vid(dev, g, &g_vid);
			if (ret < 0) {
				return ret;
			}

			if (g_vid == group_vid) {
				member[g] |= BIT(p);
			}
		}
	}

	for (uint8_t g = 0; g < RTL8305_NUM_VLAN_GROUPS; g++) {
		ret = rtl8305_vlan_set_member(dev, g, member[g]);
		if (ret < 0) {
			return ret;
		}
	}

	return rtl8305_soft_reset(dev);
}