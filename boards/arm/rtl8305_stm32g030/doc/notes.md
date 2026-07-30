# STM32G030 pin usage

# GPIO
## DIP switches
PB8 (32)
PB9 (1)
PC14 (2)
PC15 (3)

## LEDs
PA0 (7)
PA1 (8)
PA2 (9)
PA3 (10)
PA4 (11)
PA5 (12)
PA6 (13)
PA7 (14)
PB0 (15)
PB1 (16)

# UART
PA9 (19) TX
PA10 (21) RX

# MDIO
PA12 (23) MDIO
PA11 (22) MDC

uart:~$ mdio scan mdio
Scanning bus for devices. Reading register 0x0
Found MDIO device @ 0x0
Found MDIO device @ 0x2
Found MDIO device @ 0x5
Found MDIO device @ 0x6
Found MDIO device @ 0x7
Found MDIO device @ 0x8
6 devices found on mdio

mit Link PHY 0, reg 1 is 0x782d  '0b0111100000101101'
ohne Link PHY 0, reg 1 is 0x7809 '0b0111100000001001'

# RESET
PB7 (31) System reset
