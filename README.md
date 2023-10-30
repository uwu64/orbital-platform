![](pictures/r1-1.jpg)

# Orbital Platform

Integrated flight systems board with flight controller, avionics & instrumentation, radio, science, and development support for UC Davis Space and Satellite Systems REALOP-1 mission.

Featured chips: (Many ~~chips~~ are removed for Revision 2. See below.)
 - STM32L476ZGT3 (microcontroller)
 - ASM330LHH (inertial sensor)
 - QMC5883L (magnetometer)
 - ~~ADF7021 (radio front end)~~ 
 - ~~TQP7M9102 (power amplifier)~~ 
 - ~~SKY13453 (rf switch)~~
 - ~~MAX2208 (rf power detector)~~ 
 - W25Q128JVSIQ (flash memory) 
 - MB85RS256B (ferroelectric memory)
 - ~~TMP235 (analog temperature sensor)~~ 

## Documentation:

For an overview of the project, some FAQs, and links to some other documentation, see the [Cattleworks Compendium](https://docs.google.com/document/d/1Hi_DiSkjC-WS4wI39fk3itqsipQI5O-aAOiK9zkmOj8/edit#)

Other useful documents include
- [STM32L47xxx reference manual](https://www.st.com/resource/en/reference_manual/rm0351-stm32l47xxx-stm32l48xxx-stm32l49xxx-and-stm32l4axxx-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
- [STM32L476xx datasheet](https://www.st.com/resource/en/datasheet/stm32l476zg.pdf)

### toolchain:

For now we're using the Keil SDK for compiling the program, but the `gcc-arm-none-eabi` toolchain can also be used directly, with the flags (as used by Keil's invocation) `-xc -std=c11 --target=arm-arm-none-eabi -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -c
-fno-rtti -funsigned-char -fshort-enums -fshort-wchar`

The compiled program can then be uploaded over SWD using your software of choice.

![](pictures/r2-1.jpg)

## Revision 2

Revision 2 hardware of Orbital Platform is in the verification stage. So far, so good!

### Changes 
- Removed radio system: communication function delegated to the new Orbital Imager 
- Revised layout to improve routing, manufactuability, and reliabiltiy
- Inertial sensor (ASM330LHH) is now connected via SPI for reliability
- Magnetometer (QMC5883L) on its own I2C bus for reliability
- Addition of JST-GH connectors for
  - 6x Solar panels (photodiode + thermistor)
  - 4x Solar panels (pyramid design) 
  - ESC control via PWM
  - ESC control via UART
  - Expansion UART
- Less LEDs (still a usable amount)
- Fixed EXTI pins: no overlapping signals
- Fixed USB FS implementation 
- Design for manufacturability: reduced cost and BOM lines


