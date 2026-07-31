# zephyr-rtl8305-poc

Firmware (STM32 + Zephyr) proof-of-concept to configure the Realtek RTL8305 Ethernet switch chip.

**Setup and build**
1. Create and activate a Python virtual environment:
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
6. Build the application:
```
west build app
```

**Notes**
- Follow Zephyr's docs for target-specific toolchain and environment setup if build fails.
- This repository contains a small sample demonstrating MDIO/GPIO and UART shell code paths used to configure the RTL8305 switch.


