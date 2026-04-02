# MSP432 Single-Sided Pong
A single-player Pong game built in C for the MSP432P401R microcontroller. Built for a microcontrollers course at UC Davis.

## Hardware
- MSP432P401R microcontroller
- Crystalfontz 128x128 color LCD display
- Analog joystick via ADC input

## Key Concepts
- SysTick timer interrupts for game loop timing
- ADC14 interrupt-driven analog input for paddle control
- LCD graphics using TI GrLib
- Collision detection in C
