/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * RTL8305 switch register access layer.
 *
 * The RTL8305 exposes its internal register space through pseudo-PHY
 * addresses on the MDIO bus (Clause 22, 16-bit registers). This module
 * wraps the Zephyr MDIO API and provides helpers used by the shell
 * commands.
 */

#ifndef RTL8305_H
#define RTL8305_H

#include <stdint.h>
#include <zephyr/device.h>

/* Placeholder register addresses - fill in from the RTL8305 datasheet. */
#define RTL8305_CHIP_ID_REG 0x00 /* Chip ID / version register */

/* MII status register (reg 1) link status bit. */
#define RTL8305_MII_STATUS_REG 0x01
#define RTL8305_MII_LINK_BIT   BIT(2)

/**
 * @brief Read a 16-bit RTL8305 register via MDIO.
 *
 * @param dev   MDIO device
 * @param phy   Pseudo-PHY address (0x0 - 0x8)
 * @param reg   Register address
 * @param val   Pointer to store the read value
 *
 * @return 0 on success, negative errno otherwise.
 */
int rtl8305_read(const struct device *dev, uint8_t phy, uint8_t reg,
		 uint16_t *val);

/**
 * @brief Write a 16-bit RTL8305 register via MDIO.
 *
 * @param dev   MDIO device
 * @param phy   Pseudo-PHY address (0x0 - 0x8)
 * @param reg   Register address
 * @param val   Value to write
 *
 * @return 0 on success, negative errno otherwise.
 */
int rtl8305_write(const struct device *dev, uint8_t phy, uint8_t reg,
		  uint16_t val);

/**
 * @brief Read the RTL8305 chip ID / version.
 *
 * @param dev   MDIO device
 * @param id    Pointer to store the chip ID
 *
 * @return 0 on success, negative errno otherwise.
 */
int rtl8305_get_chip_id(const struct device *dev, uint16_t *id);

/**
 * @brief Read the link state of a port.
 *
 * @param dev   MDIO device
 * @param phy   Pseudo-PHY address (0x0 - 0x8)
 * @param link  Pointer to store the link state (true = up)
 *
 * @return 0 on success, negative errno otherwise.
 */
int rtl8305_get_link_state(const struct device *dev, uint8_t phy, bool *link);

/**
 * @brief Dump a range of RTL8305 registers.
 *
 * @param dev    MDIO device
 * @param phy    Pseudo-PHY address (0x0 - 0x8)
 * @param start  First register address
 * @param count  Number of registers to dump
 *
 * @return 0 on success, negative errno otherwise.
 */
int rtl8305_dump(const struct device *dev, uint8_t phy, uint8_t start,
		 uint8_t count);

#endif /* RTL8305_H */