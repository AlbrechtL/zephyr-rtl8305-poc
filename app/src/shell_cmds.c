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

/* vlan - VLAN configuration (not yet implemented). */
static int cmd_vlan(const struct shell *sh, size_t argc, char **argv)
{
	shell_print(sh, "VLAN configuration not implemented");

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	status_cmds,
	SHELL_CMD(link, NULL, "Print link state of all ports", cmd_status_link),
	SHELL_CMD_ARG(dump, NULL, "Dump registers: dump <phy> <start> <count>",
		      cmd_status_dump, 4, 0),
	SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(
	ethsw_cmds,
	SHELL_CMD(status, &status_cmds, "RTL8305 switch status", NULL),
	SHELL_CMD(vlan, NULL, "RTL8305 VLAN configuration (not implemented)",
		  cmd_vlan),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(ethsw, &ethsw_cmds, "RTL8305 switch commands", NULL);
SHELL_CMD_REGISTER(dipsw, NULL, "Print DIP switch state", cmd_dipsw);