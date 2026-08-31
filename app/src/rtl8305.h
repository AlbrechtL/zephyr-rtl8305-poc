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

/*
 * VLAN configuration registers.
 *
 * The RTL8305 supports 5 VLAN groups (A-E). Each port is mapped to a
 * group via its Port VLAN Index. Each group has a 12-bit VID and a
 * 5-bit member set (bit N = port N). All of these are EEPROM/Pin
 * registers: after writing them via SMI a soft reset (PHY3 Reg16.15)
 * is required for the changes to take effect.
 */

/* Pseudo-PHY addresses used for the VLAN registers. */
#define RTL8305_PHY_PORT_INDEX 0x0 /* PHY0: port VLAN index regs 21/22 */
#define RTL8305_PHY_VLAN       0x1 /* PHY1: group VID + member regs 24-31 */
#define RTL8305_PHY_VLAN_CTRL  0x2 /* PHY2: VLAN control reg 17 */
#define RTL8305_PHY_PORT_CTRL  0x3 /* PHY3: port control reg 16 (soft reset) */

/* PHY0 Reg21: Port 0-3 VLAN index. */
#define RTL8305_REG_PORT_INDEX_0_3 0x15
#define RTL8305_P0VLANINDEX_SHIFT  0
#define RTL8305_P1VLANINDEX_SHIFT  4
#define RTL8305_P2VLANINDEX_SHIFT  8
#define RTL8305_P3VLANINDEX_SHIFT  12
#define RTL8305_VLANINDEX_MASK     0x7

/* PHY0 Reg22: Port 4 VLAN index. */
#define RTL8305_REG_PORT_INDEX_4 0x16
#define RTL8305_P4VLANINDEX_SHIFT 0

/* PHY1 Reg24-31: group VID and member set. */
#define RTL8305_REG_VIDA 0x18 /* Reg24: VIDA[11:0] */
#define RTL8305_REG_VIDB_HI 0x19 /* Reg25: VIDB[11:8] in [15:8] */
#define RTL8305_REG_VIDB_LO 0x1a /* Reg26: VIDB[7:0] in [3:0] */
#define RTL8305_REG_VIDC 0x1b /* Reg27: VIDC[11:0] */
#define RTL8305_REG_VIDD_HI 0x1c /* Reg28: VIDD[11:8] in [15:8] */
#define RTL8305_REG_VIDD_LO 0x1d /* Reg29: VIDD[7:0] in [3:0] */
#define RTL8305_REG_VIDE 0x1e /* Reg30: VIDE[11:0] */

#define RTL8305_REG_MEMBERA 0x19 /* Reg25: MemberA[4:0] */
#define RTL8305_REG_MEMBERB 0x1a /* Reg26: MemberB[4:0] in [12:8] */
#define RTL8305_REG_MEMBERC 0x1c /* Reg28: MemberC[4:0] */
#define RTL8305_REG_MEMBERD 0x1d /* Reg29: MemberD[4:0] in [12:8] */
#define RTL8305_REG_MEMBERE 0x1f /* Reg31: MemberE[4:0] */

#define RTL8305_MEMBER_SHIFT 8
#define RTL8305_MEMBER_MASK  0x1f

/* PHY2 Reg17: VLAN control. */
#define RTL8305_REG_VLAN_CTRL 0x11
#define RTL8305_VLAN_CTRL_DISVLAN        BIT(5)
#define RTL8305_VLAN_CTRL_DISTAGAWARE    BIT(4)
#define RTL8305_VLAN_CTRL_DISMEMFILTER   BIT(3)
#define RTL8305_VLAN_CTRL_DISTAGADMIT    BIT(2)
#define RTL8305_VLAN_CTRL_DISLEAKY       BIT(1)
#define RTL8305_VLAN_CTRL_DISARP         BIT(0)

/* PHY3 Reg16: port control / soft reset. */
#define RTL8305_REG_PORT_CTRL 0x10
#define RTL8305_PORT_CTRL_SOFTRESET BIT(15)

/* Number of VLAN groups and ports. */
#define RTL8305_NUM_VLAN_GROUPS 5
#define RTL8305_NUM_PORTS       5

/* VLAN group identifiers (0=A, 1=B, 2=C, 3=D, 4=E). */
enum rtl8305_vlan_group {
	RTL8305_VLAN_A = 0,
	RTL8305_VLAN_B,
	RTL8305_VLAN_C,
	RTL8305_VLAN_D,
	RTL8305_VLAN_E,
};

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

/**
 * @brief Get the VLAN group index a port is assigned to.
 *
 * @param dev    MDIO device
 * @param port   Port number (0 - 4)
 * @param index  Pointer to store the VLAN group index (0=A .. 4=E)
 *
 * @return 0 on success, negative errno otherwise.
 */
int rtl8305_vlan_get_port_index(const struct device *dev, uint8_t port,
				uint8_t *index);

/**
 * @brief Set the VLAN group index a port is assigned to.
 *
 * @param dev    MDIO device
 * @param port   Port number (0 - 4)
 * @param index  VLAN group index (0=A .. 4=E)
 *
 * @return 0 on success, negative errno otherwise.
 */
int rtl8305_vlan_set_port_index(const struct device *dev, uint8_t port,
				uint8_t index);

/**
 * @brief Get the 12-bit VID of a VLAN group.
 *
 * @param dev    MDIO device
 * @param group  VLAN group (0=A .. 4=E)
 * @param vid    Pointer to store the VID
 *
 * @return 0 on success, negative errno otherwise.
 */
int rtl8305_vlan_get_vid(const struct device *dev, uint8_t group,
			 uint16_t *vid);

/**
 * @brief Set the 12-bit VID of a VLAN group.
 *
 * @param dev    MDIO device
 * @param group  VLAN group (0=A .. 4=E)
 * @param vid    VID value (0 - 0xFFF)
 *
 * @return 0 on success, negative errno otherwise.
 */
int rtl8305_vlan_set_vid(const struct device *dev, uint8_t group,
			 uint16_t vid);

/**
 * @brief Get the member set of a VLAN group.
 *
 * @param dev     MDIO device
 * @param group   VLAN group (0=A .. 4=E)
 * @param member  Pointer to store the member set (bit N = port N)
 *
 * @return 0 on success, negative errno otherwise.
 */
int rtl8305_vlan_get_member(const struct device *dev, uint8_t group,
			    uint8_t *member);

/**
 * @brief Set the member set of a VLAN group.
 *
 * @param dev     MDIO device
 * @param group   VLAN group (0=A .. 4=E)
 * @param member  Member set (bit N = port N)
 *
 * @return 0 on success, negative errno otherwise.
 */
int rtl8305_vlan_set_member(const struct device *dev, uint8_t group,
			    uint8_t member);

/**
 * @brief Read the VLAN control register.
 *
 * @param dev   MDIO device
 * @param ctrl  Pointer to store the control register value
 *
 * @return 0 on success, negative errno otherwise.
 */
int rtl8305_vlan_get_ctrl(const struct device *dev, uint16_t *ctrl);

/**
 * @brief Write the VLAN control register.
 *
 * @param dev   MDIO device
 * @param ctrl  Control register value
 *
 * @return 0 on success, negative errno otherwise.
 */
int rtl8305_vlan_set_ctrl(const struct device *dev, uint16_t ctrl);

/**
 * @brief Trigger a soft reset (required after VLAN register writes).
 *
 * @param dev  MDIO device
 *
 * @return 0 on success, negative errno otherwise.
 */
int rtl8305_soft_reset(const struct device *dev);

/**
 * @brief Assign a port to a LAN VLAN.
 *
 * Maps the port to its default group (port N -> group N), sets the
 * group VID, assigns the port to that group, and recomputes the member
 * set of every group so that ports sharing the same VID can
 * communicate. Ends with a soft reset.
 *
 * @param dev   MDIO device
 * @param port  Port number (0 - 4)
 * @param vid   VID value (0 - 0xFFF)
 *
 * @return 0 on success, negative errno otherwise.
 */
int rtl8305_vlan_lan(const struct device *dev, uint8_t port, uint16_t vid);

#endif /* RTL8305_H */