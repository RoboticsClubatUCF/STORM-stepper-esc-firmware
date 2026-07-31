# Stepper Driver Firmware for Project STORM

This project contains the firmware for UCF Robotics custom stepper motor controllers which are being designed for Project STORM.

## Getting Started

This project is heavly dependent on cmake and STM32's proprietary libraries for the STMG474. Heres a list of all the tools we use.

- [CMake](https://cmake.org/download/)
- [STM32CubeMX](https://www.st.com/en/development-tools/stm32cubemx.html)
- [ARM GNU Toolchain](https://gitlab.arm.com/tooling/gnu-toolchains-for-arm/-/tree/releases/15.3.rel1?ref_type=heads)

*If you want to get started developing you will need all three of the tool listed above. I hyperlinked all the names so you can just download them by clicking on their name.*

> [!NOTE]
> If you are using VSCode I **strongly suggest** you also install the [STM32CubeIDE extension pack](https://marketplace.visualstudio.com/items?itemName=stmicroelectronics.stm32-vscode-extension) from the VSCode marketplace. The normal clangd and c++ intelisesne extenstions have a hard time detecting STM32's proprietary libraries that are necessary to build the project.

After you install the tools you should be all good to go!

## General Board Components

- MCU: [STM32 G474](https://www.st.com/resource/en/datasheet/stm32g474re.pdf)
- Stepper Motor Drivers: [TMC5160A](https://www.analog.com/media/en/technical-documentation/data-sheets/TMC5160A_datasheet_rev1.18.pdf)
- Magnetic Encoder: [AS5047P](https://look.ams-osram.com/m/d05ee39221f9857/original/AS5047P-DS000324.pdf)

## Architectural Overview

The STM32 G474 will be connected to both the magnetic encoder and the motor drivers via two independent SPI busses. Will will have three of the TMC5160A motor drivers on SPI1 bus while the Magnetic encoder will be on SPI3.

The magnetic encoder is highlighed in red while the motor drivers are highlighted in blue.

![bus-overview](docs/bus_overview2.png)
