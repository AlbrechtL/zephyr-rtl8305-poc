/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/mdio.h>

#define MDIO_NODE DT_NODELABEL(mdio0)

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

	const struct device *dev = DEVICE_DT_GET(MDIO_NODE);

	memset(link_state, 0, sizeof link_state);

	if (!device_is_ready(dev)) {
		printf("MDIO-device is not ready\n");
	}

	printf("Polling thread running ...\n");

	for(;;) {
		for(int i=0; i<phy_num; i++) {
			phy = i;
			reg_addr = 1;
			if (mdio_read(dev, phy, reg_addr, &data) >= 0 && data != UINT16_MAX) {
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

		k_msleep(0.5e3);
	}
}

int main(void)
{
	printf("Hello %s\n", CONFIG_BOARD_TARGET);
	printf("Build date and time: %s %s\n", __DATE__, __TIME__);

	return 0;
}

