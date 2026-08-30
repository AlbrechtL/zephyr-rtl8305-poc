/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/mdio.h>
#include <zephyr/drivers/gpio.h>

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

void poll_thread(void);
#define STACKSIZE 512
#define PRIORITY 7
K_THREAD_DEFINE(poll_thread_id, STACKSIZE, poll_thread, NULL, NULL, NULL,
		PRIORITY, 0, 0);

void poll_thread(void)
{
	uint16_t data = 0;
	uint8_t reg_addr = 0;
	uint8_t phy = 0;
	uint8_t phy_num = 8;
	uint8_t link_state[phy_num];
	uint8_t link_state_new = 0;
	bool dip_sw_state[NUM_DIP_SWITCHES] = {0};

	const struct device *mdio_dev = DEVICE_DT_GET(MDIO_NODE);

	memset(link_state, 0, sizeof link_state);

	if (!device_is_ready(mdio_dev)) {
		printf("MDIO-device is not ready\n");
		return;
	}

	for (int i = 0; i < NUM_DIP_SWITCHES; i++) {
		if (!gpio_is_ready_dt(&dip_switches[i])) {
			printf("GPIO-device %d is not ready\n", i);
			return;
		}
		gpio_pin_configure_dt(&dip_switches[i], GPIO_INPUT);
	}

	printf("Polling thread running ...\n");

	for(;;) {
		// Poll PHYs for link state changes
		for(int i=0; i<phy_num; i++) {
			phy = i;
			reg_addr = 1;
			if (mdio_read(mdio_dev, phy, reg_addr, &data) >= 0 && data != UINT16_MAX) {
				link_state_new = (data >> 2) & 0x1;
				if(link_state_new != link_state[i]) {
					if(link_state_new)
						printf("PHY %i link is up\n", phy);
					else
						printf("PHY %i link is down\n", phy);

					link_state[i] = link_state_new;
					//printf("PHY %i, reg %i is 0x%x Link: %i\n", phy, reg_addr, data, link_state[i]);
				}
			}
		}

		// Poll DIP switches
		for (int i = 0; i < NUM_DIP_SWITCHES; i++) {
			bool current_state = gpio_pin_get_dt(&dip_switches[i]);
			if (current_state != dip_sw_state[i]) {
				dip_sw_state[i] = current_state;
				if (dip_sw_state[i] == 0)
					printf("DIP switch %d is ON\n", i);
				else
					printf("DIP switch %d is OFF\n", i);
			}
		}

		k_msleep(0.5e3);
	}
}

int main(void)
{
	printf("Hello %s\n", CONFIG_BOARD_TARGET);
	printf("Build date and time: %s %s\n", __DATE__, __TIME__);

	return 0;
}

