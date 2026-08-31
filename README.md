# zephyr-rtl8305-poc

Firmware (STM32 + Zephyr) proof-of-concept to configure the Realtek RTL8305 Ethernet switch chip.

**Setup and build**
1. Create a new folder and activate a Python virtual environment:
```
python3 -m venv .venv
source .venv/bin/activate
```
2. Install west:
```
pip install west
```
3. Initialize the Zephyr workspace and fetch this repository:
```
west init -m https://github.com/AlbrechtL/zephyr-rtl8305-poc.git --mr master
cd rtl8305_poc
```
4. Update manifests and fetch modules:
```
west update
```
5. Install Python packages declared by `west`:
```
west packages pip --install
```
6. Export a Zephyr CMake package:
```
west zephyr-export
```
7. Install the Zephyr SDK (necessary only once):
```
west sdk install
```
8. Build the application:
```
west build app
```
9. Flash the application:
```
west flash
```
or
```
west flash --runner openocd
```

More information can be found at https://docs.zephyrproject.org/latest/develop/getting_started/index.html.

**Shell commands**

The application provides an `ethsw` shell command to configure the RTL8305 switch, and a `dipsw` command to read the DIP switches:

```
dipsw
    Print the state of all DIP switches.

ethsw status link
    Print the link state of all ports (PHY 0x0 - 0x8).

ethsw status dump <phy> <start> <count>
    Dump a range of raw RTL8305 registers via MDIO.
    <phy>   Pseudo-PHY address (0x0 - 0x8)
    <start> First register address
    <count> Number of registers to dump

ethsw vlan show
    Show the current VLAN configuration (control register, each group's
    VID and member set, and each port's VLAN group).

ethsw vlan lan <port> <vid>
    Assign a port to a LAN VLAN. Port N maps to VLAN group N (0=A, 1=B,
    2=C, 3=D, 4=E). Sets the group VID, assigns the port to that group,
    and recomputes the member sets so that ports sharing the same VID
    can communicate. Ends with a soft reset.
    <port> Port number (0 - 4)
    <vid>  VLAN ID (0 - 0xFFF)

ethsw vlan vid <group> <vid>
    Set the 12-bit VID of a VLAN group. Ends with a soft reset.
    <group> VLAN group (0=A, 1=B, 2=C, 3=D, 4=E)
    <vid>   VLAN ID (0 - 0xFFF)

ethsw vlan member <group> <member>
    Set the member set of a VLAN group (bit N = port N). Ends with a
    soft reset.
    <group>  VLAN group (0=A, 1=B, 2=C, 3=D, 4=E)
    <member> Member bitmask (0 - 0x1F)
```

**Notes**
- Follow Zephyr's docs for target-specific toolchain and environment setup if build fails.
- This repository contains a small sample demonstrating MDIO/GPIO and UART shell code paths used to configure the RTL8305 switch.

**Disclaimer**
- Parts of this code were created with the assistance of an AI assistant. The code should be reviewed and tested before use in production.

