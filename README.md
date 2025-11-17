# ILDAWaveX16 - Open Source Laser DAC
[![](https://stanleyprojects.com/projects/ildawavex16/thumbs/cover-text.jpg)](https://stanleyprojects.com/projects/ildawavex16)

**ILDAWaveX16** is an open-source, high-resolution, wireless laser DAC with SD-card playback, network control, and ILDA output.

Traditional laser DACs are often expensive, proprietary, or limited in flexibility, especially for those who want to experiment with laser systems, build custom projectors, or interface with low-cost driver kits.

The goal of this project is to create the "Arduino of laser DACs" - a flexible, hackable platform for makers and enthusiasts that can serve as a foundation for any DIY laser project, capable of real show control, yet simple enough for beginners and hobbyists to learn from.

Full documentation is available at [stanleyprojects.com](https://stanleyprojects.com/projects/ildawavex16)

## Features
- **ESP32-S3 dual-core MCU** @ 240 MHz with native USB, Wi-Fi, and BLE  
- **16-bit, 8-channel DAC (DAC80508)**  
  - X/Y resolution: 65,536 x 65,536  
  - R/G/B resolution: 281 trillion colors  
  - Intensity resolution: 65,536  
  - 16-bit user-programmable channels USR1/USR2  
- **ILDA DB25 connector** for standard projectors  
- **JST SH connectors** for direct wiring to driver kits
- **USB-C power** (no need for bipolar PSU) or optional ±15V external supply  
- **microSD card slot** for ILDA file playback  
- Compact: 55 x 53 mm  

## Firmware

This repository provides a **starting-point firmware**, which is **open-source** and built on the **Arduino framework**. It serves as a foundation for experimenting, learning, and customizing the board for your own projects.  

Although still in development and likely to contain bugs, it includes basic implementations of:
- Playing `.ild` files directly from the SD card  
- **IDN (ILDA Digital Network)** - standard real-time streaming
- **IWP (ILDAWaveProtocol)** - simple, lightweight UDP streaming
- **Web server interface** to control SD card playback, brightness, scan speed, and Wi-Fi settings

Contributions are welcome! Feel free to share improvements, optimizations, or custom firmware to help the community.

## Repository Contents
- `pcb/` - Schematic, BOM  
- `firmware/ILDAWaveX16` - ESP32-S3 source code (Arduino / PlatformIO)  
- `firmware/Python/iwp-ilda.py` - Python script to open `.ild` files and stream over UDP using IWP  
- `firmware/Python/iwp-gen.ipynb` - Jupyter notebook for generating patterns and streaming them via IWP