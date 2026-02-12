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
        compatible = "robotronix,ssd1306";
        reg = <0x00>;
        spi-max-frequency = <4000000>;
        dc-gpios = <&gpio 26 GPIO_ACTIVE_HIGH>;
        reset-gpios = <&gpio 24 GPIO_ACTIVE_LOW>;
    };
};
``` 
