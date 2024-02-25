# BitBangChrono

## Overview
BitBangChrono is a specialized tool designed for precision measurement of time intervals between writing to and reading from two pins using an FTDI device in bitbang mode. I am using this to look for regressions on the Linux kernel.

## Requirements
- An FTD232 device with two of it's ports connected to each other
- Linux
- User access to the usb subsystem (I added the following udev rule: `SUBSYSTEM=="usb", MODE="0664", GROUP="plugdev"` and added my user to the `plugdev` group)
- FTDI C libraries
- GCC

## Installation

### Clone the Repository
```bash
git clone https://github.com/petersenna/bitbangchrono.git
cd bitbangchrono
```

### Call make
```bash
make
```