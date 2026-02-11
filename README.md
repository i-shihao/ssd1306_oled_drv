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
