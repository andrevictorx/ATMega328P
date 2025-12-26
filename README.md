# 📟 ATmega328P – Embedded C Code Collection

This repository contains a **collection of bare-metal C projects** developed for the **ATmega328P microcontroller**, focused on **low-level embedded systems programming**, without using the Arduino framework.

The goal of this repository is to demonstrate **direct register manipulation**, **interrupt handling**, **timers**, **PWM**, **ADC**, **UART**, and **firmware architecture** in AVR microcontrollers.

---

## 🧠 Featured Project – RGB Control with PWM, ADC, UART, and 7-Segment Displays

The main project in this repository implements a **complete embedded system** with the following features:

### 🔴🟢🔵 RGB LED Control
- Independent **PWM duty-cycle control** for:
  - Red
  - Green
  - Blue
- Adjustment via **physical buttons** (increment/decrement)
- Active color selection via a dedicated button
- PWM implementation:
  - **Hardware PWM (Timer0 – OC0A / OC0B)**
  - **Software PWM** for the blue channel

---

### ⏱️ Timers and Interrupts
- **Timer0**
  - Prescaler configuration
  - Overflow interrupt enabled
  - Fast PWM mode
- **Timer1**
  - CTC mode
  - Approximate **2-second timing**
  - Used for temporary display control
- **Pin Change Interrupt (PCINT)**
  - Button detection (PD2, PD3, PD4)
  - Software-based debounce logic
- **USART RX/TX Interrupts**
  - Non-blocking serial communication

---

### 📊 7-Segment Displays (Multiplexed)
- Two 7-segment displays
- Display of:
  - Selected color (R / G / B)
  - Duty cycle percentage (0–99%)
- Direct control via:
  - PORTB
  - PORTC
  - PORTD
- BCD-to-7-segment decoder implemented using a lookup table

---

### 🔌 Serial Communication (UART)
- Baud rate: **9600 bps @ 16 MHz**
- Command reception via UART
- Color selection and PWM adjustment through serial commands
- Status messages transmitted over UART
- Simple frame-based command parser

---

### 📐 Analog-to-Digital Conversion (ADC)
- Continuous ADC reading (ADC0 channel)
- Analog voltage to digital conversion
- Voltage calculation (0–5V)
- Integer and decimal separation
- Designed for integration with analog sensors

---

## 🧩 Firmware Architecture

- **Bare-metal programming** (no HAL / Arduino framework)
- Direct use of:
  - `DDRx`
  - `PORTx`
  - `PINx`
  - `TCCRn`
  - `OCRn`
  - `TIMSK`
  - `UCSR`
- Architecture based on:
  - Dedicated setup functions
  - Interrupt Service Routines (ISRs)
  - Non-blocking main loop
- Communication between ISRs and the main loop via **global flags**

---

## 🛠️ Technologies and Concepts Used

- **C programming language**
- AVR-GCC toolchain
- Register-level programming
- External and internal interrupts
- Timers / Counters
- PWM (hardware and software)
- ADC
- UART
- Multiplexed displays
- Simple data structures
- Implicit state machines
- Embedded firmware best practices

---

## 🔧 Target Hardware

- Microcontroller: **ATmega328P**
- Clock frequency: **16 MHz**
- RGB LEDs
- Push buttons with internal pull-ups
- Two 7-segment displays
- 5V power supply
- Serial interface (USB-to-UART)

---

## 🚀 Repository Purpose

This repository aims to:

- Serve as a **technical portfolio** for embedded firmware development
- Demonstrate **low-level microcontroller expertise**
- Support studies in:
  - Embedded systems
  - Digital electronics
  - Microcontroller architecture
- Act as reference material for technical interviews in:
  - Firmware Engineer
  - Embedded Software Developer
  - Hardware/Firmware Integration roles

---

## 👤 Author

**André Victor Xavier Pires**  
Electrical Engineering – UFPR  
Focus on Embedded Electronic Systems  

🔗 LinkedIn: *https://www.linkedin.com/in/andre-victor-xavier-pires/*  
📧 Email: *andrevictorxavierpires@gmail.com*  

---

## 📌 Notes

This project was developed with an **educational and technical focus**, prioritizing functional clarity, hardware control, and full ownership of the microcontroller.
