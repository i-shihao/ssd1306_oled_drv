# An SSD1306 OLED kernel driver over SPI

## Table of Contents

 - [Features](#features)
 - [Hardware Connections](#hardware-connections)
 - [Device Tree Examples](#device-tree-examples)
 - [Build and Install](#build-and-install)
 - [Usage](#usage)
 - [Examples](#examples)
 - [Screenshots](#screenshots)
 - [License](#license)

## Features

 - Linux SPI subsystem based serial communication for OLED
 - Device Tree configured GPIOs for Data/Command and reset
 - Linux fbdev framework integration for userspace access
 - Suspend/Resume support for power management

## Hardware Connections

 Tested with SSD1306 SPI OLED module (3.3V logic).

### Pin Connections

    | OLED |      SIGNAL     | CONNECT  |
    |------|-----------------|----------|
    | VCC  |      Power      | 3.3V     |
    | SCL  |   SPI Clock     | SPI_CLK  |
    | SDA  |   SPI MOSI      | SPI_MOSI |
    | RES  |     Reset       | GPIO     |
    | DC   |  Data/Command   | GPIO     |
    | GND  |     Ground      | GND      |


 Note: This module does not expose a Chip Select pin.

## Device Tree Examples

 - A sample Device Tree node for probing the OLED over SPI bus.

```dts
   spi@7e204000 {
    ssd1306@0 {
        compatible = "solomon,ssd1306";
        reg = <0x00>;
        spi-max-frequency = <4000000>;
        dc-gpios = <&gpio 26 GPIO_ACTIVE_HIGH>;
        reset-gpios = <&gpio 24 GPIO_ACTIVE_LOW>;
    };
};
```
## Build and Install

### Requirements

 - Linux kernel: 6.18
 - Architecture: ARM64
 - Board: Raspberry Pi 4 Model B
 - Compiler: GCC (15.2.1)


### Build

 Run this command inside driver directory
 ```
make
 ```

### Install

 Run the following commands
```
 git clone git@github.com:i-shihao/ssd1306_oled_drv.git
 cd ssd1306_oled_drv
 sudo make
 sudo insmod ssd1306_spi.ko
 sudo dmesg | tail
 sudo rmmod ssd1306_spi
```
### Usage

 After successful module insertion the driver probes the SPI device and
 initialize the device as per the datasheet instruction.The driver exposes
 sysfs entries under its spi device path. Userspace can interact with the
 display by writing strings and integer to those sysfs attributes.Success-
 full command execution and driver intialization can be verified using
 kernel log messages.

### Examples

    Test the below commands in the driver directory

 Example userspace sysfs interaction:
 ```
 ls /sys/bus/spi/device/spi.0/
 ```
 Navigate to sysfs  directory
 ```
 cd /sys/bus/spi/devices/spi.0/
 ```
 write string to display
 ```
 echo hello world > write
  ```
 clear after writing
 ```
 echo 1 > clear
  ```
