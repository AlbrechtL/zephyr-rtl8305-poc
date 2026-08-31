/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * RTL8305 switch shell commands.
 */

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/gpio.h>

#include "rtl8305.h"

#define MDIO_NODE DT_NODELABEL(mdio0)
#define DIP_SW_0_NODE DT_ALIAS(dip0)
#define DIP_SW_1_NODE DT_ALIAS(dip1)
#define DIP_SW_2_NODE DT_ALIAS(dip2)
#define DIP_SW_3_NODE DT_ALIAS(dip3)

#define NUM_DIP_SWITCHES 4
static const struct gpio_dt_spec dip_switches[NUM_DIP_SWITCHES] = {
	GPIO_DT_SPEC_GET(DIP_SW_0_NODE, gpios),
	GPIO_DT_SPEC_GET(DIP_SW_1_NODE, gpios),
	GPIO_DT_SPEC_GET(DIP_SW_2_NODE, gpios),
	GPIO_DT_SPEC_GET(DIP_SW_3_NODE, gpios),
};

static const struct device *rtl8305_mdio_dev(void)
{
	return DEVICE_DT_GET(MDIO_NODE);
}

/* status link - print the link state of all ports. */
static int cmd_status_link(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev = rtl8305_mdio_dev();
	bool link;
	int ret;

	if (!device_is_ready(dev)) {
		shell_error(sh, "MDIO device not ready");
		return -ENODEV;
	}

	for (uint8_t phy = 0; phy <= 0x8; phy++) {
		ret = rtl8305_get_link_state(dev, phy, &link);
		if (ret < 0) {
			shell_print(sh, "PHY %u: read failed (%d)", phy, ret);
			continue;
		}

		shell_print(sh, "PHY %u: %s", phy, link ? "link up" : "link down");
	}

	return 0;
}

/* status dump <phy> <start> <count> - dump a raw register range. */
static int cmd_status_dump(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev = rtl8305_mdio_dev();
	uint8_t phy, start, count;
	int ret;

	if (!device_is_ready(dev)) {
		shell_error(sh, "MDIO device not ready");
		return -ENODEV;
	}

	phy = (uint8_t)shell_strtoul(argv[1], 0, 0);
	start = (uint8_t)shell_strtoul(argv[2], 0, 0);
	count = (uint8_t)shell_strtoul(argv[3], 0, 0);

	if (phy > 0x8) {
		shell_error(sh, "Invalid PHY address %u (max 8)", phy);
		return -EINVAL;
	}

	ret = rtl8305_dump(dev, phy, start, count);
	if (ret < 0) {
		shell_error(sh, "Register dump failed (%d)", ret);
		return ret;
	}

	return 0;
}

/* dipsw - print the state of all DIP switches. */
static int cmd_dipsw(const struct shell *sh, size_t argc, char **argv)
{
	for (int i = 0; i < NUM_DIP_SWITCHES; i++) {
		if (!gpio_is_ready_dt(&dip_switches[i])) {
			shell_error(sh, "DIP switch %d GPIO not ready", i);
			return -ENODEV;
		}

		gpio_pin_configure_dt(&dip_switches[i], GPIO_INPUT);
		shell_print(sh, "DIP switch %d: %s", i,
			    gpio_pin_get_dt(&dip_switches[i]) ? "OFF" : "ON");
	}

	return 0;
}

/* vlan - VLAN configuration. */
static const char *vlan_group_name(uint8_t group)
{
	static const char *const names[RTL8305_NUM_VLAN_GROUPS] = {
		"A", "B", "C", "D", "E",
	};

	if (group >= RTL8305_NUM_VLAN_GROUPS) {
		return "?";
	}

	return names[group];
}

/* vlan show - display the current VLAN configuration. */
static int cmd_vlan_show(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev = rtl8305_mdio_dev();
	uint16_t vid, ctrl;
	uint8_t member;
	int ret;

	if (!device_is_ready(dev)) {
		shell_error(sh, "MDIO device not ready");
		return -ENODEV;
	}

	ret = rtl8305_vlan_get_ctrl(dev, &ctrl);
	if (ret < 0) {
		shell_error(sh, "Failed to read VLAN control (%d)", ret);
		return ret;
	}

	shell_print(sh, "VLAN control: 0x%04x%s", ctrl,
		    (ctrl & RTL8305_VLAN_CTRL_DISVLAN) ? " (VLAN disabled)"
						       : " (VLAN enabled)");

	for (uint8_t g = 0; g < RTL8305_NUM_VLAN_GROUPS; g++) {
		ret = rtl8305_vlan_get_vid(dev, g, &vid);
		if (ret < 0) {
			shell_error(sh, "Failed to read VID of group %s (%d)",
				    vlan_group_name(g), ret);
			return ret;
		}

		ret = rtl8305_vlan_get_member(dev, g, &member);
		if (ret < 0) {
			shell_error(sh, "Failed to read member of group %s (%d)",
				    vlan_group_name(g), ret);
			return ret;
		}

		shell_print(sh, "VLAN %s: VID 0x%03x, member 0x%02x", 
			    vlan_group_name(g), vid, member);
	}

	for (uint8_t p = 0; p < RTL8305_NUM_PORTS; p++) {
		uint8_t index;

		ret = rtl8305_vlan_get_port_index(dev, p, &index);
		if (ret < 0) {
			shell_error(sh, "Failed to read port %u VLAN index (%d)",
				    p, ret);
			return ret;
		}

		shell_print(sh, "Port %u -> VLAN %s", p, vlan_group_name(index));
	}

	return 0;
}

/* vlan lan <port> <vid> - assign a port to a LAN VLAN. */
static int cmd_vlan_lan(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev = rtl8305_mdio_dev();
	uint8_t port;
	uint16_t vid;
	int ret;

	if (!device_is_ready(dev)) {
		shell_error(sh, "MDIO device not ready");
		return -ENODEV;
	}

	port = (uint8_t)shell_strtoul(argv[1], 0, 0);
	vid = (uint16_t)shell_strtoul(argv[2], 0, 0);

	if (port >= RTL8305_NUM_PORTS) {
		shell_error(sh, "Invalid port %u (0-%u)", port,
			    RTL8305_NUM_PORTS - 1);
		return -EINVAL;
	}

	if (vid > 0xfff) {
		shell_error(sh, "Invalid VID 0x%03x (max 0xfff)", vid);
		return -EINVAL;
	}

	ret = rtl8305_vlan_lan(dev, port, vid);
	if (ret < 0) {
		shell_error(sh, "VLAN configuration failed (%d)", ret);
		return ret;
	}

	shell_print(sh, "Port %u assigned to VLAN %s (VID 0x%03x)", port,
		    vlan_group_name(port), vid);

	return 0;
}

/* vlan vid <group> <vid> - set the VID of a VLAN group. */
static int cmd_vlan_vid(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev = rtl8305_mdio_dev();
	uint8_t group;
	uint16_t vid;
	int ret;

	if (!device_is_ready(dev)) {
		shell_error(sh, "MDIO device not ready");
		return -ENODEV;
	}

	group = (uint8_t)shell_strtoul(argv[1], 0, 0);
	vid = (uint16_t)shell_strtoul(argv[2], 0, 0);

	if (group >= RTL8305_NUM_VLAN_GROUPS) {
		shell_error(sh, "Invalid group %u (0-%u)", group,
			    RTL8305_NUM_VLAN_GROUPS - 1);
		return -EINVAL;
	}

	if (vid > 0xfff) {
		shell_error(sh, "Invalid VID 0x%03x (max 0xfff)", vid);
		return -EINVAL;
	}

	ret = rtl8305_vlan_set_vid(dev, group, vid);
	if (ret < 0) {
		shell_error(sh, "Failed to set VID (%d)", ret);
		return ret;
	}

	ret = rtl8305_soft_reset(dev);
	if (ret < 0) {
		shell_error(sh, "Soft reset failed (%d)", ret);
		return ret;
	}

	shell_print(sh, "VLAN %s VID set to 0x%03x", vlan_group_name(group),
		    vid);

	return 0;
}

/* vlan member <group> <member> - set the member set of a VLAN group. */
static int cmd_vlan_member(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev = rtl8305_mdio_dev();
	uint8_t group, member;
	int ret;

	if (!device_is_ready(dev)) {
		shell_error(sh, "MDIO device not ready");
		return -ENODEV;
	}

	group = (uint8_t)shell_strtoul(argv[1], 0, 0);
	member = (uint8_t)shell_strtoul(argv[2], 0, 0);

	if (group >= RTL8305_NUM_VLAN_GROUPS) {
		shell_error(sh, "Invalid group %u (0-%u)", group,
			    RTL8305_NUM_VLAN_GROUPS - 1);
		return -EINVAL;
	}

	if (member > RTL8305_MEMBER_MASK) {
		shell_error(sh, "Invalid member 0x%02x (max 0x%02x)", member,
			    RTL8305_MEMBER_MASK);
		return -EINVAL;
	}

	ret = rtl8305_vlan_set_member(dev, group, member);
	if (ret < 0) {
		shell_error(sh, "Failed to set member (%d)", ret);
		return ret;
	}

	ret = rtl8305_soft_reset(dev);
	if (ret < 0) {
		shell_error(sh, "Soft reset failed (%d)", ret);
		return ret;
	}

	shell_print(sh, "VLAN %s member set to 0x%02x", vlan_group_name(group),
		    member);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	vlan_cmds,
	SHELL_CMD(show, NULL, "Show the current VLAN configuration",
		  cmd_vlan_show),
	SHELL_CMD_ARG(lan, NULL, "Assign a port to a LAN VLAN: lan <port> <vid>",
		      cmd_vlan_lan, 3, 0),
	SHELL_CMD_ARG(vid, NULL, "Set a group VID: vid <group> <vid>",
		      cmd_vlan_vid, 3, 0),
	SHELL_CMD_ARG(member, NULL,
		      "Set a group member set: member <group> <member>",
		      cmd_vlan_member, 3, 0),
	SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(
	status_cmds,
	SHELL_CMD(link, NULL, "Print link state of all ports", cmd_status_link),
	SHELL_CMD_ARG(dump, NULL, "Dump registers: dump <phy> <start> <count>",
		      cmd_status_dump, 4, 0),
	SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(
	ethsw_cmds,
	SHELL_CMD(status, &status_cmds, "RTL8305 switch status", NULL),
	SHELL_CMD(vlan, &vlan_cmds, "RTL8305 VLAN configuration", NULL),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(ethsw, &ethsw_cmds, "RTL8305 switch commands", NULL);
SHELL_CMD_REGISTER(dipsw, NULL, "Print DIP switch state", cmd_dipsw);